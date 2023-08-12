/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite manager
 */

#include <chrono>

#include <Kube/Core/Platform.hpp>

#if KUBE_COMPILER_GCC | KUBE_COMPILER_CLANG
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wold-style-cast"
# pragma GCC diagnostic ignored "-Wcast-qual"
# pragma GCC diagnostic ignored "-Wconversion"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if KUBE_COMPILER_GCC | KUBE_COMPILER_CLANG
# pragma GCC diagnostic pop
#endif

#include <Kube/Core/Assert.hpp>
#include <Kube/GPU/GPU.hpp>
#include <Kube/GPU/Buffer.hpp>
#include <Kube/GPU/DescriptorSetUpdate.hpp>
#include <Kube/IO/File.hpp>

#include "SpriteManager.hpp"

using namespace kF;

UI::SpriteManager::SpriteManager(void) noexcept
    : _maxSpriteCount([this] {
        const auto max = std::min(
            DefaultMaxSpriteCount.value,
            parent().physicalDevice().limits().maxDescriptorSetSampledImages
        );
        kFEnsure(max != 0u, "UI::SpriteManager: Maximum sprite count cannot be 0");
        return max;
    }())
    , _sampler(GPU::SamplerModel(
            GPU::SamplerCreateFlags::None,
            GPU::Filter::Linear,
            GPU::Filter::Linear,
            GPU::SamplerMipmapMode::Linear,
            GPU::SamplerAddressMode::ClampToBorder,
            GPU::SamplerAddressMode::ClampToBorder,
            GPU::SamplerAddressMode::ClampToBorder,
            false, 0.0f, // Anisotropy
            false, GPU::CompareOp::Never, // Compare
            0.0f, 0.0f, 0.0f, // Lod
            GPU::BorderColor::FloatTransparentBlack, // Border
            false // Unormalized
        ))
    , _descriptorSetLayout(GPU::DescriptorSetLayout::Make(
        GPU::DescriptorSetLayoutCreateFlags::UpdateAfterBindPool,
        {
            GPU::DescriptorSetLayoutBinding(
                0,
                GPU::DescriptorType::CombinedImageSampler,
                DefaultMaxSpriteCount,
                Core::MakeFlags(GPU::ShaderStageFlags::Compute, GPU::ShaderStageFlags::Vertex, GPU::ShaderStageFlags::Fragment)
            ),
            GPU::DescriptorSetLayoutBinding(
                1,
                GPU::DescriptorType::StorageBuffer,
                1,
                Core::MakeFlags(GPU::ShaderStageFlags::Compute, GPU::ShaderStageFlags::Vertex, GPU::ShaderStageFlags::Fragment)
            )
        },
        {
            Core::MakeFlags(
                GPU::DescriptorBindingFlags::UpdateAfterBind,
                GPU::DescriptorBindingFlags::UpdateUnusedWhilePending,
                GPU::DescriptorBindingFlags::PartiallyBound
            ),
            GPU::DescriptorBindingFlags()
        }
    ))
    , _commandPool(GPU::QueueType::Transfer, GPU::CommandPoolCreateFlags::Transient)
    , _command(_commandPool.add(GPU::CommandLevel::Primary))
    , _perFrameCache(parent().frameCount(), [this] {
        FrameCache frameCache {
            .descriptorPool = GPU::DescriptorPool::Make(
                GPU::DescriptorPoolCreateFlags::UpdateAfterBind,
                1,
                {
                    GPU::DescriptorPoolSize(GPU::DescriptorType::CombinedImageSampler, DefaultMaxSpriteCount),
                    GPU::DescriptorPoolSize(GPU::DescriptorType::StorageBuffer, 1)
                }
            ),
            .descriptorSet = frameCache.descriptorPool.allocate(_descriptorSetLayout),
            .spriteSizesBuffer = GPU::Buffer::MakeExclusive(
                _maxSpriteCount * sizeof(Size),
                Core::MakeFlags(GPU::BufferUsageFlags::TransferDst, GPU::BufferUsageFlags::StorageBuffer)
            ),
            .spriteSizesAllocation = GPU::MemoryAllocation::MakeLocal(frameCache.spriteSizesBuffer),
        };
        return frameCache;
    })
{
    parent().frameAcquiredDispatcher().add([this](const GPU::FrameIndex frameIndex) noexcept {
        _perFrameCache.setCurrentFrame(frameIndex);
    });

    // Add default sprite
    const Color defaultBufferData { 255, 80, 255, 255 };
    const auto defaultSpriteIndex = addImpl(Core::HashedName {}, 0.0f);
    kFEnsure(defaultSpriteIndex == DefaultSprite, "UI::SpriteManager: Implementation error");
    load(defaultSpriteIndex, SpriteBuffer {
        .data = &defaultBufferData,
        .extent = GPU::Extent2D { 1, 1 }
    });

    // Initialize descriptor sets
    const auto &defaultImageView = _spriteCaches.at(defaultSpriteIndex).imageView;
    const Core::Vector<GPU::DescriptorImageInfo, UIAllocator> imageInfos(
        _maxSpriteCount,
        GPU::DescriptorImageInfo(_sampler, defaultImageView, GPU::ImageLayout::ShaderReadOnlyOptimal)
    );
    for (auto &frameCache : _perFrameCache) {
        const GPU::DescriptorBufferInfo bufferInfo(frameCache.spriteSizesBuffer);
        GPU::DescriptorSetUpdate::UpdateWrite({
            GPU::DescriptorSetWriteModel(
                frameCache.descriptorSet,
                0,
                0,
                GPU::DescriptorType::CombinedImageSampler,
                imageInfos.begin(),
                imageInfos.end()
            ),
            GPU::DescriptorSetWriteModel(
                frameCache.descriptorSet,
                1,
                0,
                GPU::DescriptorType::StorageBuffer,
                &bufferInfo,
                &bufferInfo + 1
            )
        });
    }
}

UI::Sprite UI::SpriteManager::add(const std::string_view &path, const float removeDelaySeconds) noexcept
{
    using namespace GPU;

    kFEnsure(!path.empty(), "UI::SpriteManager::add: Empty path");

    // Try to find an existing instance of the queried sprite
    const auto spriteName = Core::Hash(path);
    if (const auto it = _spriteNames.find(spriteName); it != _spriteNames.end()) [[likely]] {
        const auto spriteIndex = SpriteIndex { _spriteNames.indexOf(it) };
        if (++_spriteCaches.at(spriteIndex).counter.refCount == 1) [[unlikely]]
            cancelDelayedRemove(spriteIndex);
        return Sprite(*this, spriteIndex);
    }

    // Decode image
    int x {}, y {}, channelCount {};
    Color *data {};
    // If file is a resource it is already loaded in RAM
    if (const IO::File file(path); file.isResource()) {
        const auto range = file.queryResource();
        data = reinterpret_cast<Color *>(
            ::stbi_load_from_memory(range.begin(), static_cast<int>(range.size()), &x, &y, &channelCount, ::STBI_rgb_alpha)
        );
    // Else we need to load file from flash storage
    } else {
        std::string str(path);
        data = reinterpret_cast<Color *>(
            ::stbi_load(str.c_str(), &x, &y, &channelCount, ::STBI_rgb_alpha)
        );
    }
    if (!data) {
        kFError("[SpriteManager] Couldn't load sprite at path: ", path);
        return UI::Sprite();
    }

    // Reserve sprite index
    kFEnsure(!path.empty(), "UI::SpriteManager::add: Path cannot be empty");
    const auto spriteIndex = addImpl(spriteName, removeDelaySeconds);


    // Build sprite cache at 'spriteIndex'
    load(spriteIndex, SpriteBuffer {
        .data = data,
        .extent = { static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y) }
    });

    // Release decoded image
    ::stbi_image_free(data);

#if KUBE_DEBUG_BUILD
    kFInfo("[UI] Init sprite ", spriteIndex, ":\t Path '", path, "' Extent (", x, ", ", y, ')');
#endif

    // Build sprite
    return Sprite(*this, spriteIndex);
}

UI::Sprite UI::SpriteManager::add(const SpriteBuffer &spriteBuffer, const float removeDelaySeconds) noexcept
{
    // Reserve sprite index
    const auto spriteIndex = addImpl(Core::HashedName {}, removeDelaySeconds);

    // Build sprite cache at 'spriteIndex'
    load(spriteIndex, spriteBuffer);

#if KUBE_DEBUG_BUILD
    kFInfo("[UI] Init sprite ", spriteIndex, ":\t Path '{Buffer}' Extent (", spriteBuffer.extent.width, ", ", spriteBuffer.extent.height, ')');
#endif

    // Build sprite
    return Sprite(*this, spriteIndex);
}

UI::Sprite UI::SpriteManager::add(const Core::IteratorRange<const std::uint8_t *> &encodedData, const float removeDelaySeconds) noexcept
{
    // Reserve sprite index
    const auto spriteIndex = addImpl(Core::HashedName {}, removeDelaySeconds);

    // Decode data
    int x {};
    int y {};
    int channelCount {};
    const auto data = reinterpret_cast<Color *>(
        ::stbi_load_from_memory(encodedData.begin(), static_cast<int>(encodedData.size()), &x, &y, &channelCount, ::STBI_rgb_alpha)
    );
    if (!data) {
        kFError("[SpriteManager] Couldn't decode raw sprite data");
        return UI::Sprite();
    }

    // Build sprite cache at 'spriteIndex'
    load(spriteIndex, SpriteBuffer {
        .data = data,
        .extent = { static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y) }
    });

    // Release decoded image
    ::stbi_image_free(data);

#if KUBE_DEBUG_BUILD
    kFInfo("[UI] Init sprite ", spriteIndex, ":\t Path '{Encoded Buffer}' Extent (", x, ", ", y, ')');
#endif

    // Build sprite
    return Sprite(*this, spriteIndex);
}

UI::SpriteIndex UI::SpriteManager::addImpl(const Core::HashedName spriteName, const float removeDelaySeconds) noexcept
{
    SpriteIndex spriteIndex {};
    if (!_spriteFreeList.empty()) {
        spriteIndex.value = _spriteFreeList.back();
        _spriteFreeList.pop();
    } else {
        spriteIndex.value = _spriteNames.size();
        _spriteNames.push();
        _spriteCaches.push();
        _spriteSizes.push();
    }

    // Set sprite reference count and name
    _spriteCaches.at(spriteIndex).counter = SpriteCache::Counter { .refCount = 1u, .removeDelaySeconds = removeDelaySeconds };
    _spriteNames.at(spriteIndex) = spriteName;
    return spriteIndex;
}

void UI::SpriteManager::load(const SpriteIndex spriteIndex, const SpriteBuffer &spriteBuffer) noexcept
{
    using namespace GPU;

    // Copy image to staging buffer
    const auto imageSize = spriteBuffer.extent.width * spriteBuffer.extent.height;
    const auto stagingBuffer = Buffer::MakeStaging(imageSize * sizeof(Color));
    auto stagingAllocation = MemoryAllocation::MakeStaging(stagingBuffer);
    stagingAllocation.memoryMap(spriteBuffer.data, spriteBuffer.data + imageSize);

    // Set sprite size
    _spriteSizes.at(spriteIndex) = Size(static_cast<Pixel>(spriteBuffer.extent.width), static_cast<Pixel>(spriteBuffer.extent.height));

    // Set sprite cache
    auto &spriteCache = _spriteCaches.at(spriteIndex);
    spriteCache.image = Image::MakeSingleLayer2D(
        spriteBuffer.extent,
        Format::R8G8B8A8_UNORM,
        Core::MakeFlags(ImageUsageFlags::TransferDst, ImageUsageFlags::Sampled),
        ImageTiling::TilingOptimal
    );
    spriteCache.memoryAllocation = MemoryAllocation::MakeLocal(spriteCache.image);
    spriteCache.imageView = ImageView(ImageViewModel(
        ImageViewCreateFlags::None,
        spriteCache.image,
        ImageViewType::Image2D,
        Format::R8G8B8A8_UNORM,
        ComponentMapping(),
        ImageSubresourceRange(ImageAspectFlags::Color)
    ));

    // Record transfer command
    _commandPool.reset();
    _commandPool.record(_command, CommandBufferUsageFlags::OneTimeSubmit,
        [&spriteBuffer, &stagingBuffer, &spriteCache](const CommandRecorder &recorder) {
            // Transition device image into transfer dest
            recorder.pipelineBarrier(
                PipelineStageFlags::TopOfPipe, PipelineStageFlags::Transfer,
                DependencyFlags::None,
                ImageMemoryBarrier(
                    AccessFlags::None, AccessFlags::TransferWrite,
                    ImageLayout::Undefined, ImageLayout::TransferDstOptimal,
                    IgnoredFamilyQueue, IgnoredFamilyQueue,
                    spriteCache.image,
                    ImageSubresourceRange(ImageAspectFlags::Color)
                )
            );

            // Copy staging buffer to device image
            recorder.copyBufferToImage(
                stagingBuffer,
                spriteCache.image,
                ImageLayout::TransferDstOptimal,
                BufferImageCopy(
                    0, spriteBuffer.extent.width, spriteBuffer.extent.height,
                    ImageSubresourceLayers(ImageAspectFlags::Color),
                    Offset3D(),
                    Extent3D(spriteBuffer.extent.width, spriteBuffer.extent.height, 1u)
                )
            );

            // Transition device image into read only
            recorder.pipelineBarrier(
                PipelineStageFlags::Transfer, PipelineStageFlags::AllCommands,
                DependencyFlags::None,
                ImageMemoryBarrier(
                    AccessFlags::TransferWrite, AccessFlags::ShaderRead,
                    ImageLayout::TransferDstOptimal, ImageLayout::ShaderReadOnlyOptimal,
                    IgnoredFamilyQueue, IgnoredFamilyQueue,
                    spriteCache.image,
                    ImageSubresourceRange(ImageAspectFlags::Color)
                )
            );
        }
    );

    // Submit transfer command
    _fence.reset();
    parent().commandDispatcher().dispatch(
        QueueType::Transfer,
        { _command },
        {},
        {},
        {},
        _fence
    );

    // Add insert events to frame caches
    for (auto &frameCache : _perFrameCache) {
        frameCache.events.push(Event {
            .type = Event::Type::Add,
            .spriteIndex = spriteIndex
        });
    }

    // Wait until transfer completed
    _fence.wait();
}

void UI::SpriteManager::decrementRefCount(const SpriteIndex spriteIndex) noexcept
{
    // Check sprite reference counter
    if (--_spriteCaches.at(spriteIndex).counter.refCount) [[likely]]
        return;

    // Add sprite to delayed remove list
    _spriteDelayedRemoves.push(SpriteDelayedRemove {
        .spriteIndex = spriteIndex,
        .frameCount = _perFrameCache.count() - 1u,
        .beginTimestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count()
    });

#if KUBE_DEBUG_BUILD
    kFInfo("[UI] Delete sprite required ", spriteIndex);
#endif
}

void UI::SpriteManager::prepareFrameCache(void) noexcept
{
    updateDelayedRemoves();

    auto &frameCache = _perFrameCache.current();
    const auto eventCount = frameCache.events.size();

    if (!eventCount) [[likely]]
        return;

    // Prepare image infos
    Core::SmallVector<GPU::DescriptorImageInfo, 8, UIAllocator> imageInfos(
        eventCount,
        [this, &frameCache](const auto index) {
            const auto &event = frameCache.events.at(index);
            const auto targetSprite = event.type == Event::Type::Add ? event.spriteIndex : DefaultSprite;
            return GPU::DescriptorImageInfo(
                _sampler,
                _spriteCaches.at(targetSprite).imageView,
                GPU::ImageLayout::ShaderReadOnlyOptimal
            );
        }
    );

    // Prepare descriptor set write models
    Core::SmallVector<GPU::DescriptorSetWriteModel, 8, UIAllocator> models(
        eventCount,
        [&frameCache, &imageInfos](const auto index) {
            return GPU::DescriptorSetWriteModel(
                frameCache.descriptorSet,
                0,
                frameCache.events.at(index).spriteIndex,
                GPU::DescriptorType::CombinedImageSampler,
                imageInfos.begin() + index,
                imageInfos.begin() + index + 1
            );
        }
    );

    // Store sprite size update indexes
    frameCache.spriteSizesUpdateIndexes.resize(
        frameCache.events.begin(),
        frameCache.events.end(),
        [](const auto &event) { return event.spriteIndex; }
    );

    // Clear events
    frameCache.events.clear();

    // Write descriptors
    GPU::DescriptorSetUpdate::UpdateWrite(models.begin(), models.end());
}

void UI::SpriteManager::transferSpriteSizesBuffer(const GPU::CommandRecorder &recorder) noexcept
{
    // Get current frame cache
    auto &frameCache = _perFrameCache.current();

    // Reset staging buffer
    // @todo Investigate if the buffer can still be in use at this time
    frameCache.spriteSizesStagingBuffer = {};
    frameCache.spriteSizesStagingAllocation = {};

    // Nothing to do here
    if (frameCache.spriteSizesUpdateIndexes.empty())
        return;

    // Prepare staging buffer
    frameCache.spriteSizesStagingBuffer = GPU::Buffer::MakeStaging(frameCache.spriteSizesUpdateIndexes.size() * sizeof(Size));
    frameCache.spriteSizesStagingAllocation = GPU::MemoryAllocation::MakeStaging(frameCache.spriteSizesStagingBuffer);
    const auto mappedMemory = frameCache.spriteSizesStagingAllocation.beginMemoryMap<Size>();
    for (auto index = 0u, count = frameCache.spriteSizesUpdateIndexes.size(); index != count; ++index)
        new (mappedMemory + index) Size { _spriteSizes[index] };
    frameCache.spriteSizesStagingAllocation.endMemoryMap();

    // Sort then erase duplicates
    frameCache.spriteSizesUpdateIndexes.sort();
    if (const auto eraseIt = std::unique(frameCache.spriteSizesUpdateIndexes.begin(), frameCache.spriteSizesUpdateIndexes.end()); eraseIt != frameCache.spriteSizesUpdateIndexes.end())
        frameCache.spriteSizesUpdateIndexes.erase(eraseIt, frameCache.spriteSizesUpdateIndexes.end());

    // Copy contiguous regions of the staging buffer into the final buffer
    for (auto it = frameCache.spriteSizesUpdateIndexes.begin(), end = frameCache.spriteSizesUpdateIndexes.end(); it != end;) {
        const auto contiguousBegin = it;
        const auto indexFrom = *it;
        SpriteIndex indexTo { it->value + 1 };
        while (++it != end && *it == indexTo)
            ++indexTo.value;
        recorder.copyBuffer(frameCache.spriteSizesStagingBuffer, frameCache.spriteSizesBuffer, GPU::BufferCopy(
            std::uint64_t(indexTo.value - indexFrom.value) * sizeof(Size),
            Core::Distance<std::uint64_t>(frameCache.spriteSizesUpdateIndexes.begin(), contiguousBegin) * sizeof(Size),
            std::uint64_t(indexFrom.value) * sizeof(Size)
        ));
    }

    // Clear events
    frameCache.spriteSizesUpdateIndexes.clear();
}

void UI::SpriteManager::updateDelayedRemoves(void) noexcept
{
    const auto end = _spriteDelayedRemoves.end();
    const auto it = std::remove_if(
        _spriteDelayedRemoves.begin(),
        end,
        [this, now = std::chrono::high_resolution_clock::now().time_since_epoch().count()](auto &delayedRemove) {
            delayedRemove.frameCount = Core::BranchlessIf(delayedRemove.frameCount, delayedRemove.frameCount - 1u, 0u);
            const auto delay = std::int64_t(double(_spriteCaches.at(delayedRemove.spriteIndex).counter.removeDelaySeconds) * 1'000'000'000.0);
            if (delayedRemove.frameCount | ((now - delayedRemove.beginTimestamp) < delay)) [[likely]]
                return false;

            // Send remove events to each frame
            for (auto &frameCache : _perFrameCache) {
                frameCache.events.push(Event {
                    .type = Event::Type::Remove,
                    .spriteIndex = delayedRemove.spriteIndex
                });
            }

            // Reset sprite name
            _spriteNames.at(delayedRemove.spriteIndex) = 0u;

            // Reset sprite cache
            _spriteCaches.at(delayedRemove.spriteIndex) = {};

            // Reset sprite size
            _spriteSizes.at(delayedRemove.spriteIndex) = {};

            // Insert sprite index into free list
            _spriteFreeList.push(delayedRemove.spriteIndex);
            return true;
        }
    );

#if KUBE_DEBUG_BUILD
    if (it != end) {
        bool first = true;
        kFInfoRaw("[UI] Delete sprites { ");
        for (auto current = it; current != end; ++current) {
            if (!first)
                kFInfoRaw(", ");
            else
                first = false;
            kFInfoRaw(current->spriteIndex);
        }
        kFInfo(" }");
    }
#endif

    if (it != end)
        _spriteDelayedRemoves.erase(it, end);
}

void UI::SpriteManager::cancelDelayedRemove(const SpriteIndex spriteIndex) noexcept
{
    // Erase delayed sprite remove
    const auto it = _spriteDelayedRemoves.find([spriteIndex](const auto &delayedRemove) { return delayedRemove.spriteIndex == spriteIndex; });
    kFAssert(it != _spriteDelayedRemoves.end(), "UI::SpriteManager::cancelDelayedRemove: Implementation error");
    _spriteDelayedRemoves.erase(it);

#if KUBE_DEBUG_BUILD
    kFInfo("[UI] Delete sprite canceled ", spriteIndex);
#endif
}