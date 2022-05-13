/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Text processor
 */

#include <Kube/IO/File.hpp>

#include <Kube/UI/App.hpp>
#include <Kube/UI/UISystem.hpp>

#include "TextProcessor.hpp"

using namespace kF;

namespace kF::UI
{
    /** @brief Single glyph instance */
    struct alignas_half_cacheline Glyph
    {
        Area uv {};
        Point pos {};
        SpriteIndex spriteIndex {};
        Color color {};
    };
    static_assert_alignof_half_cacheline(Glyph);

    /** @brief Pixel small optimized cache */
    using PixelCache = Core::TinySmallVector<Pixel, Core::CacheLineSize / sizeof(Pixel), UIAllocator>;

    /** @brief Stores all parameters of a text computation */
    struct alignas_double_cacheline ComputeParameters
    {
        const Text *text {};
        const FontManager::GlyphIndexSet *glyphIndexSet {};
        const FontManager::GlyphUVs *glyphUVs {};
        Pixel spaceWidth {};
        Pixel lineHeight {};
        Size mapSize {};
        SpriteIndex spriteIndex {};
        PixelCache pixelCache {};
    };
    static_assert_fit_double_cacheline(ComputeParameters);


    /** @brief Compute all glyphs of a text in packed mode */
    void ComputeGlyphPacked(Glyph *&out, ComputeParameters &params) noexcept;

    /** @brief Compute all glyphs of a text in justified mode */
    void ComputeGlyphJustified(Glyph *&out, ComputeParameters &params) noexcept;

    /** @brief Compute the anchor of all glyphs within range */
    void ComputeGlyphPositions(Glyph * const from, Glyph * const to,
            const ComputeParameters &params, const Size metrics) noexcept;

    /** @brief Apply the offset of all glyphs within range */
    void ApplyGlyphOffsets(Glyph * const from, Glyph * const to,
            const ComputeParameters &params, const Size metrics, const Point offset) noexcept;
}

template<>
UI::PrimitiveProcessorModel UI::PrimitiveProcessor::QueryModel<UI::Text>(void) noexcept
{
    return UI::PrimitiveProcessorModel {
            .computeShader = GPU::Shader(IO::File(":/UI/Shaders/Text.comp.spv").queryResource()),
        .computeLocalGroupSize = 1,
        .instanceSize = sizeof(Glyph),
        .instanceAlignment = alignof(Glyph),
        .verticesPerInstance = 4,
        .indicesPerInstance = 6
    };
}

template<>
std::uint32_t UI::PrimitiveProcessor::GetInstanceCount<UI::Text>(
        const Text * const primitiveBegin, const Text * const primitiveEnd) noexcept
{
    std::uint32_t count {};
    for (auto it = primitiveBegin; it != primitiveEnd; ++it) {
        for (const auto unicode : it->str) {
            if (!std::isspace(unicode)) [[likely]]
                ++count;
        }
    }
    return count;
}

template<>
void UI::PrimitiveProcessor::InsertInstances<UI::Text>(
        const Text * const primitiveBegin, const Text * const primitiveEnd,
        std::uint8_t * const instanceBegin) noexcept
{
    const auto &fontManager = App::Get().uiSystem().fontManager();
    auto *out = reinterpret_cast<Glyph *>(instanceBegin);
    ComputeParameters params;

    for (const auto &text : Core::IteratorRange { primitiveBegin, primitiveEnd }) {
        // Query compute parameters
        params.text = &text;
        params.glyphIndexSet = &fontManager.glyphIndexSetAt(text.font);
        params.glyphUVs = &fontManager.glyphUVsAt(text.font, text.fontInstance);
        params.spaceWidth = fontManager.spaceWidthAt(text.font, text.fontInstance);
        params.lineHeight = fontManager.lineHeightAt(text.font, text.fontInstance);
        params.mapSize = fontManager.mapSizeAt(text.font, text.fontInstance);
        params.spriteIndex = fontManager.spriteAt(text.font, text.fontInstance);
        params.pixelCache.clear();

        // Compute glyphs areas
        if (!text.justify) [[likely]]
            ComputeGlyphPacked(out, params);
        else
            ComputeGlyphJustified(out, params);
    }
}

static void UI::ComputeGlyphPacked(Glyph *&out, ComputeParameters &params) noexcept
{
    constexpr auto UpdateMetrics = [](ComputeParameters &params, const Point &pos, Size &metrics) {
        metrics = Size {
            .width = std::max(metrics.width, pos.x),
            .height = metrics.height + params.lineHeight
        };
        params.pixelCache.push(pos.x);
    };

    const auto begin = out;
    Size metrics;
    Point pos;

    for (const auto unicode : params.text->str) {
        if (!std::isspace(unicode)) [[likely]] {
            const auto index = params.glyphIndexSet->at(unicode);
            const auto &uv = params.glyphUVs->at(index);
            new (out) Glyph {
                .uv = uv,
                .pos = pos,
                .spriteIndex = params.spriteIndex,
                .color = params.text->color
            };
            pos.x += uv.size.width;
            ++out;
        } else if (const bool isTab = unicode == '\t'; unicode == ' ' | isTab) {
            pos.x += params.spaceWidth * (1 + 3 * isTab);
        } else [[unlikely]] {
            UpdateMetrics(params, pos, metrics);
            pos.x = 0;
            pos.y += params.lineHeight;
        }
    }
    UpdateMetrics(params, pos, metrics);

    // No character to draw
    if (begin == out) [[unlikely]]
        return;

    // Compute anchor
    ComputeGlyphPositions(begin, out, params, metrics);
}

static void UI::ComputeGlyphPositions(Glyph * const from, Glyph * const to, const ComputeParameters &params, const Size metrics) noexcept
{
    // Compute global offset
    Point offset {};
    switch (params.text->anchor) {
    case Anchor::Center:
        offset = (params.text->area.size / 2.0f - metrics / 2.0f).toPoint();
        break;
    case Anchor::Left:
        offset = Point {
            .x = 0.0f,
            .y = params.text->area.size.height / 2.0f - metrics.height / 2.0f
        };
        break;
    case Anchor::Right:
        offset = Point {
            .x = params.text->area.size.width - metrics.width,
            .y = params.text->area.size.height / 2.0f - metrics.height / 2.0f
        };
        break;
    case Anchor::Top:
        offset = Point {
            .x = params.text->area.size.width / 2.0f - metrics.width / 2.0f,
            .y = 0.0f
        };
        break;
    case Anchor::Bottom:
        offset = Point {
            .x = params.text->area.size.width / 2.0f - metrics.width / 2.0f,
            .y = params.text->area.size.height - metrics.height
        };
        break;
    case Anchor::TopLeft:
        break;
    case Anchor::TopRight:
        offset = Point {
            .x = params.text->area.size.width - metrics.width,
            .y = 0.0f
        };
        break;
    case Anchor::BottomLeft:
        offset = Point {
            .x = 0.0f,
            .y = params.text->area.size.height - metrics.height
        };
        break;
    case Anchor::BottomRight:
        offset = Point {
            .x = params.text->area.size.width - metrics.width,
            .y = params.text->area.size.height - metrics.height
        };
        break;
    }

    // Apply offsets
    offset += params.text->area.pos;
    ApplyGlyphOffsets(from, to, params, metrics, offset);
}

static void UI::ApplyGlyphOffsets(Glyph * const from, Glyph * const to, const ComputeParameters &params, const Size metrics, const Point offset) noexcept
{
    constexpr auto ApplyOffsets = [](Glyph * const from, Glyph * const to, const ComputeParameters &params, const Size metrics, const Point offset, auto &&computeFunc) {
        auto lineWidthIt = params.pixelCache.begin();
        std::uint32_t index {};
        Pixel oldY = from->pos.y;
        auto currentOffset = computeFunc(offset, metrics, *lineWidthIt);
        for (auto it = from; it != to; ++it) {
            if (oldY != it->pos.y) [[unlikely]] {
                oldY = it->pos.y;
                ++lineWidthIt;
                currentOffset = computeFunc(offset, metrics, *lineWidthIt);
            }
            it->pos += currentOffset;
        }
    };

    switch (params.text->alignment) {
    case TextAlignment::Left:
        for (auto it = from; it != to; ++it)
            it->pos += offset;
        break;
    case TextAlignment::Center:
        ApplyOffsets(from, to, params, metrics, offset, [](const Point offset, const Size metrics, const Pixel lineWidth) {
            return offset + Point(metrics.width / 2.0f - lineWidth / 2.0f, 0.0f);
        });
        break;
    case TextAlignment::Right:
        ApplyOffsets(from, to, params, metrics, offset, [](const Point offset, const Size metrics, const Pixel lineWidth) {
            return offset + Point(metrics.width - lineWidth, 0.0f);
        });
        break;
    default:
        kFAbort("UI::ComputeGlyphPositions: Unexpected text alignment");
    }
}

static void UI::ComputeGlyphJustified(Glyph *&out, ComputeParameters &params) noexcept
{
    ComputeGlyphPacked(out, params);
    // auto from = params.text->str.begin();
    // auto to = from;
    // const auto end = params.text->str.end();
    // Pixel totalLineWidth {};

    // // Loop over all characters
    // while (from != end) {
    //     totalLineWidth = Pixel {};
    //     // Compute all words of a strict line
    //     while (from != end && *from == '\n') {
    //         // Compute word width
    //         Pixel wordWidth {};
    //         const auto old = from;
    //         for (to = from; to != end && !std::isspace(*to); ++to) {
    //             const auto index = params.glyphIndexSet->at(*to);
    //             const auto &uv = params.glyphUVs->at(index);
    //             wordWidth += uv.size.width * params.mapSize.width;
    //         }
    //         if (wordWidth) [[likely]]
    //             params.pixelCache.push(wordWidth);
    //         totalLineWidth += wordWidth;
    //         if (totalLineWidth > params.area.size.width)

    //         // Compute space width
    //         wordWidth = Pixel {};
    //         while (to != end && (*to == ' ' | *to == '\t')) {
    //             wordWidth += params.spaceWidth * ((3.0f * to == '\t') + 1.0f);
    //         }
    //         if (wordWidth) [[likely]]
    //             params.pixelCache.push(wordWidth);
    //     }

    //     // Clear cache
    //     params.pixelCache.clear();
    // }
}