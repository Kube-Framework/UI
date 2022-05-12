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

UI::FontManager::FontCache::~FontCache(void) noexcept
{
    if (face)
        FT_Done_Face(face);
}

UI::FontManager::FontCache::FontCache(FontCache &&other) noexcept
    :   instanceModels(std::move(other.instanceModels)),
        instanceCaches(std::move(other.instanceCaches)),
        instanceFreeList(std::move(other.instanceFreeList)),
        instanceCounters(std::move(other.instanceCounters)),
        face(other.face),
        glyphIndexSet(std::move(other.glyphIndexSet))
{
    other.face = FT_Face {};
}

void UI::FontManager::FontCache::swap(FontCache &other) noexcept
{
    std::swap(instanceModels, other.instanceModels);
    std::swap(instanceCaches, other.instanceCaches);
    std::swap(instanceFreeList, other.instanceFreeList);
    std::swap(instanceCounters, other.instanceCounters);
    std::swap(face, other.face);
    std::swap(glyphIndexSet, other.glyphIndexSet);
}

UI::FontManager::~FontManager(void) noexcept
{
    FT_Done_FreeType(_backend);
}

UI::FontManager::FontManager(void) noexcept
{
    FT_Init_FreeType(&_backend);
}

UI::Font UI::FontManager::add(const std::string_view &path) noexcept
{
    // Try to find an existing instance of the queried font
    const auto fontName = Core::Hash(path);
    if (const auto it = _fontNames.find(fontName); it != _fontNames.end()) [[likely]] {
        const FontIndex fontIndex { static_cast<FontIndex::IndexType>(std::distance(it, _fontNames.end())) };
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

    // Build font cache at 'fontIndex'
    load(path, fontIndex);

    // Build font
    return Font(*this, fontIndex);
}

void UI::FontManager::load(const std::string_view &path, const FontIndex fontIndex) noexcept
{
    auto &fontCache = _fontCaches.at(fontIndex);
    FT_Error code {};

    // If file is a resource it is already loaded in RAM
    if (IO::File file(path); file.isResource()) {
        const auto range = file.queryResource();
        code = FT_New_Memory_Face(_backend, range.begin(), static_cast<FT_Long>(range.size()), FT_Long { 0 }, &fontCache.face);
    // Else we need to load file from flash storage
    } else {
        code = FT_New_Face(_backend, std::string(path).c_str(), FT_Long { 0 }, &fontCache.face);
    }
    kFEnsure(!code,
        "UI::FontManager::load: Couldn't load font at path '", path, " (error: ", code, ')');

    // Collect each glyph index
    FT_UInt glyphIndex {};
    std::uint32_t glyphCount {};
    auto unicode = FT_Get_First_Char(fontCache.face, &glyphIndex);
    for (std::uint32_t index {}; glyphIndex; ++index) {
        fontCache.glyphIndexSet.add(unicode, index);
        unicode = FT_Get_Next_Char(fontCache.face, unicode, &glyphIndex);
        ++glyphCount;
    }
    fontCache.glyphCount = glyphCount;
}

UI::FontInstance UI::FontManager::addInstance(const FontIndex fontIndex, const FontInstanceModel &model) noexcept
{
    auto &fontCache = _fontCaches.at(fontIndex);

    // Try to find an existing instance of the queried font
    if (const auto it = fontCache.instanceModels.find(model); it != fontCache.instanceModels.end()) [[likely]] {
        const FontInstanceIndex instanceIndex { static_cast<FontInstanceIndex::IndexType>(std::distance(it, fontCache.instanceModels.end())) };
        ++fontCache.instanceCounters.at(instanceIndex);
        const SpriteIndex spriteIndex = fontCache.instanceCaches.at(instanceIndex).sprite;
        return FontInstance(*this, fontIndex, instanceIndex, spriteIndex);
    }

    // We didn't found an instance: either get free index or create a new one
    FontInstanceIndex instanceIndex {};
    if (!fontCache.instanceFreeList.empty()) {
        instanceIndex.value = fontCache.instanceFreeList.back();
        fontCache.instanceFreeList.pop();
    } else {
        instanceIndex.value = fontCache.instanceModels.size();
        fontCache.instanceModels.push();
        fontCache.instanceCaches.push();
        fontCache.instanceCounters.push();
    }

    // Set font reference count and name
    fontCache.instanceCounters.at(instanceIndex) = 1u;
    fontCache.instanceModels.at(instanceIndex) = model;

    // Build font cache at 'fontIndex'
    loadInstance(fontIndex, instanceIndex);

    // Build font instance
    const SpriteIndex spriteIndex = fontCache.instanceCaches.at(instanceIndex).sprite;
    return FontInstance(*this, fontIndex, instanceIndex, spriteIndex);
}

void UI::FontManager::loadInstance(const FontIndex fontIndex, const FontInstanceIndex instanceIndex) noexcept
{
    // Query font data
    auto &fontCache = _fontCaches.at(fontIndex);
    auto &instanceCache = fontCache.instanceCaches.at(instanceIndex);
    const auto &instanceModel = fontCache.instanceModels.at(instanceIndex);

    { // Set font size
        auto code = FT_Set_Pixel_Sizes(fontCache.face, 0, instanceModel.pixelHeight);
        if (!code)
            code = FT_Activate_Size(fontCache.face->size);
        kFEnsure(!code,
            "UI::FontManager::load: Couldn't set font pixel size to ", instanceModel.pixelHeight, " of font '", fontIndex,  "(error: ", code, ')');
    }

    // Collect all glyphs to render
    const auto mapSize = collectGlyphs(fontIndex, instanceIndex);

    // Render all glyphs
    const auto buffer = renderGlyphs(fontIndex, instanceIndex, mapSize);

    // Add sprite
    instanceCache.sprite = UI::App::Get().uiSystem().spriteManager().add(SpriteManager::SpriteBuffer {
        .data = buffer.data(),
        .extent = GPU::Extent2D { mapSize.width, mapSize.height }
    });

    // Save bitmap as a file
    // ::stbi_write_bmp("bitmap.bmp", mapSize.width, mapSize.height, 4, buffer.data());

    kFInfo("Font: ", fontIndex,
        "\n\tMaximum under baseline: ", instanceCache.maxUnderBaseline,
        "\n\tNum glyphs: ", fontCache.face->num_glyphs,
        "\n\tNum charmap: ", fontCache.face->num_charmaps,
        "\n\tFamily: ", fontCache.face->family_name,
        "\n\tStyle: ", fontCache.face->style_name,
        "\n\tMetrics max_advance_width: ", fontCache.face->size->metrics.max_advance / 64,
        "\n\tMetrics height: ", fontCache.face->size->metrics.height / 64,
        "\n\tMetrics Over baseline2: ", fontCache.face->size->metrics.ascender / 64,
        "\n\tMetrics Under baseline2: ", fontCache.face->size->metrics.descender / 64
    );
}

UI::FontManager::MapSize UI::FontManager::collectGlyphs(const FontIndex fontIndex, const FontInstanceIndex instanceIndex) noexcept
{
    // Query font data
    auto &fontCache = _fontCaches.at(fontIndex);
    auto &instanceCache = fontCache.instanceCaches.at(instanceIndex);
    const auto &instanceModel = fontCache.instanceModels.at(instanceIndex);

    // Compute map size
    MapSize mapSize {
        .width = Core::Utils::NextPowerOf2(
            static_cast<std::uint32_t>(instanceModel.pixelHeight * std::sqrt(static_cast<float>(fontCache.glyphCount)))
        ),
        .height = 0u
    };

    // Setup compute cache
    FT_UInt glyphIndex {};
    auto unicode = FT_Get_First_Char(fontCache.face, &glyphIndex);
    const Pixel pixelMapWidth = static_cast<Pixel>(mapSize.width);
    Area glyphArea { .size = { .height = static_cast<Pixel>(fontCache.face->size->metrics.height) / 64.0f } };

    // Collect each glyph meta data
    instanceCache.glyphUVs.resizeUninitialized(fontCache.glyphCount);
    for (std::uint32_t index {}; glyphIndex; ++index) {
        { // Load glyph metrics
            const auto code = FT_Load_Glyph(fontCache.face, glyphIndex, FT_LOAD_BITMAP_METRICS_ONLY);
            kFEnsure(!code,
                "UI::FontManager::collectGlyphs: Couldn't compute glyph metrics ", unicode, " (", glyphIndex, ") of font '", fontIndex, "' (error: ", code, ')');
        }

        // Discard empty glyphs
        const FT_Glyph_Metrics &metrics = fontCache.face->glyph->metrics;
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
            new (&instanceCache.glyphUVs.at(index)) Area { glyphArea };
            glyphArea.pos.x = nextX;
        } else [[unlikely]] {
            new (&instanceCache.glyphUVs.at(index)) Area {};
        }

        unicode = FT_Get_Next_Char(fontCache.face, unicode, &glyphIndex);
    }

    // Update maximum baseline
    instanceCache.maxUnderBaseline = static_cast<Pixel>(fontCache.face->size->metrics.descender) / 64.0f;

    // Update map height
    mapSize.height = static_cast<std::uint32_t>(glyphArea.pos.y + glyphArea.size.height);

    // Update instance map size
    instanceCache.mapSize = Size(static_cast<Pixel>(mapSize.width), static_cast<Pixel>(mapSize.height));

    // Update instance space width
    instanceCache.spaceWidth = instanceCache.glyphUVs.at(fontCache.glyphIndexSet.at(' ')).size.width;

    // Update instance line height
    instanceCache.lineHeight = glyphArea.size.height;

    return mapSize;
}

UI::FontManager::MapBuffer UI::FontManager::renderGlyphs(const FontIndex fontIndex, const FontInstanceIndex instanceIndex, const MapSize mapSize) noexcept
{
    // Query font data
    const auto &fontCache = _fontCaches.at(fontIndex);
    const auto &instanceCache = fontCache.instanceCaches.at(instanceIndex);

    // Allocate map buffer
    MapBuffer buffer(mapSize.width * mapSize.height);

    // Setup compute cache
    const auto maxUnderBaseline = instanceCache.maxUnderBaseline;
    FT_UInt glyphIndex {};
    auto unicode = FT_Get_First_Char(fontCache.face, &glyphIndex);
    const FT_Bitmap &bitmap = fontCache.face->glyph->bitmap;

    for (std::uint32_t index {}; glyphIndex; ++index) {
        { // Render glyph
            const auto code = FT_Load_Glyph(fontCache.face, glyphIndex, FT_LOAD_RENDER);
            kFEnsure(!code,
                "UI::FontManager::renderGlyphs: Couldn't render glyph ", unicode, " (", glyphIndex, ") of font '", fontIndex, "' (error: ", code, ')');
        }

        // Ensure we are rendering good metrics
        const auto &metrics = fontCache.face->glyph->metrics;
        if (metrics.horiAdvance) [[likely]] {
            // Compute global position
            const auto &area = instanceCache.glyphUVs.at(index);
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

        unicode = FT_Get_Next_Char(fontCache.face, unicode, &glyphIndex);
    }

    return buffer;
}

void UI::FontManager::removeUnsafe(const FontIndex fontIndex) noexcept
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

void UI::FontManager::removeInstanceUnsafe(const FontIndex fontIndex, const FontInstanceIndex instanceIndex) noexcept
{
    auto &fontCache = _fontCaches.at(fontIndex);

    // Check instance reference counter
    if (--fontCache.instanceCounters.at(instanceIndex)) [[likely]]
        return;

    // Reset instance model
    fontCache.instanceModels.at(instanceIndex) = FontInstanceModel {};

    // Reset instance cache
    fontCache.instanceCaches.at(instanceIndex) = InstanceCache {};

    // Insert instance index into free list
    fontCache.instanceFreeList.push(instanceIndex);
}
