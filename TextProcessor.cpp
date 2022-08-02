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
    struct alignas_quarter_cacheline Glyph
    {
        Area uv {};
        Point pos {};
        SpriteIndex spriteIndex {};
        Color color {};
        Point rotationOrigin {};
        float rotationAngle {};
        float vertical {};
    };
    static_assert_alignof_quarter_cacheline(Glyph);

    /** @brief Pixel small optimized cache */
    using PixelCache = Core::SmallVector<Pixel, Core::CacheLineSize / sizeof(Pixel), UIAllocator>;

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


    /** @brief Compute all glyphs of a text */
    static void ComputeGlyph(const Text &text, Glyph *&out, ComputeParameters &params) noexcept;

    /** @brief Dispatch compute instantiation glyph of a text */
    template<auto GetX, auto GetY>
    static void DispatchComputeGlyph(const Text &text, Glyph *&out, ComputeParameters &params) noexcept;


    /** @brief Compute all glyphs of a text in packed mode */
    template<auto GetX, auto GetY>
    static void ComputeGlyphPacked(Glyph *&out, ComputeParameters &params) noexcept;

    /** @brief Compute the anchor of all glyphs within range */
    template<auto GetX, auto GetY>
    static void ComputeGlyphPositions(Glyph * const from, Glyph * const to,
            const ComputeParameters &params, const Size metrics) noexcept;

    /** @brief Apply the offset of all glyphs within range */
    template<auto GetX, auto GetY>
    static void ApplyGlyphOffsets(Glyph * const from, Glyph * const to,
            const ComputeParameters &params, const Size metrics, const Point offset) noexcept;


    /** @brief Compute all glyphs of a text in justified mode */
    template<auto GetX, auto GetY>
    static void ComputeGlyphJustified(Glyph *&out, ComputeParameters &params) noexcept;
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
std::uint32_t UI::PrimitiveProcessor::InsertInstances<UI::Text>(
        const Text * const primitiveBegin, const Text * const primitiveEnd,
        std::uint8_t * const instanceBegin) noexcept
{
    const auto &fontManager = App::Get().uiSystem().fontManager();
    auto * const begin = reinterpret_cast<Glyph *>(instanceBegin);
    auto *out = begin;
    ComputeParameters params;

    for (const auto &text : Core::IteratorRange { primitiveBegin, primitiveEnd }) {
        // Query compute parameters
        params.text = &text;
        params.glyphIndexSet = &fontManager.glyphIndexSetAt(text.fontIndex);
        params.glyphUVs = &fontManager.glyphUVsAt(text.fontIndex);
        params.spaceWidth = fontManager.spaceWidthAt(text.fontIndex);
        params.lineHeight = fontManager.lineHeightAt(text.fontIndex);
        params.mapSize = fontManager.mapSizeAt(text.fontIndex);
        params.spriteIndex = fontManager.spriteAt(text.fontIndex);
        params.pixelCache.clear();

        // Dispatch
        ComputeGlyph(text, out, params);
    }
    return static_cast<std::uint32_t>(std::distance(begin, out));
}

static void UI::ComputeGlyph(const Text &text, Glyph *&out, ComputeParameters &params) noexcept
{
    if (!text.vertical) [[likely]]
        DispatchComputeGlyph<GetXAxis, GetYAxis>(text, out, params);
    else
        DispatchComputeGlyph<GetYAxis, GetXAxis>(text, out, params);
}

template<auto GetX, auto GetY>
static void UI::DispatchComputeGlyph(const Text &text, Glyph *&out, ComputeParameters &params) noexcept
{
    // Compute glyphs areas
    if (!text.justify) [[likely]]
        ComputeGlyphPacked<GetX, GetY>(out, params);
    else
        ComputeGlyphJustified<GetX, GetY>(out, params);
};

template<auto GetX, auto GetY>
static void UI::ComputeGlyphPacked(Glyph *&out, ComputeParameters &params) noexcept
{
    constexpr auto UpdateMetrics = [](ComputeParameters &params, const Point &pos, Size &metrics) {
        GetX(metrics) = std::max(GetX(metrics), GetX(pos));
        GetY(metrics) = GetY(metrics) + params.lineHeight;
        params.pixelCache.push(GetX(pos));
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
                .color = params.text->color,
                .rotationAngle = params.text->rotationAngle,
                .vertical = static_cast<float>(params.text->vertical)
            };
            GetX(pos) += uv.size.width;
            ++out;
        } else if (const bool isTab = unicode == '\t'; (unicode == ' ') | isTab) {
            GetX(pos) += params.spaceWidth * (1.0f + 3.0f * static_cast<float>(isTab));
        } else [[unlikely]] {
            UpdateMetrics(params, pos, metrics);
            GetX(pos) = 0;
            GetY(pos) += params.lineHeight;
        }
    }
    UpdateMetrics(params, pos, metrics);

    // Position characters if any to draw
    if (begin != out) [[likely]]
        ComputeGlyphPositions<GetX, GetY>(begin, out, params, metrics);
}

template<auto GetX, auto GetY>
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
    ApplyGlyphOffsets<GetX, GetY>(from, to, params, metrics, offset);
}

template<auto GetX, auto GetY>
static void UI::ApplyGlyphOffsets(Glyph * const from, Glyph * const to, const ComputeParameters &params, const Size metrics, const Point offset) noexcept
{
    constexpr auto ApplyOffsets = [](Glyph * const from, Glyph * const to, const ComputeParameters &params, const Size metrics, const Point offset, const Point rotationOrigin, auto &&computeFunc) {
        auto lineWidthIt = params.pixelCache.begin();
        Pixel oldY = GetY(from->pos);
        auto currentOffset = computeFunc(offset, metrics, *lineWidthIt);
        for (auto it = from; it != to; ++it) {
            if (oldY != GetY(it->pos)) [[unlikely]] {
                oldY = GetY(it->pos);
                ++lineWidthIt;
                currentOffset = computeFunc(offset, metrics, *lineWidthIt);
            }
            it->pos += currentOffset;
            it->rotationOrigin = rotationOrigin;
        }
    };

    const Point rotationOrigin = offset + metrics / 2.0f;

    switch (params.text->textAlignment) {
    case TextAlignment::Left:
        for (auto it = from; it != to; ++it) {
            it->pos += offset;
            it->rotationOrigin = rotationOrigin;
        }
        break;
    case TextAlignment::Center:
        ApplyOffsets(from, to, params, metrics, offset, rotationOrigin, [](const Point offset, const Size metrics, const Pixel lineWidth) {
            Point point {};
            GetX(point) = GetX(metrics) / 2.0f - lineWidth / 2.0f;
            return offset + point;
        });
        break;
    case TextAlignment::Right:
        ApplyOffsets(from, to, params, metrics, offset, rotationOrigin, [](const Point offset, const Size metrics, const Pixel lineWidth) {
            Point point {};
            GetX(point) = GetX(metrics) - lineWidth;
            return offset + point;
        });
        break;
    default:
        kFAbort("UI::ComputeGlyphPositions: Unexpected text alignment");
    }
}

template<auto GetX, auto GetY>
static void UI::ComputeGlyphJustified(Glyph *&out, ComputeParameters &params) noexcept
{
    ComputeGlyphPacked<GetX, GetY>(out, params);
}
