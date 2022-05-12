/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Font
 */

#pragma once

#include <Kube/Core/Utils.hpp>

#include "Sprite.hpp"

namespace kF::UI
{
    class Font;
    class FontInstance;
    class FontManager;


    /** @brief Index of a font */
    struct FontIndex
    {
        /** @brief Type of index */
        using IndexType = std::uint16_t;

        IndexType value {};

        /** @brief Implicit conversion to value */
        [[nodiscard]] inline operator IndexType(void) const noexcept { return value; }
    };

    /** @brief Index of a font instance */
    struct FontInstanceIndex
    {
        /** @brief Type of index */
        using IndexType = std::uint16_t;

        IndexType value {};

        /** @brief Implicit conversion to value */
        [[nodiscard]] inline operator IndexType(void) const noexcept { return value; }
    };
}

/** @brief Font class manager the lifecycle of a Font */
class alignas_quarter_cacheline kF::UI::Font
{
public:
    /** @brief Destructor */
    ~Font(void) noexcept;

    /** @brief Default constructor */
    Font(void) noexcept = default;

    /** @brief Constructor */
    inline Font(FontManager &manager, const FontIndex index) noexcept
        : _manager(&manager), _index(index) {}

    /** @brief Move constructor */
    Font(Font &&other) noexcept;

    /** @brief Move assignment */
    Font &operator=(Font &&other) noexcept;


    /** @brief Implicit conversion to FontIndex */
    [[nodiscard]] inline operator FontIndex(void) const noexcept { return _index; }

    /** @brief Get index */
    [[nodiscard]] inline FontIndex index(void) const noexcept { return _index; }


private:
    FontManager *_manager {};
    FontIndex _index {};
};
static_assert_fit_quarter_cacheline(kF::UI::Font);

/** @brief FontInstance class manager the lifecycle of a FontInstance */
class alignas_quarter_cacheline kF::UI::FontInstance
{
public:
    /** @brief Destructor */
    ~FontInstance(void) noexcept;

    /** @brief Default constructor */
    FontInstance(void) noexcept = default;

    /** @brief Constructor  */
    inline FontInstance(FontManager &manager, const FontIndex parentIndex, const FontInstanceIndex index, const SpriteIndex spriteIndex) noexcept
        : _manager(&manager), _parentIndex(parentIndex), _index(index), _spriteIndex(spriteIndex) {}

    /** @brief Move constructor */
    FontInstance(FontInstance &&other) noexcept;

    /** @brief Move assignment */
    FontInstance &operator=(FontInstance &&other) noexcept;


    /** @brief Implicit conversion to FontInstanceIndex */
    [[nodiscard]] inline operator FontInstanceIndex(void) const noexcept { return _index; }

    /** @brief Implicit conversion to SpriteIndex */
    [[nodiscard]] inline operator SpriteIndex(void) const noexcept { return _spriteIndex; }

    /** @brief Get parent index */
    [[nodiscard]] inline FontIndex parentIndex(void) const noexcept { return _parentIndex; }

    /** @brief Get index */
    [[nodiscard]] inline FontInstanceIndex index(void) const noexcept { return _index; }

    /** @brief Get sprite index */
    [[nodiscard]] inline SpriteIndex spriteIndex(void) const noexcept { return _spriteIndex; }


private:
    FontManager *_manager {};
    FontIndex _parentIndex {};
    FontInstanceIndex _index {};
    SpriteIndex _spriteIndex {};
};
static_assert_fit_quarter_cacheline(kF::UI::FontInstance);

#include "Font.ipp"