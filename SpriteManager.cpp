/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite manager
 */

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
            { GPU::DescriptorBindingFlags::PartiallyBound }
        )),
        _descriptorPool(GPU::DescriptorPool::Make(
            GPU::DescriptorPoolCreateFlags::UpdateAfterBind,
            1,
            {
                GPU::DescriptorPoolSize(GPU::DescriptorType::CombinedImageSampler, _maxSpriteCount)
            }
        )),
        _descriptorSet(_descriptorPool.allocate(_descriptorSetLayout)),
        _commandPool(GPU::QueueType::Transfer, GPU::CommandPoolCreateFlags::Transient),
        _command(_commandPool.add(GPU::CommandLevel::Primary))
{
}

UI::Sprite UI::SpriteManager::add(const std::string_view &path) noexcept
{
    using namespace GPU;

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
    const auto spriteIndex = addImpl(path);

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
    const auto spriteIndex = addImpl(std::string_view {});

    // Build sprite cache at 'spriteIndex'
    load(spriteIndex, spriteBuffer);

    // Build sprite
    return Sprite(*this, spriteIndex);
}

UI::SpriteIndex UI::SpriteManager::addImpl(const std::string_view &path) noexcept
{
    // Try to find an existing instance of the queried sprite
    Core::HashedName spriteName {};
    if (!path.empty()) [[likely]] {
        spriteName = Core::Hash(path);
        if (const auto it = _spriteNames.find(spriteName); it != _spriteNames.end()) [[likely]] {
            const auto spriteIndex = static_cast<SpriteIndex>(std::distance(it, _spriteNames.end()));
            ++_spriteCounters.at(spriteIndex);
            return Sprite(*this, spriteIndex);
        }
    }

    // We didn't found an instance: either get free index or create a new one
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

    // Update descriptor set to reference loaded sprite
    const DescriptorImageInfo imageInfo(
        _sampler,
        spriteCache.imageView,
        ImageLayout::ShaderReadOnlyOptimal
    );
    DescriptorSetUpdate::UpdateWrite({
        DescriptorSetWriteModel(_descriptorSet, 0, spriteIndex, DescriptorType::CombinedImageSampler, &imageInfo, &imageInfo + 1)
    });

    // Wait until transfer completed
    _fence.wait();
}

void UI::SpriteManager::removeUnsafe(const SpriteIndex spriteIndex) noexcept
{
    // Check sprite reference counter
    if (--_spriteCounters.at(spriteIndex)) [[likely]]
        return;

    // Reset sprite name
    _spriteNames.at(spriteIndex) = 0u;

    // Reset sprite cache
    _spriteCaches.at(spriteIndex) = SpriteCache {};

    // Insert sprite index into free list
    _spriteFreeList.push(spriteIndex);
}

UI::Size UI::SpriteManager::spriteSizeAt(const SpriteIndex spriteIndex) const noexcept
{
    return _spriteCaches.at(spriteIndex).size;
}