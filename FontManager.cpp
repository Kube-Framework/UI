/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Font manager
 */

#include <cmath>

// #include <Kube/Core/Platform.hpp>
// #if KUBE_COMPILER_GCC | KUBE_COMPILER_CLANG
// # pragma GCC diagnostic push
// # pragma GCC diagnostic ignored "-Wold-style-cast"
// # pragma GCC diagnostic ignored "-Wcast-qual"
// # pragma GCC diagnostic ignored "-Wconversion"
// #endif
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include <stb_image_write.h>
// #if KUBE_COMPILER_GCC | KUBE_COMPILER_CLANG
// # pragma GCC diagnostic pop
// #endif

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
    auto &uiSystem = App::Get().uiSystem();
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

    { // Set font size
        const auto scaledPixelHeight = ScalePixel(static_cast<Pixel>(fontCache.model.pixelHeight), uiSystem.windowDPI().vertical);
        code = FT_Set_Pixel_Sizes(fontFace, 0, fontCache.model.pixelHeight);
        if (!code)
            code = FT_Activate_Size(fontFace->size);
        kFEnsure(!code,
            "UI::FontManager::load: Couldn't set font pixel size to ", fontCache.model.pixelHeight, " (scaled: ", scaledPixelHeight, ") of font '", fontIndex,  "(error: ", code, ')');
    }

    // Allocate glyph uvs
    fontCache.glyphsMetrics.resize(std::uint32_t(fontFace->num_glyphs));

    // Update instance line height
    fontCache.lineHeight = Pixel((fontFace->size->metrics.ascender - fontFace->size->metrics.descender) / 64);

    { // Collect metrics of each glyph and determine map size
        Size mapSize {
            .width = Pixel(Core::NextPowerOf2(
                std::uint32_t(fontCache.lineHeight * 0.5f * std::sqrt(Pixel(fontFace->num_glyphs)))
            ))
        };
        UI::Area glyphArea { .pos = { 1.0f, 1.0f } };
        FT_UInt glyphIndex {};
        auto unicode = FT_Get_First_Char(fontFace, &glyphIndex);
        for (std::uint32_t index {}; glyphIndex; ++index) {
            // Register glyph into sparse set
            fontCache.glyphIndexSet.add(static_cast<wchar_t>(unicode), index);

            // Load glyph metrics
            code = FT_Load_Glyph(fontFace, glyphIndex, FT_LOAD_BITMAP_METRICS_ONLY);
            kFEnsure(!code, "UI::FontManager::collectGlyphs: Couldn't compute glyph metrics ", unicode,
                " (", glyphIndex, ") of font '", fontIndex, "' (error: ", code, ')');
            const auto &metrics = fontFace->glyph->metrics;
            glyphArea.size = Size { Pixel(metrics.width / 64), Pixel(metrics.height / 64) };

            auto &glyphMetrics = fontCache.glyphsMetrics.at(index);
            if (bool(glyphArea.size.width) & bool(glyphArea.size.height)) [[likely]] {
                // Break line if we reach end of map
                if (glyphArea.pos.x + glyphArea.size.width + 1.0f >= mapSize.width)
                    glyphArea.pos = Point { 1.0f, glyphArea.pos.y + fontCache.lineHeight + 1.0f };

                // Register font coordinates
                glyphMetrics.uv = glyphArea;
                glyphMetrics.bearing = Point { Pixel(metrics.horiBearingX / 64), Pixel((fontFace->size->metrics.ascender - metrics.horiBearingY) / 64) };
                glyphMetrics.advance = Pixel(metrics.horiAdvance / 64);

                // Increment x coordinate for next glyph
                glyphArea.pos.x += glyphArea.size.width + 1.0f;
            }

            // Get next glyph
            unicode = FT_Get_Next_Char(fontFace, unicode, &glyphIndex);
        }
        mapSize.height = glyphArea.pos.y + fontCache.lineHeight + 1.0f;
        fontCache.mapSize = mapSize;
    }

    { // Query space width
        const auto glyphIndex = FT_Get_Char_Index(fontFace, ' ');
        code = FT_Load_Glyph(fontFace, glyphIndex, FT_LOAD_BITMAP_METRICS_ONLY);
        kFEnsure(!code, "UI::FontManager::collectGlyphs: Couldn't compute space glyph metrics (", glyphIndex, ") of font '", fontIndex, "' (error: ", code, ')');
        fontCache.spaceWidth = Pixel(fontFace->glyph->metrics.horiAdvance / 64);
    }

    // Allocate map buffer
    MapBuffer buffer(std::uint32_t(fontCache.mapSize.width * fontCache.mapSize.height));

    { // Render glyphs
        // Setup compute cache
        FT_UInt glyphIndex {};
        auto unicode = FT_Get_First_Char(fontFace, &glyphIndex);
        const FT_Bitmap &bitmap = fontFace->glyph->bitmap;
        const auto mapWidth = std::uint32_t(fontCache.mapSize.width);
        for (std::uint32_t index {}; glyphIndex; ++index) {
            // Render glyph
            code = FT_Load_Glyph(fontFace, glyphIndex, FT_LOAD_RENDER);
            kFEnsure(!code, "UI::FontManager::renderGlyphs: Couldn't render glyph ", unicode,
                " (", glyphIndex, ") of font '", fontIndex, "' (error: ", code, ')');

            // Copy glyph into map buffer
            if (bool(fontFace->glyph->metrics.width) & bool(fontFace->glyph->metrics.height)) [[likely]] {
                // Compute global position
                const auto &glyphMetrics = fontCache.glyphsMetrics.at(index);
                const auto globalX = std::uint32_t(glyphMetrics.uv.pos.x);
                const auto globalY = std::uint32_t(glyphMetrics.uv.pos.y);
                for (auto localY = 0u; localY != bitmap.rows; ++localY) {
                    for (auto localX = 0u; localX != bitmap.width; ++localX) {
                        const auto localIndex = localY * bitmap.width + localX;
                        const auto alpha = std::uint8_t(bitmap.buffer[localIndex]);
                        const auto globalIndex = globalX + localX + (globalY + localY) * mapWidth;
                        buffer[globalIndex] = Color { .r = 255, .g = 255, .b = 255, .a = alpha };
                    }
                }
            }

            // Get next glyph
            unicode = FT_Get_Next_Char(fontFace, unicode, &glyphIndex);
        }
    }

    // Add sprite
    fontCache.sprite = uiSystem.spriteManager().add(SpriteManager::SpriteBuffer {
        .data = buffer.data(),
        .extent = GPU::Extent2D { std::uint32_t(fontCache.mapSize.width), std::uint32_t(fontCache.mapSize.height) }
    });

    // // Save bitmap as a file
    // auto imgPath = std::string(IO::File(path).filename()) + "_" + std::to_string(fontIndex) +".bmp";
    // ::stbi_write_bmp(imgPath.c_str(), std::uint32_t(fontCache.mapSize.width), std::uint32_t(fontCache.mapSize.height), 4, buffer.data());

#if KUBE_DEBUG_BUILD
    kFInfo("[UI] Init font ", fontIndex, ":\t Family ", fontFace->family_name, " Style ", fontFace->style_name);
#endif
    // Release font face
    FT_Done_Face(fontFace);
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

UI::Size UI::FontManager::computeTextMetrics(const FontIndex fontIndex, const std::string_view &text, const Pixel spacesPerTab) const noexcept
{
    constexpr auto UpdateMetrics = [](UI::Size &metrics, const UI::Point pen) {
        metrics.width = std::max(metrics.width, pen.x);
        metrics.height = std::max(metrics.height, pen.y);
    };
    const auto &glyphsMetrics = glyphsMetricsAt(fontIndex);
    const auto &glyphIndexSet = glyphIndexSetAt(fontIndex);
    const auto lineHeight = lineHeightAt(fontIndex);
    const auto spaceWidth = spaceWidthAt(fontIndex);
    Size metrics {};
    Point pen {};

    for (const auto it : text) {
        if (!std::isspace(it)) {
            pen.x += glyphsMetrics.at(glyphIndexSet.at(it)).advance;
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
