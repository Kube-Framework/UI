/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite manager
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wconversion"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma GCC diagnostic pop

#include <Kube/Core/Assert.hpp>
#include <Kube/GPU/GPU.hpp>
#include <Kube/GPU/Buffer.hpp>
#include <Kube/GPU/DescriptorSetUpdate.hpp>
#include <Kube/IO/File.hpp>

#include "SpriteManager.hpp"

using namespace kF;

UI::SpriteManager::SpriteManager(const std::uint32_t maxSpriteCount) noexcept
    :   _maxSpriteCount([this, maxSpriteCount] {
            const auto max = std::min(
                maxSpriteCount,
                parent().physicalDevice().limits().maxDescriptorSetSampledImages
            );
            kFEnsure(max != 0u, "UI::SpriteManager: Maximum sprite count cannot be 0");
            return max;
        }()),
        _sampler(GPU::SamplerModel(
            GPU::SamplerCreateFlags::None,
            GPU::Filter::Linear,
            GPU::Filter::Linear,
            GPU::SamplerMipmapMode::Nearest,
            GPU::SamplerAddressMode::ClampToBorder,
            GPU::SamplerAddressMode::ClampToBorder,
            GPU::SamplerAddressMode::ClampToBorder,
            false, 0.0f, // Anisotropy
            false, GPU::CompareOp::Never, // Compare
            0.0f, 0.0f, 0.0f, // Lod
            GPU::BorderColor::FloatTransparentBlack, // Border
            false // Unormalized
        )),
        _descriptorSetLayout(GPU::DescriptorSetLayout::Make(
            GPU::DescriptorSetLayoutCreateFlags::UpdateAfterBindPool,
            {
                GPU::DescriptorSetLayoutBinding(
                    0u,
                    GPU::DescriptorType::CombinedImageSampler, _maxSpriteCount,
                    Core::MakeFlags(GPU::ShaderStageFlags::Compute, GPU::ShaderStageFlags::Vertex, GPU::ShaderStageFlags::Fragment)
                )
            },
            {
                Core::MakeFlags(
                    GPU::DescriptorBindingFlags::UpdateAfterBind,
                    GPU::DescriptorBindingFlags::UpdateUnusedWhilePending,
                    GPU::DescriptorBindingFlags::PartiallyBound
                )
            }
        )),
                _commandPool(GPU::QueueType::Transfer,
        GPU::CommandPoolCreateFlags::Transient),
        _command(_commandPool.add(GPU::CommandLevel::Primary)),
        _perFrameCache(parent().frameCount(), [this] {
            FrameCache frameCache {
                .descriptorPool = GPU::DescriptorPool::Make(
                    GPU::DescriptorPoolCreateFlags::UpdateAfterBind,
                    1,
                    {
                        GPU::DescriptorPoolSize(GPU::DescriptorType::CombinedImageSampler, _maxSpriteCount)
                    }
                ),
                .descriptorSet = frameCache.descriptorPool.allocate(_descriptorSetLayout)
            };
            return frameCache;
        })
{
    parent().frameAcquiredDispatcher().add([this](const GPU::FrameIndex frameIndex) noexcept {
        _perFrameCache.setCurrentFrame(frameIndex);
    });

    // Add default sprite
    const Color defaultBufferData { 255, 80, 255, 255 };
    const auto defaultSpriteIndex = addImpl(Core::HashedName {});
    kFEnsure(defaultSpriteIndex == DefaultSprite, "UI::SpriteManager: Implementation error");
    load(defaultSpriteIndex, SpriteBuffer {
        .data = &defaultBufferData,
        .extent = GPU::Extent2D { 1, 1 }
    });
}

UI::Sprite UI::SpriteManager::add(const std::string_view &path) noexcept
{
    using namespace GPU;

    kFEnsure(!path.empty(), "UI::SpriteManager::add: Empty path");

    // Try to find an existing instance of the queried sprite
    const auto spriteName = Core::Hash(path);
    if (const auto it = _spriteNames.find(spriteName); it != _spriteNames.end()) [[likely]] {
        const auto spriteIndex = SpriteIndex { _spriteNames.indexOf(it) };
        if (++_spriteCounters.at(spriteIndex) == 1) [[unlikely]]
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
    kFEnsure(data,
        "UI::SpriteManager::load: Couldn't load sprite at path '", path, '\'');

    // Reserve sprite index
    kFEnsure(!path.empty(), "UI::SpriteManager::add: Path cannot be empty");
    const auto spriteIndex = addImpl(spriteName);

    // Build sprite cache at 'spriteIndex'
    load(spriteIndex, SpriteBuffer {
        .data = data,
        .extent = { static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y) }
    });

    // Release decoded image
    ::stbi_image_free(data);

    // Build sprite
    return Sprite(*this, spriteIndex);
}

UI::Sprite UI::SpriteManager::add(const SpriteBuffer &spriteBuffer) noexcept
{
    // Reserve sprite index
    const auto spriteIndex = addImpl(Core::HashedName {});

    // Build sprite cache at 'spriteIndex'
    load(spriteIndex, spriteBuffer);

    // Build sprite
    return Sprite(*this, spriteIndex);
}

UI::SpriteIndex UI::SpriteManager::addImpl(const Core::HashedName spriteName) noexcept
{
    SpriteIndex spriteIndex {};
    if (!_spriteFreeList.empty()) {
        spriteIndex.value = _spriteFreeList.back();
        _spriteFreeList.pop();
    } else {
        spriteIndex.value = _spriteNames.size();
        _spriteNames.push();
        _spriteCaches.push();
        _spriteCounters.push();
    }

    // Set sprite reference count and name
    _spriteCounters.at(spriteIndex) = 1u;
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

    // Create sprite cache
    auto &spriteCache = _spriteCaches.at(spriteIndex);
    spriteCache.size = Size(static_cast<Pixel>(spriteBuffer.extent.width), static_cast<Pixel>(spriteBuffer.extent.height));
    spriteCache.image = Image::MakeSingleLayer2D(
        spriteBuffer.extent,
        Format::R8G8B8A8_SRGB,
        Core::MakeFlags(ImageUsageFlags::TransferDst, ImageUsageFlags::Sampled),
        ImageTiling::TilingOptimal
    );
    spriteCache.memoryAllocation = MemoryAllocation::MakeLocal(spriteCache.image);
    spriteCache.imageView = ImageView(ImageViewModel(
        ImageViewCreateFlags::None,
        spriteCache.image,
        ImageViewType::Image2D,
        Format::R8G8B8A8_SRGB,
        ComponentMapping(),
        ImageSubresourceRange(ImageAspectFlags::Color)
    ));

    // Record transfer command
    _commandPool.reset();
    _commandPool.record(_command, CommandBufferUsageFlags::OneTimeSubmit,
        [this, &spriteBuffer, &stagingBuffer, &spriteCache](const CommandRecorder &recorder) {
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
    if (--_spriteCounters.at(spriteIndex)) [[likely]]
        return;

    // Add remove events to frame caches
    for (auto &frameCache : _perFrameCache) {
        const auto it = frameCache.events.find([spriteIndex](const auto &event) { return event.spriteIndex == spriteIndex; });
        if (it != frameCache.events.end()) [[unlikely]]
            frameCache.events.erase(it);
        frameCache.events.push(Event {
            .type = Event::Type::Remove,
            .spriteIndex = spriteIndex
        });
    }

    _spriteDelayedRemoves.push(SpriteDelayedRemove {
        .spriteIndex = spriteIndex
    });
}

void UI::SpriteManager::prepareFrameCache(void) noexcept
{
    updateDelayedRemoves();

    auto &currentCache = _perFrameCache.current();
    const auto eventCount = currentCache.events.size();

    if (!eventCount) [[likely]]
        return;

    // Prepare image infos
    Core::SmallVector<GPU::DescriptorImageInfo, 8, ResourceAllocator> imageInfos(
        eventCount,
        [this, &currentCache](const auto index) {
            const auto &event = currentCache.events.at(index);
            const auto targetSprite = event.type == Event::Type::Add ? event.spriteIndex : DefaultSprite;
            kFEnsure(_spriteCaches.at(targetSprite).imageView != GPU::NullHandle, "ERRRROOOR");
            return GPU::DescriptorImageInfo(
                _sampler,
                _spriteCaches.at(targetSprite).imageView,
                GPU::ImageLayout::ShaderReadOnlyOptimal
            );
        }
    );

    // Prepare descriptor set write models
    Core::SmallVector<GPU::DescriptorSetWriteModel, 8, ResourceAllocator> models(
        eventCount,
        [this, &currentCache, &imageInfos](const auto index) {
            return GPU::DescriptorSetWriteModel(
                currentCache.descriptorSet,
                0,
                currentCache.events.at(index).spriteIndex,
                GPU::DescriptorType::CombinedImageSampler,
                imageInfos.begin() + index, imageInfos.begin() + index + 1
            );
        }
    );

    // Clear events
    currentCache.events.clear();

    // Write descriptors
    GPU::DescriptorSetUpdate::UpdateWrite(models.begin(), models.end());
}

void UI::SpriteManager::updateDelayedRemoves(void) noexcept
{
    const auto end = _spriteDelayedRemoves.end();
    const auto it = std::remove_if(_spriteDelayedRemoves.begin(), end,
        [this, frameCount = _perFrameCache.count()](auto &delayedRemove) {
            if (++delayedRemove.elapsedFrames < frameCount) [[likely]]
                return false;
            if (!_spriteCounters.at(delayedRemove.spriteIndex)) [[likely]] {
                // Reset sprite name
                _spriteNames.at(delayedRemove.spriteIndex) = 0u;

                // Reset sprite cache
                _spriteCaches.at(delayedRemove.spriteIndex) = SpriteCache {};

                // Insert sprite index into free list
                _spriteFreeList.push(delayedRemove.spriteIndex);
            }
            return true;
        }
    );

    if (it != end)
        _spriteDelayedRemoves.erase(it, end);
}

void UI::SpriteManager::cancelDelayedRemove(const SpriteIndex spriteIndex) noexcept
{
    const auto it = _spriteDelayedRemoves.find([spriteIndex](const auto &delayedRemove) { return delayedRemove.spriteIndex == spriteIndex; });

    kFAssert(it != _spriteDelayedRemoves.end(), "UI::SpriteManager::cancelDelayedRemove: Implementation error");
    _spriteDelayedRemoves.erase(it);

    for (auto &frameCache : _perFrameCache) {
        const auto end = frameCache.events.end();
        const auto it = frameCache.events.find([spriteIndex](const auto &event) { return event.spriteIndex == spriteIndex; });
        if (it != end) {
            frameCache.events.erase(it, end);
        } else {
            frameCache.events.push(Event {
                .type = Event::Type::Add,
                .spriteIndex = spriteIndex
            });
        }
    }
}