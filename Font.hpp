/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Font
 */

#pragma once

#include <Kube/Core/Utils.hpp>

namespace kF::UI
{
    class Font;
    class FontManager;


    /** @brief Unit of font size */
    using FontSize = std::uint32_t;

    /** @brief Index of a font */
    struct FontIndex
    {
        /** @brief Type of index */
        using IndexType = std::uint32_t;

        IndexType value {};

        /** @brief Implicit conversion to value */
        [[nodiscard]] inline operator IndexType(void) const noexcept { return value; }
    };

    /** @brief Describes a font */
    struct FontModel
    {
        FontSize pixelHeight {};

        /** @brief Comparison operator */
        [[nodiscard]] bool operator==(const FontModel &other) const noexcept = default;
        [[nodiscard]] bool operator!=(const FontModel &other) const noexcept = default;
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

    /** @brief Global manager constructor */
    Font(const std::string_view &path, const FontModel &fontModel) noexcept;

    /** @brief Global manager constructor with only font size parameter */
    inline Font(const std::string_view &path, const FontSize fontSize) noexcept
        : Font(path, FontModel { fontSize }) {}

    /** @brief Copy constructor */
    Font(const Font &other) noexcept;

    /** @brief Move constructor */
    Font(Font &&other) noexcept;

    /** @brief Copy assignment */
    Font &operator=(const Font &other) noexcept;

    /** @brief Move assignment */
    Font &operator=(Font &&other) noexcept;


    /** @brief Implicit conversion to FontIndex */
    [[nodiscard]] inline operator FontIndex(void) const noexcept { return _index; }

    /** @brief Get index */
    [[nodiscard]] inline FontIndex index(void) const noexcept { return _index; }


    /** @brief Get font line height */
    [[nodiscard]] Pixel lineHeight(void) const noexcept;

private:
    FontManager *_manager {};
    FontIndex _index {};
};
static_assert_fit_quarter_cacheline(kF::UI::Font);

#include "Font.ipp"