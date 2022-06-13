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

    /** @brief Describes a font */
    struct FontModel
    {
        FontSize pixelHeight {};

        /** @brief Comparison operator */
        [[nodiscard]] bool operator==(const FontModel &other) const noexcept = default;
        [[nodiscard]] bool operator!=(const FontModel &other) const noexcept = default;
    };
}

/** @brief Font manager abstract the management of bindless textures */
class alignas_cacheline kF::UI::FontManager
{
public:
    /** @brief Glyph index set */
    using GlyphIndexSet = Core::SparseSet<std::uint32_t, 1024, ResourceAllocator, wchar_t>;

    /** @brief Glyph uv coordinates */
    using GlyphUVs = Core::Vector<Area, ResourceAllocator, std::uint32_t>;

    /** @brief Cache of a font file instance */
    struct alignas_double_cacheline FontCache
    {
        GlyphIndexSet glyphIndexSet {};
        Sprite sprite {};
        GlyphUVs glyphUVs {};
        std::uint32_t glyphCount {};
        FontModel model {};
        Size mapSize {};
        Pixel spaceWidth {};
        Pixel lineHeight {};
        int maxUnderBaseline {};
    };
    static_assert_fit_double_cacheline(FontCache);

    /** @brief Contains the size of a map */
    struct MapSize
    {
        std::uint32_t width {};
        std::uint32_t height {};
    };

    /** @brief Buffer type of a map */
    using MapBuffer = Core::Vector<Color, ResourceAllocator>;


    /** @brief Destructor */
    ~FontManager(void) noexcept;

    /** @brief Constructor */
    FontManager(void) noexcept;


    /** @brief Add a font to the manager using its path if it doesn't exists
     *  @note If the font is already loaded this function does not duplicate its memory */
    [[nodiscard]] Font add(const std::string_view &path, const FontModel &model) noexcept;


public: // Unsafe functions reserved for internal usage
    /** @brief Remove a font from the manager
     *  @note If the font is still used elswhere, this function does not deallocate its memory */
    void removeUnsafe(const FontIndex fontIndex) noexcept;


    /** @brief Get map size of a font instance */
    [[nodiscard]] inline Size mapSizeAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).mapSize; }

    /** @brief Get space width of a font instance */
    [[nodiscard]] inline Pixel spaceWidthAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).spaceWidth; }

    /** @brief Get line height of a font instance */
    [[nodiscard]] inline Pixel lineHeightAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).lineHeight; }

    /** @brief Get glyph index set of a texture */
    [[nodiscard]] inline const GlyphIndexSet &glyphIndexSetAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).glyphIndexSet; }

    /** @brief Get glyph uv coordinates of a texture instance */
    [[nodiscard]] inline const GlyphUVs &glyphUVsAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).glyphUVs; }

    /** @brief Get sprite of a texture instance index */
    [[nodiscard]] inline SpriteIndex spriteAt(const FontIndex fontIndex) const noexcept
        { return _fontCaches.at(fontIndex).sprite.index(); }

private:
    /** @brief Load a font from 'path' that is stored at 'fontIndex' */
    void load(const std::string_view &path, const FontIndex fontIndex) noexcept;

    /** @brief Collect glyphs data of a font instance
     *  @note Font instance UVs are not normalized
     *  @return Returns the map height */
    [[nodiscard]] MapSize collectGlyphs(const FT_Face fontFace, const FontIndex fontIndex) noexcept;

    /** @brief Render glyphs of a font instance into a map
     *  @note Font instance UVs must not be normalized
     *  @return Map pixel buffer */
    [[nodiscard]] MapBuffer renderGlyphs(const FT_Face fontFace, const FontIndex fontIndex, const MapSize mapSize) noexcept;


    /** @brief Generate a unique font name from a path and a model */
    [[nodiscard]] inline Core::HashedName GenerateFontName(const std::string_view &path, const FontModel &model) noexcept
        { return Core::Hash(path) + model.pixelHeight; }


    // Cacheline 0
    Core::Vector<Core::HashedName, ResourceAllocator, FontIndex::IndexType> _fontNames {};
    Core::Vector<FontCache, ResourceAllocator, FontIndex::IndexType> _fontCaches {};
    Core::Vector<std::uint32_t, ResourceAllocator, FontIndex::IndexType> _fontCounters {};
    Core::FlatVector<FontIndex, ResourceAllocator, FontIndex::IndexType> _fontFreeList {};
    FT_Library _backend {};
};
static_assert_fit_cacheline(kF::UI::FontManager);