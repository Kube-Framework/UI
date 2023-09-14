/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Text processor
 */

#include <Kube/Core/Unicode.hpp>

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

    /** @brief Metrics of a single line */
    struct LineMetrics
    {
        std::uint32_t charCount {};
        Pixel spaceCount {};
        Pixel totalSize {};
        Pixel totalGlyphSize {};
        Pixel width {};
        bool elided {};
    };

    /** @brief Pixel small optimized cache */
    using LinesMetrics = Core::SmallVector<LineMetrics, Core::CacheLineSize / sizeof(LineMetrics), UIAllocator>;

    /** @brief Stores all parameters from a text computation */
    struct alignas_double_cacheline ComputeParameters
    {
        const Text *text {};
        const FontManager::GlyphIndexSet *glyphIndexSet {};
        const FontManager::GlyphsMetrics *glyphsMetrics {};
        Pixel spaceWidth {};
        Pixel ascender {};
        Pixel descender {};
        Pixel lineHeight {};
        Pixel baseline {};
        Pixel lineCount {};
        SpriteIndex spriteIndex {};
        Pixel elideSize {};
        LinesMetrics linesMetrics {};

        /** @brief Query glyph metrics of an unicode character */
        [[nodiscard]] inline const FontManager::GlyphMetrics &getMetricsOf(const std::uint32_t desired) const noexcept
        {
            return FontManager::GetMetricsOf(*glyphIndexSet, *glyphsMetrics, desired);
        }
    };
    static_assert_fit_double_cacheline(ComputeParameters);

    /** @brief Compute all glyphs from a text in packed mode */
    template<auto GetX, auto GetY>
    static void ComputeGlyph(Glyph *&out, ComputeParameters &params) noexcept;

    /** @brief Compute the metrics of a line of glyphs */
    template<auto GetX, auto GetY>
    [[nodiscard]] static LineMetrics ComputeLineMetrics(
        ComputeParameters &params,
        const std::string_view::iterator from,
        const std::string_view::iterator to,
        const Pixel yOffset
    ) noexcept;

    /** @brief Compute a line of glyphs from a text */
    template<auto GetX, auto GetY>
    static Pixel ComputeLine(
        Glyph *&out,
        ComputeParameters &params,
        std::string_view::iterator &it,
        const std::string_view::iterator to,
        const LineMetrics &metrics,
        const Pixel yOffset
    ) noexcept;

    /** @brief Compute the anchor of all glyphs within range */
    template<auto GetX, auto GetY>
    static void ComputeGlyphPositions(
        Glyph * const from,
        Glyph * const to,
        const ComputeParameters &params,
        const Size metrics
    ) noexcept;

    /** @brief Apply the offset of all glyphs within range */
    template<auto GetX, auto GetY>
    static void ApplyGlyphOffsets(
        Glyph * const from,
        Glyph * const to,
        const ComputeParameters &params,
        const Size metrics,
        const Point offset
    ) noexcept;
}

template<>
UI::PrimitiveProcessorModel UI::PrimitiveProcessor::QueryModel<UI::Text>(void) noexcept
{
    return UI::PrimitiveProcessorModel {
        .computeShader = GPU::Shader(":/UI/Shaders/FilledQuad/Text.comp.spv"),
        .computeLocalGroupSize = 128,
        .instanceSize = sizeof(Glyph),
        .instanceAlignment = alignof(Glyph),
        .verticesPerInstance = 4,
        .indicesPerInstance = 6
    };
}

template<>
std::uint32_t UI::PrimitiveProcessor::GetInstanceCount<UI::Text>(
    const Text * const primitiveBegin,
    const Text * const primitiveEnd
) noexcept
{
    std::uint32_t count {};
    for (auto it = primitiveBegin; it != primitiveEnd; ++it) {
        count += it->elide * ElideDotCount;
        for (const auto unicode : it->str) {
            if (!std::isspace(unicode)) [[likely]]
                ++count;
        }
    }
    return count;
}

template<>
std::uint32_t UI::PrimitiveProcessor::InsertInstances<UI::Text>(
    const Text * const primitiveBegin,
    const Text * const primitiveEnd,
    std::uint8_t * const instanceBegin
) noexcept
{
    const auto &fontManager = App::Get().uiSystem().fontManager();
    auto * const begin = reinterpret_cast<Glyph *>(instanceBegin);
    auto *out = begin;
    ComputeParameters params;

    for (const Text &text : Core::IteratorRange { primitiveBegin, primitiveEnd }) {
        // Query compute parameters
        params.text = &text;
        params.glyphIndexSet = &fontManager.glyphIndexSetAt(text.fontIndex);
        params.glyphsMetrics = &fontManager.glyphsMetricsAt(text.fontIndex);
        params.spaceWidth = fontManager.spaceWidthAt(text.fontIndex);
        params.ascender = fontManager.ascenderAt(text.fontIndex);
        params.descender = fontManager.descenderAt(text.fontIndex);
        params.lineHeight = fontManager.lineHeightAt(text.fontIndex);
        params.spriteIndex = fontManager.spriteAt(text.fontIndex);
        params.elideSize = params.getMetricsOf('.').advance * ElideDotCount * text.elide;
        params.linesMetrics.clear();

        // Dispatch
        if (!text.vertical) [[likely]]
            ComputeGlyph<GetXAxis, GetYAxis>(out, params);
        else
            ComputeGlyph<GetYAxis, GetXAxis>(out, params);
    }
    return Core::Distance<std::uint32_t>(begin, out);
}

template<auto GetX, auto GetY>
static void UI::ComputeGlyph(Glyph *&out, ComputeParameters &params) noexcept
{
    const auto begin = out;
    auto it = params.text->str.begin();
    const auto end = params.text->str.end();
    Size size {};

    while (it != end) {
        // Compute line metrics
        auto lineMetrics = ComputeLineMetrics<GetX, GetY>(params, it, end, GetY(size));

        // Compute line glyphs
            lineMetrics.width = ComputeLine<GetX, GetY>(out, params, it, end, lineMetrics, GetY(size));

        // Update caches
        params.linesMetrics.push(std::move(lineMetrics));
        GetX(size) = std::max(GetX(size), lineMetrics.width);
        GetY(size) += params.lineHeight;

        // Stop on line elided
        if (lineMetrics.elided) [[unlikely]]
            break;
    }

    // Position characters if any to draw
    if (begin != out) [[likely]]
        ComputeGlyphPositions<GetX, GetY>(begin, out, params, size);
}

template<auto GetX, auto GetY>
static UI::LineMetrics UI::ComputeLineMetrics(
    ComputeParameters &params,
    const std::string_view::iterator from,
    const std::string_view::iterator to,
    const Pixel yOffset
) noexcept
{
    constexpr auto CheckFit = [](const auto &metrics, const auto textSize, const auto xFit, const auto size) {
        return !xFit | (metrics.totalSize + size <= GetX(textSize));
    };

    LineMetrics elideMetrics;
    LineMetrics metrics;
    const auto spaceWidth = params.spaceWidth;
    const auto tabMultiplier = params.text->spacesPerTab - 1;
    const auto textSize = params.text->area.size;
    const bool lastLine = (yOffset + params.lineHeight * 2) > (GetY(params.text->area.pos) + GetY(params.text->area.size));
    const bool xFit = params.text->fit | params.text->elide;
    const bool xElide = params.text->elide & (lastLine | !params.text->fit);
    const auto elideSize = xElide * params.elideSize;
    auto charCount = 0u;

    for (auto it = from; true;) {
        // Get next unicode character
        const auto unicode = Core::Unicode::GetNextChar(it, to);
        // End of text
        if (!unicode)
            break;
        ++charCount;
        // Glyph
        if (!std::isspace(unicode)) {
            const auto advance = params.getMetricsOf(unicode).advance;
            if (CheckFit(metrics, textSize, xFit, advance + elideSize)) [[likely]] {
                metrics.totalSize += advance;
                metrics.totalGlyphSize += advance;
            } else [[unlikely]] {
                metrics.elided = xElide & params.text->elide;
                break;
            }
        // Space
        } else if (const bool isTab = unicode == '\t'; isTab | (unicode == ' ')) {
            const auto spaceCount = 1.0f + tabMultiplier * static_cast<Pixel>(isTab);
            const auto size = spaceWidth * spaceCount;
            if (CheckFit(metrics, textSize, xFit, size + elideSize)) [[likely]] {
                metrics.spaceCount += spaceCount;
                metrics.totalSize += size;
            } else [[unlikely]] {
                metrics.elided = xElide & params.text->elide;
                break;
            }
        // End of line
        } else
            break;
    }

    // Compute character count
    metrics.charCount = charCount;
    return metrics;
}

template<auto GetX, auto GetY>
static UI::Pixel UI::ComputeLine(
    Glyph *&out,
    ComputeParameters &params,
    std::string_view::iterator &it,
    const std::string_view::iterator to,
    const LineMetrics &metrics,
    const Pixel yOffset
) noexcept
{
    const auto spaceWidth = Core::BranchlessIf(
        params.text->textAlignment == TextAlignment::Justify,
        metrics.spaceCount ? (GetX(params.text->area.size) - metrics.totalGlyphSize) / metrics.spaceCount : 0.0f,
        params.spaceWidth
    );
    const auto tabMultiplier = params.text->spacesPerTab - 1;
    Point pos {};
    GetY(pos) = yOffset;

    // Insert glyphs
    const auto insertGlyph = [
        &out,
        &pos,
        &params,
        color = params.text->color,
        rotationAngle = params.text->rotationAngle,
        vertical = params.text->vertical
    ](const auto &metrics) {
        auto glyphPos = pos;
        GetX(glyphPos) += metrics.bearing.x;
        GetY(glyphPos) += !vertical
            ? params.ascender - metrics.bearing.y
            : -params.descender - (metrics.uv.size.height - metrics.bearing.y);
        new (out++) Glyph {
            .uv = metrics.uv,
            .pos = glyphPos,
            .spriteIndex = params.spriteIndex,
            .color = color,
            .rotationAngle = rotationAngle,
            .vertical = float(vertical),
        };
        GetX(pos) += metrics.advance;
    };
    for (auto count = 0u; count != metrics.charCount; ++count) {
        // Get next unicode character
        const auto unicode = Core::Unicode::GetNextChar(it, to);
        // End of text
        if (!unicode)
            break;
        // Glyph
        else if (!std::isspace(unicode)) {
            const auto &metrics = params.getMetricsOf(unicode);
            insertGlyph(metrics);
        // Space
        } else if (const bool isTab = unicode == '\t'; isTab | (unicode == ' ')) {
            const auto spaceCount = 1.0f + tabMultiplier * static_cast<Pixel>(isTab);
            GetX(pos) += spaceWidth * spaceCount;
        } else
            break;
    }

    if (metrics.elided) [[unlikely]] {
        const auto &metrics = params.getMetricsOf('.');
        for (auto i = 0u; i != ElideDotCount; ++i)
            insertGlyph(metrics);
    }
    return GetX(pos);
}

template<auto GetX, auto GetY>
static void UI::ComputeGlyphPositions(
    Glyph * const from,
    Glyph * const to,
    const ComputeParameters &params,
    const Size metrics
) noexcept
{
    // Compute global offset
    Point offset {};
    switch (params.text->anchor) {
    case Anchor::TopLeft:
        break;
    case Anchor::Top:
        offset = Point {
            .x = params.text->area.size.width / 2.0f - metrics.width / 2.0f,
            .y = 0.0f
        };
        break;
    case Anchor::TopRight:
        offset = Point {
            .x = params.text->area.size.width - metrics.width,
            .y = 0.0f
        };
        break;
    case Anchor::Left:
        offset = Point {
            .x = 0.0f,
            .y = params.text->area.size.height / 2.0f - metrics.height / 2.0f
        };
        break;
    case Anchor::Center:
        offset = (params.text->area.size / 2.0f - metrics / 2.0f).toPoint();
        break;
    case Anchor::Right:
        offset = Point {
            .x = params.text->area.size.width - metrics.width,
            .y = params.text->area.size.height / 2.0f - metrics.height / 2.0f
        };
        break;
    case Anchor::BottomLeft:
        offset = Point {
            .x = 0.0f,
            .y = params.text->area.size.height - metrics.height
        };
        break;
    case Anchor::Bottom:
        offset = Point {
            .x = params.text->area.size.width / 2.0f - metrics.width / 2.0f,
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
    offset = Point {
        .x = std::round(offset.x + params.text->area.pos.x),
        .y = std::round(offset.y + params.text->area.pos.y),
    };
    ApplyGlyphOffsets<GetX, GetY>(from, to, params, metrics, offset);
}

template<auto GetX, auto GetY>
static void UI::ApplyGlyphOffsets(Glyph * const from, Glyph * const to, const ComputeParameters &params, const Size metrics, const Point offset) noexcept
{
    constexpr auto ApplyOffsets = [](Glyph * const from, Glyph * const to, const ComputeParameters &params, const Size metrics, const Point offset, const Point rotationOrigin, auto &&computeFunc) {
        auto lineMetricsIt = params.linesMetrics.begin();
        auto currentOffset = computeFunc(offset, metrics, lineMetricsIt->width);
        auto nextLineIt = from + std::uint32_t(lineMetricsIt->charCount - std::uint32_t(lineMetricsIt->spaceCount));
        for (auto it = from; it != to; ++it) {
            if (it == nextLineIt) [[unlikely]] {
                nextLineIt = from + lineMetricsIt->charCount - std::uint32_t(lineMetricsIt->spaceCount);
                ++lineMetricsIt;
                currentOffset = computeFunc(offset, metrics, lineMetricsIt->width);
            }
            it->pos += currentOffset;
            it->rotationOrigin = rotationOrigin;
        }
    };

    const Point rotationOrigin = offset + metrics / 2.0f;
    switch (params.text->textAlignment) {
    case TextAlignment::Left:
    case TextAlignment::Justify:
        for (auto it = from; it != to; ++it) {
            it->pos += offset;
            it->rotationOrigin = rotationOrigin;
        }
        break;
    case TextAlignment::Center:
        ApplyOffsets(from, to, params, metrics, offset, rotationOrigin,
            [](const Point offset, const Size metrics, const Pixel lineWidth) {
                Point point {};
                GetX(point) = GetX(metrics) / 2.0f - lineWidth / 2.0f;
                return offset + point;
            }
        );
        break;
    case TextAlignment::Right:
        ApplyOffsets(from, to, params, metrics, offset, rotationOrigin,
            [](const Point offset, const Size metrics, const Pixel lineWidth) {
                Point point {};
                GetX(point) = GetX(metrics) - lineWidth;
                return offset + point;
            }
        );
        break;
    }
}