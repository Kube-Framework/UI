/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite manager
 */

#pragma once

#include <Kube/Core/Hash.hpp>
#include <Kube/Core/Vector.hpp>
#include <Kube/Core/FlatVector.hpp>

#include <Kube/GPU/Buffer.hpp>
#include <Kube/GPU/CommandPool.hpp>
#include <Kube/GPU/DescriptorPool.hpp>
#include <Kube/GPU/DescriptorSetLayout.hpp>
#include <Kube/GPU/Fence.hpp>
#include <Kube/GPU/Image.hpp>
#include <Kube/GPU/ImageView.hpp>
#include <Kube/GPU/MemoryAllocation.hpp>
#include <Kube/GPU/PerFrameCache.hpp>
#include <Kube/GPU/Sampler.hpp>

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
    static constexpr SpriteIndex DefaultMaxSpriteCount { 512u };

    /** @brief Sprite cache */
    struct alignas_eighth_cacheline SpriteCache
    {
        /** @brief Sprite reference count */
        struct Counter
        {
            std::uint32_t refCount {};
            float removeDelaySeconds {};
        };

        GPU::Image image {};
        GPU::MemoryAllocation memoryAllocation {};
        GPU::ImageView imageView {};
        Counter counter {};
        UI::Size size {};
    };
    static_assert_alignof_eighth_cacheline(SpriteCache);
    static_assert_sizeof(SpriteCache, Core::CacheLineEighthSize * 5);

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
    struct alignas_cacheline FrameCache
    {
        GPU::DescriptorPool descriptorPool {};
        GPU::DescriptorSetHandle descriptorSet {};
        Core::Vector<Event, UIAllocator> events {};
    };
    static_assert_fit_cacheline(FrameCache);

    /** @brief Store a sprite that must be removed with delay */
    struct alignas_quarter_cacheline SpriteDelayedRemove
    {
        SpriteIndex spriteIndex {};
        GPU::FrameIndex frameCount {}; // Minimum frame count before release
        std::int64_t beginTimestamp {}; // Minimum user time before release
    };
    static_assert_fit_quarter_cacheline(SpriteDelayedRemove);


    /** @brief Destructor */
    ~SpriteManager(void) noexcept = default;

    /** @brief Constructor */
    SpriteManager(void) noexcept;

    /** @brief SpriteManager is not copiable */
    SpriteManager(const SpriteManager &other) noexcept = delete;
    SpriteManager &operator=(const SpriteManager &other) noexcept = delete;


    /** @brief Get the maximum number of simultaneous loaded sprite */
    [[nodiscard]] std::uint32_t maxSpriteCount(void) const noexcept { return _maxSpriteCount; }


    /** @brief Add a sprite to the manager using its path if it doesn't exists
     *  @note If the sprite is already loaded this function does not duplicate its memory */
    [[nodiscard]] Sprite add(const std::string_view &path, const float removeDelaySeconds = Sprite::DefaultRemoveDelay) noexcept;


    /** @brief Add a sprite to the manager using RGBA 32bits color data
     *  @note The sprite instance is unique and cannot be copied nor queried */
    [[nodiscard]] Sprite add(const SpriteBuffer &spriteBuffer, const float removeDelaySeconds = Sprite::DefaultRemoveDelay) noexcept;

    /** @brief Add a sprite to the manager using encoded raw data
     *  @note The sprite instance is unique and cannot be copied nor queried */
    [[nodiscard]] Sprite add(const Core::IteratorRange<const std::uint8_t *> &encodedData, const float removeDelaySeconds = Sprite::DefaultRemoveDelay) noexcept;


    /** @brief Get the size of a sprite */
    [[nodiscard]] inline Size spriteSizeAt(const SpriteIndex spriteIndex) const noexcept
        { return _spriteCaches.at(spriteIndex).size; }


public: // Unsafe functions reserved for internal usage
    /** @brief Increment the reference count of a sprite */
    inline void incrementRefCount(const SpriteIndex spriteIndex) noexcept { ++_spriteCaches.at(spriteIndex).counter.refCount; }

    /** @brief Remove a sprite from the manager
     *  @note If the sprite is still used elswhere, this function does not deallocate its memory */
    void decrementRefCount(const SpriteIndex spriteIndex) noexcept;


    /** @brief Get internal DescriptorSetLayout */
    [[nodiscard]] inline GPU::DescriptorSetLayoutHandle descriptorSetLayout(void) const noexcept { return _descriptorSetLayout; }

    /** @brief Get internal DescriptorSetHandle */
    [[nodiscard]] inline GPU::DescriptorSetHandle descriptorSet(void) const noexcept { return _perFrameCache.current().descriptorSet; }


    /** @brief Prepare frame cache to draw
     *  @param Any transfer will be executed through recorder */
    void prepareFrameCache(void) noexcept;

private:
    /** @brief Base implementation of the add function */
    [[nodiscard]] SpriteIndex addImpl(const Core::HashedName spriteName, const float removeDelaySeconds) noexcept;

    /** @brief Load a sprite stored at 'spriteIndex' */
    void load(const SpriteIndex spriteIndex, const SpriteBuffer &spriteBuffer) noexcept;


    /** @brief Update all delayed sprite removes */
    void updateDelayedRemoves(void) noexcept;

    /** @brief Cancel a pending delayed remove */
    void cancelDelayedRemove(const SpriteIndex spriteIndex) noexcept;


    // Cacheline 0
    Core::Vector<Core::HashedName, UIAllocator, SpriteIndex::IndexType> _spriteNames {};
    Core::Vector<SpriteCache, UIAllocator, SpriteIndex::IndexType> _spriteCaches {};
    Core::Vector<SpriteIndex, UIAllocator, SpriteIndex::IndexType> _spriteFreeList {};
    Core::Vector<SpriteDelayedRemove, UIAllocator, SpriteIndex::IndexType> _spriteDelayedRemoves {};
    // Cacheline 1
    std::uint32_t _maxSpriteCount {};
    GPU::Sampler _sampler {};
    GPU::DescriptorSetLayout _descriptorSetLayout {};
    GPU::CommandPool _commandPool;
    GPU::CommandHandle _command {};
    GPU::Fence _fence {};
    GPU::PerFrameCache<FrameCache, UIAllocator> _perFrameCache {};
};
static_assert_fit_double_cacheline(kF::UI::SpriteManager);