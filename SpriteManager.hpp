/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite manager
 */

#pragma once

#include <Kube/Core/Hash.hpp>
#include <Kube/Core/Vector.hpp>

#include <Kube/GPU/MemoryAllocation.hpp>
#include <Kube/GPU/DescriptorSetLayout.hpp>
#include <Kube/GPU/DescriptorPool.hpp>
#include <Kube/GPU/CommandPool.hpp>
#include <Kube/GPU/Fence.hpp>
#include <Kube/GPU/Sampler.hpp>
#include <Kube/GPU/Image.hpp>
#include <Kube/GPU/ImageView.hpp>

#include "Base.hpp"
#include "Sprite.hpp"

namespace kF::UI
{
    class SpriteManager;
}

/** @brief Sprite manager abstract the management of bindless textures */
class alignas_double_cacheline kF::UI::SpriteManager : public GPU::GPUObject
{
public:
    /** @brief Default sprite index */
    static constexpr SpriteIndex DefaultSprite { 0u };

    /** @brief Default sprite index */
    static constexpr SpriteIndex DefaultMaxSpriteCount { 4096u };


    /** @brief Sprite cache */
    struct SpriteCache
    {
        GPU::Image image {};
        GPU::MemoryAllocation memoryAllocation {};
        GPU::ImageView imageView {};
        Size size {};
    };

    /** @brief Staging buffer */
    struct SpriteBuffer
    {
        const Color *data {};
        GPU::Extent2D extent {};
    };


    /** @brief Destructor */
    ~SpriteManager(void) noexcept = default;

    /** @brief Constructor */
    SpriteManager(const std::uint32_t maxSpriteCount = DefaultMaxSpriteCount) noexcept;


    /** @brief Get the maximum number of simultaneous loaded sprite */
    [[nodiscard]] std::uint32_t maxSpriteCount(void) const noexcept { return _maxSpriteCount; }


    /** @brief Add a sprite to the manager using its path if it doesn't exists
     *  @note If the sprite is already loaded this function does not duplicate its memory */
    [[nodiscard]] Sprite add(const std::string_view &path) noexcept;


    /** @brief Add a sprite to the manager using a fake path and RGBA 32bits color data
     *  @note The sprite instance is unique and cannot be copied nor queried */
    [[nodiscard]] Sprite add(const SpriteBuffer &spriteBuffer) noexcept;


    /** @brief Get the size of a sprite */
    [[nodiscard]] Size spriteSizeAt(const SpriteIndex spriteIndex) const noexcept;


public: // Unsafe functions reserved for internal usage
    /** @brief Remove a sprite from the manager
     *  @note If the sprite is still used elswhere, this function does not deallocate its memory */
    void removeUnsafe(const SpriteIndex spriteIndex) noexcept;


    /** @brief Get internal DescriptorSetLayout */
    [[nodiscard]] inline GPU::DescriptorSetLayoutHandle descriptorSetLayout(void) const noexcept { return _descriptorSetLayout; }

    /** @brief Get internal DescriptorSetHandle */
    [[nodiscard]] inline GPU::DescriptorSetHandle descriptorSet(void) const noexcept { return _descriptorSet; }

private:
    /** @brief Base implementation of the add function */
    [[nodiscard]] SpriteIndex addImpl(const std::string_view &path) noexcept;

    /** @brief Load a sprite stored at 'spriteIndex' */
    void load(const SpriteIndex spriteIndex, const SpriteBuffer &spriteBuffer) noexcept;


    // Cacheline 0
    Core::Vector<Core::HashedName, ResourceAllocator, SpriteIndex::IndexType> _spriteNames {};
    Core::Vector<SpriteCache, ResourceAllocator, SpriteIndex::IndexType> _spriteCaches {};
    Core::Vector<std::uint32_t, ResourceAllocator, SpriteIndex::IndexType> _spriteCounters {};
    Core::Vector<SpriteIndex, ResourceAllocator, SpriteIndex::IndexType> _spriteFreeList {};
    // Cacheline 1
    std::uint32_t _maxSpriteCount {};
    GPU::Sampler _sampler;
    GPU::Sampler _sampler2;
    GPU::DescriptorSetLayout _descriptorSetLayout;
    GPU::DescriptorPool _descriptorPool;
    GPU::DescriptorSetHandle _descriptorSet;
    GPU::CommandPool _commandPool;
    GPU::CommandHandle _command;
    GPU::Fence _fence {};
};
// static_assert_fit_double_cacheline(kF::UI::SpriteManager);