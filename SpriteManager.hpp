/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite manager
 */

#pragma once

#include <Kube/Core/Hash.hpp>
#include <Kube/Core/Vector.hpp>
#include <Kube/Core/FlatVector.hpp>

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
    struct alignas_half_cacheline SpriteCache
    {
        GPU::Image image {};
        GPU::MemoryAllocation memoryAllocation {};
        GPU::ImageView imageView {};
        Size size {};
    };
    static_assert_fit_half_cacheline(SpriteCache);

    /** @brief Staging buffer */
    struct alignas_quarter_cacheline SpriteBuffer
    {
        const Color *data {};
        GPU::Extent2D extent {};
    };
    static_assert_fit_quarter_cacheline(SpriteBuffer);

    /** @brief Sprite event */
    struct alignas_eighth_cacheline Event
    {
        /** @brief Type of event */
        enum class Type : std::uint32_t
        {
            Add,
            Remove
        };

        Type type {};
        SpriteIndex spriteIndex {};
    };
    static_assert_fit_eighth_cacheline(Event);

    /** @brief Sprite cache */
    struct alignas_half_cacheline FrameCache
    {
        GPU::DescriptorPool descriptorPool;
        GPU::DescriptorSetHandle descriptorSet;
        Core::Vector<Event, ResourceAllocator> events {};
    };
    static_assert_fit_half_cacheline(FrameCache);

    struct alignas_eighth_cacheline SpriteDelayedRemove
    {
        SpriteIndex spriteIndex {};
        std::uint32_t elapsedFrames {};
    };
    static_assert_fit_eighth_cacheline(SpriteDelayedRemove);


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
    [[nodiscard]] inline Size spriteSizeAt(const SpriteIndex spriteIndex) const noexcept
        { return _spriteCaches.at(spriteIndex).size; }


public: // Unsafe functions reserved for internal usage
    /** @brief Increment the reference count of a sprite */
    inline void incrementRefCount(const SpriteIndex spriteIndex) noexcept { ++_spriteCounters.at(spriteIndex); }

    /** @brief Remove a sprite from the manager
     *  @note If the sprite is still used elswhere, this function does not deallocate its memory */
    void decrementRefCount(const SpriteIndex spriteIndex) noexcept;


    /** @brief Get internal DescriptorSetLayout */
    [[nodiscard]] inline GPU::DescriptorSetLayoutHandle descriptorSetLayout(void) const noexcept { return _descriptorSetLayout; }

    /** @brief Get internal DescriptorSetHandle */
    [[nodiscard]] inline GPU::DescriptorSetHandle descriptorSet(void) const noexcept { return _perFrameCache.current().descriptorSet; }


    /** @brief Prepare frame cache to draw */
    void prepareFrameCache(void) noexcept;

private:
    /** @brief Base implementation of the add function */
    [[nodiscard]] SpriteIndex addImpl(const Core::HashedName spriteName) noexcept;

    /** @brief Load a sprite stored at 'spriteIndex' */
    void load(const SpriteIndex spriteIndex, const SpriteBuffer &spriteBuffer) noexcept;


    /** @brief Update all delayed sprite removes */
    void updateDelayedRemoves(void) noexcept;

    /** @brief Cancel a pending delayed remove */
    void cancelDelayedRemove(const SpriteIndex spriteIndex) noexcept;

    // Cacheline 0
    Core::Vector<Core::HashedName, ResourceAllocator, SpriteIndex::IndexType> _spriteNames {};
    Core::FlatVector<SpriteCache, ResourceAllocator, SpriteIndex::IndexType> _spriteCaches {};
    Core::FlatVector<std::uint32_t, ResourceAllocator, SpriteIndex::IndexType> _spriteCounters {};
    Core::Vector<SpriteIndex, ResourceAllocator, SpriteIndex::IndexType> _spriteFreeList {};
    Core::Vector<SpriteDelayedRemove, ResourceAllocator, SpriteIndex::IndexType> _spriteDelayedRemoves {};
    // Cacheline 1
    std::uint32_t _maxSpriteCount {};
    GPU::Sampler _sampler;
    GPU::DescriptorSetLayout _descriptorSetLayout;
    GPU::CommandPool _commandPool;
    GPU::CommandHandle _command;
    GPU::Fence _fence {};
    GPU::PerFrameCache<FrameCache, ResourceAllocator> _perFrameCache {};
};
static_assert_fit_double_cacheline(kF::UI::SpriteManager);