/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Font manager
 */

#pragma once

#include <Kube/Core/Hash.hpp>
#include <Kube/Core/Vector.hpp>
#include <Kube/Core/FlatVector.hpp>
#include <Kube/Core/SparseSet.hpp>

#include "Base.hpp"
#include "Font.hpp"
#include "Sprite.hpp"

typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_ *FT_Face;

namespace kF::UI
{
    class FontManager;
}

/** @brief Font manager abstract the management of bindless textures */
class alignas_cacheline kF::UI::FontManager
{
public:
    /** @brief Undefined glyph */
    static constexpr auto UndefinedGlyph = std::numeric_limits<std::uint32_t>::max();

    /** @brief Initializer of the glyph index set */
    static constexpr void GlyphIndexSetInitializer(std::uint32_t *from, std::uint32_t *to) noexcept { std::fill(from, to, UndefinedGlyph); }

    /** @brief Glyph index set */
    using GlyphIndexSet = Core::SparseSet<std::uint32_t, 1024, UIAllocator, std::uint32_t, &GlyphIndexSetInitializer>;

    /** @brief Glyph metrics */
    struct GlyphMetrics
    {
        Area uv {};
        Point bearing {};
        Pixel advance {};
    };

    /** @brief Glyph metrics */
    using GlyphsMetrics = Core::Vector<GlyphMetrics, UIAllocator, std::uint32_t>;

    /** @brief Cache of a font file instance */
    struct alignas_double_cacheline FontCache
    {
        GlyphIndexSet glyphIndexSet {};
        Sprite sprite {};
        GlyphsMetrics glyphsMetrics {};
        FontModel model {};
        Size mapSize {};
        Pixel spaceWidth {};
        Pixel ascender {};
        Pixel descender {};
        Pixel lineHeight {};
    };
    static_assert_fit_double_cacheline(FontCache);

    /** @brief Buffer type of a map */
    using MapBuffer = Core::Vector<Color, UIAllocator>;

    /** @brief Query glyph metrics of an unicode character */
    [[nodiscard]] static const GlyphMetrics &GetMetricsOf(const GlyphIndexSet &glyphIndexSet, const GlyphsMetrics &glyphsMetrics, const std::uint32_t unicode) noexcept;


    /** @brief Destructor */
    ~FontManager(void) noexcept;

    /** @brief Constructor */
    FontManager(void) noexcept;

    /** @brief FontManager is not copiable */
    FontManager(const FontManager &other) noexcept = delete;
    FontManager &operator=(const FontManager &other) noexcept = delete;


    /** @brief Add a font to the manager using its path if it doesn't exists
     *  @note If the font is already loaded this function does not duplicate its memory */
    [[nodiscard]] Font add(const std::string_view &path, const FontModel &model) noexcept;


    /** @brief Query glyph metrics of an unicode character */
    [[nodiscard]] inline const GlyphMetrics &getMetricsOf(const FontIndex fontIndex, const std::uint32_t unicode) const noexcept
        { return GetMetricsOf(glyphIndexSetAt(fontIndex), glyphsMetricsAt(fontIndex), unicode); }


public: // Unsafe functions reserved for internal usage
    /** @brief Increment the reference count of a font */
    inline void incrementRefCount(const FontIndex fontIndex) noexcept { ++_fontCounters.at(fontIndex); }

    /** @brief Remove a font from the manager
     *  @note If the font is still used elswhere, this function does not deallocate its memory */
    void decrementRefCount(const FontIndex fontIndex) noexcept;


    /** @brief Get map size of a font instance */
    [[nodiscard]] inline Size mapSizeAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).mapSize; }

    /** @brief Get space width of a font instance */
    [[nodiscard]] inline Pixel spaceWidthAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).spaceWidth; }

    /** @brief Get ascender of a font instance */
    [[nodiscard]] inline Pixel ascenderAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).ascender; }

    /** @brief Get descender of a font instance */
    [[nodiscard]] inline Pixel descenderAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).descender; }

    /** @brief Get line height of a font instance */
    [[nodiscard]] inline Pixel lineHeightAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).lineHeight; }

    /** @brief Get glyph index set of a texture */
    [[nodiscard]] inline const GlyphIndexSet &glyphIndexSetAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).glyphIndexSet; }

    /** @brief Get glyph metrics of a texture instance */
    [[nodiscard]] inline const GlyphsMetrics &glyphsMetricsAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).glyphsMetrics; }

    /** @brief Get sprite of a texture instance index */
    [[nodiscard]] inline SpriteIndex spriteAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).sprite.index(); }


    /** @brief Compute text metrics using a given font */
    [[nodiscard]] UI::Size computeTextMetrics(const FontIndex fontIndex, const std::string_view &text, const Pixel spacesPerTab = DefaultSpacesPerTab) const noexcept;

private:
    /** @brief Load a font from 'path' that is stored at 'fontIndex' */
    void load(const std::string_view &path, const FontIndex fontIndex) noexcept;


    /** @brief Generate a unique font name from a path and a model */
    [[nodiscard]] inline Core::HashedName GenerateFontName(const std::string_view &path, const FontModel &model) noexcept
        { return Core::Hash(path) + model.pixelHeight; }


    // Cacheline 0
    Core::Vector<Core::HashedName, UIAllocator, FontIndex::IndexType> _fontNames {};
    Core::Vector<FontCache, UIAllocator, FontIndex::IndexType> _fontCaches {};
    Core::Vector<std::uint32_t, UIAllocator, FontIndex::IndexType> _fontCounters {};
    Core::FlatVector<FontIndex, UIAllocator, FontIndex::IndexType> _fontFreeList {};
    FT_Library _backend {};
};
static_assert_fit_cacheline(kF::UI::FontManager);
