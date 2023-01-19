/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Font manager
 */

#include <cmath>

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include <stb_image_write.h>

#include <freetype/freetype.h>
#include <freetype/ftsizes.h>

#include <Kube/IO/File.hpp>

#include "App.hpp"
#include "UISystem.hpp"
#include "FontManager.hpp"

using namespace kF;

UI::FontManager::~FontManager(void) noexcept
{
    FT_Done_FreeType(_backend);
}

UI::FontManager::FontManager(void) noexcept
{
    FT_Init_FreeType(&_backend);
}

UI::Font UI::FontManager::add(const std::string_view &path, const FontModel &model) noexcept
{
    // Try to find an existing instance of the queried font
    const auto fontName = GenerateFontName(path, model);
    if (const auto it = _fontNames.find(fontName); it != _fontNames.end()) [[likely]] {
        const FontIndex fontIndex { static_cast<FontIndex::IndexType>(std::distance(_fontNames.begin(), it)) };
        ++_fontCounters.at(fontIndex);
        return Font(*this, fontIndex);
    }

    // We didn't found an instance: either get free index or create a new one
    FontIndex fontIndex {};
    if (!_fontFreeList.empty()) {
        fontIndex.value = _fontFreeList.back();
        _fontFreeList.pop();
    } else {
        fontIndex.value = _fontNames.size();
        _fontNames.push();
        _fontCaches.push();
        _fontCounters.push();
    }

    // Set font reference count and name
    _fontCounters.at(fontIndex) = 1u;
    _fontNames.at(fontIndex) = fontName;
    _fontCaches.at(fontIndex).model = model;

    // Build font cache at 'fontIndex'
    load(path, fontIndex);

    // Build font shared reference
    return Font(*this, fontIndex);
}

void UI::FontManager::load(const std::string_view &path, const FontIndex fontIndex) noexcept
{
    auto &uiSystem = UI::App::Get().uiSystem();
    auto &fontCache = _fontCaches.at(fontIndex);
    FT_Face fontFace {};
    FT_Error code {};

    // If file is a resource it is already loaded in RAM
    if (IO::File file(path); file.isResource()) {
        const auto range = file.queryResource();
        code = FT_New_Memory_Face(_backend, range.begin(), static_cast<FT_Long>(range.size()), FT_Long { 0 }, &fontFace);
    // Else we need to load file from flash storage
    } else {
        code = FT_New_Face(_backend, std::string(path).c_str(), FT_Long { 0 }, &fontFace);
    }
    kFEnsure(!code,
        "UI::FontManager::load: Couldn't load font at path '", path, " (error: ", code, ')');

    // Collect each glyph index
    FT_UInt glyphIndex {};
    std::uint32_t glyphCount {};
    auto unicode = FT_Get_First_Char(fontFace, &glyphIndex);
    for (std::uint32_t index {}; glyphIndex; ++index) {
        fontCache.glyphIndexSet.add(static_cast<wchar_t>(unicode), index);
        unicode = FT_Get_Next_Char(fontFace, unicode, &glyphIndex);
        ++glyphCount;
    }
    fontCache.glyphCount = glyphCount;

    { // Set font size
        const auto scaledPixelHeight = UI::ScalePixel(static_cast<Pixel>(fontCache.model.pixelHeight), uiSystem.windowDPI().vertical);
        auto code = FT_Set_Pixel_Sizes(fontFace, 0, fontCache.model.pixelHeight);
        if (!code)
            code = FT_Activate_Size(fontFace->size);
        kFEnsure(!code,
            "UI::FontManager::load: Couldn't set font pixel size to ", fontCache.model.pixelHeight, " (scaled: ", scaledPixelHeight, ") of font '", fontIndex,  "(error: ", code, ')');
    }

    // Collect all glyphs to render
    const auto mapSize = collectGlyphs(fontFace, fontIndex);

    // Render all glyphs
    const auto buffer = renderGlyphs(fontFace, fontIndex, mapSize);

    // Add sprite
    fontCache.sprite = uiSystem.spriteManager().add(SpriteManager::SpriteBuffer {
        .data = buffer.data(),
        .extent = GPU::Extent2D { mapSize.width, mapSize.height }
    });

    // // Save bitmap as a file
    // ::stbi_write_bmp("bitmap.bmp", mapSize.width, mapSize.height, 4, buffer.data());

#if KUBE_DEBUG_BUILD
    kFInfo("[UI] Init font ", fontIndex, ":\t Family ", fontFace->family_name, " Style ", fontFace->style_name, " Size (", fontFace->size->metrics.max_advance / 64, ", ", fontFace->size->metrics.height / 64, ')');
#endif
    // Release font face
    FT_Done_Face(fontFace);
}

UI::FontManager::MapSize UI::FontManager::collectGlyphs(const FT_Face fontFace, const FontIndex fontIndex) noexcept
{
    // Query font data
    auto &fontCache = _fontCaches.at(fontIndex);

    // Setup compute cache
    FT_UInt glyphIndex {};
    auto unicode = FT_Get_First_Char(fontFace, &glyphIndex);
    Area glyphArea { Point(), Size(0.0f, static_cast<Pixel>(fontFace->size->metrics.height) / 64.0f) };

    // Compute map size
    MapSize mapSize {
        .width = Core::NextPowerOf2(
            static_cast<std::uint32_t>(glyphArea.size.height * std::sqrt(static_cast<float>(fontCache.glyphCount)))
        ),
        .height = 0u
    };
    const Pixel pixelMapWidth = static_cast<Pixel>(mapSize.width);

    // Collect each glyph meta data
    fontCache.glyphUVs.resizeUninitialized(fontCache.glyphCount);
    for (std::uint32_t index {}; glyphIndex; ++index) {
        { // Load glyph metrics
            const auto code = FT_Load_Glyph(fontFace, glyphIndex, FT_LOAD_BITMAP_METRICS_ONLY);
            kFEnsure(!code,
                "UI::FontManager::collectGlyphs: Couldn't compute glyph metrics ", unicode, " (", glyphIndex, ") of font '", fontIndex, "' (error: ", code, ')');
        }

        // Discard empty glyphs
        const FT_Glyph_Metrics &metrics = fontFace->glyph->metrics;
        if (metrics.horiAdvance) [[likely]] {
            // Ensure we have enough horizontal space to render the glyph
            glyphArea.size.width = static_cast<Pixel>(metrics.horiAdvance) / 64.0f;
            auto nextX = glyphArea.pos.x + glyphArea.size.width + 1.0f;
            if (nextX >= pixelMapWidth) [[unlikely]] {
                nextX = glyphArea.size.width + 1.0f;
                glyphArea.pos.x = 0.0f;
                glyphArea.pos.y += glyphArea.size.height + 1.0f;
            }

            // Update UV Area
            new (&fontCache.glyphUVs.at(index)) Area { glyphArea };
            glyphArea.pos.x = nextX;
        } else [[unlikely]] {
            new (&fontCache.glyphUVs.at(index)) Area {};
        }

        unicode = FT_Get_Next_Char(fontFace, unicode, &glyphIndex);
    }

    // Update maximum baseline
    fontCache.maxUnderBaseline = static_cast<Pixel>(fontFace->size->metrics.descender) / 64.0f;

    // Update map height
    mapSize.height = static_cast<std::uint32_t>(glyphArea.pos.y + glyphArea.size.height);

    // Update instance map size
    fontCache.mapSize = Size(static_cast<Pixel>(mapSize.width), static_cast<Pixel>(mapSize.height));

    // Update instance space width
    fontCache.spaceWidth = fontCache.glyphUVs.at(fontCache.glyphIndexSet.at(' ')).size.width;

    // Update instance line height
    fontCache.lineHeight = glyphArea.size.height;

    return mapSize;
}

UI::FontManager::MapBuffer UI::FontManager::renderGlyphs(const FT_Face fontFace, const FontIndex fontIndex, const MapSize mapSize) noexcept
{
    // Query font data
    const auto &fontCache = _fontCaches.at(fontIndex);

    // Allocate map buffer
    MapBuffer buffer(mapSize.width * mapSize.height);

    // Setup compute cache
    const auto maxUnderBaseline = fontCache.maxUnderBaseline;
    FT_UInt glyphIndex {};
    auto unicode = FT_Get_First_Char(fontFace, &glyphIndex);
    const FT_Bitmap &bitmap = fontFace->glyph->bitmap;

    for (std::uint32_t index {}; glyphIndex; ++index) {
        { // Render glyph
            const auto code = FT_Load_Glyph(fontFace, glyphIndex, FT_LOAD_RENDER);
            kFEnsure(!code,
                "UI::FontManager::renderGlyphs: Couldn't render glyph ", unicode, " (", glyphIndex, ") of font '", fontIndex, "' (error: ", code, ')');
        }

        // Ensure we are rendering good metrics
        const auto &metrics = fontFace->glyph->metrics;
        if (metrics.horiAdvance) [[likely]] {
            // Compute global position
            const auto &area = fontCache.glyphUVs.at(index);
            const auto underlineY = static_cast<Pixel>(metrics.horiBearingY - metrics.height) / 64.0f;
            const auto offsetX = static_cast<Pixel>(metrics.horiBearingX) / 64.0f;
            const auto offsetY = area.size.height - static_cast<Pixel>(metrics.height) / 64.0f + maxUnderBaseline - underlineY;
            const auto globalX = static_cast<std::uint32_t>(area.pos.x + offsetX * (offsetX > 0.0f));
            const auto globalY = static_cast<std::uint32_t>(area.pos.y + offsetY * (offsetY > 0.0f));

            // Copy glyph into map buffer
            for (auto localY = 0u; localY != bitmap.rows; ++localY) {
                for (auto localX = 0u; localX != bitmap.width; ++localX) {
                    const auto localIndex = localY * bitmap.width + localX;
                    const auto globalIndex = (globalY + localY) * mapSize.width + globalX + localX;
                    const std::uint8_t r = bitmap.buffer[localIndex];
                    buffer[globalIndex] = Color { .r = 255, .g = 255, .b = 255, .a = r };
                }
            }
        }

        unicode = FT_Get_Next_Char(fontFace, unicode, &glyphIndex);
    }

    return buffer;
}

void UI::FontManager::decrementRefCount(const FontIndex fontIndex) noexcept
{
    // Check font reference counter
    if (--_fontCounters.at(fontIndex)) [[likely]]
        return;

    // Reset font name
    _fontNames.at(fontIndex) = 0u;

    // Reset font cache
    _fontCaches.at(fontIndex) = FontCache {};

    // Insert font index into free list
    _fontFreeList.push(fontIndex);
}

UI::Size UI::FontManager::computeTextMetrics(const FontIndex fontIndex, const std::string_view &text, const Pixel spacesPerTab) noexcept
{
    constexpr auto UpdateMetrics = [](UI::Size &metrics, const UI::Point pen) {
        metrics.width = std::max(metrics.width, pen.x);
        metrics.height = std::max(metrics.height, pen.y);
    };
    const auto &glyphUVs = glyphUVsAt(fontIndex);
    const auto &glyphIndexSet = glyphIndexSetAt(fontIndex);
    const auto lineHeight = lineHeightAt(fontIndex);
    const auto spaceWidth = spaceWidthAt(fontIndex);
    Size metrics {};
    Point pen {};

    for (const auto it : text) {
        if (!std::isspace(it)) {
            pen.x += glyphUVs.at(glyphIndexSet.at(it)).size.width;
        } else if (const bool isTab = it == '\t'; isTab | (it == ' ')) {
            pen.x += spaceWidth * (1.0f + spacesPerTab * isTab);
        } else {
            pen.x = {};
            pen.y += lineHeight;
        }
        UpdateMetrics(metrics, pen);
    }
    pen.y += lineHeight * !text.empty();
    UpdateMetrics(metrics, pen);
    return metrics;
}
