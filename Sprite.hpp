/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite
 */

#pragma once

#include <string_view>

#include <Kube/Core/Utils.hpp>

namespace kF::UI
{
    class Sprite;
    class SpriteManager;

    /** @brief Index of a sprite */
    struct SpriteIndex
    {
        /** @brief Type of index */
        using IndexType = std::uint32_t;

        IndexType value {};

        /** @brief Implicit conversion to value */
        [[nodiscard]] inline operator IndexType(void) const noexcept { return value; }
    };

    /** @brief Null sprite */
    constexpr SpriteIndex NullSpriteIndex { .value = ~static_cast<SpriteIndex::IndexType>(0) };
}

/** @brief Sprite class manager the lifecycle of a SpriteManager instance */
class alignas_quarter_cacheline kF::UI::Sprite
{
public:
    /** @brief Destructor */
    ~Sprite(void) noexcept;

    /** @brief Default constructor */
    Sprite(void) noexcept = default;

    /** @brief Constructor */
    inline Sprite(SpriteManager &manager, const SpriteIndex index) noexcept
        : _manager(&manager), _index(index) {}

    /** @brief Copy constructor */
    Sprite(const Sprite &other) noexcept;

    /** @brief Move constructor */
    Sprite(Sprite &&other) noexcept;

    /** @brief Copy assignment */
    Sprite &operator=(const Sprite &other) noexcept;

    /** @brief Move assignment */
    Sprite &operator=(Sprite &&other) noexcept;

    /** @brief Check if the sprite is valid */
    [[nodiscard]] inline bool isValid(void) const noexcept { return _index != NullSpriteIndex; }

    /** @brief Implicit conversion to SpriteIndex */
    [[nodiscard]] inline operator SpriteIndex(void) const noexcept { return _index; }

    /** @brief Get index */
    [[nodiscard]] inline SpriteIndex index(void) const noexcept { return _index; }


private:
    SpriteManager *_manager {};
    SpriteIndex _index { NullSpriteIndex };
};
static_assert_fit_quarter_cacheline(kF::UI::Sprite);

#include "Sprite.ipp"