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

    /** @brief Stores all parameters from a text computation */
    struct alignas_double_cacheline ComputeParameters
    {
        const Text *text {};
        const FontManager::GlyphIndexSet *glyphIndexSet {};
        const FontManager::GlyphUVs *glyphUVs {};
        Size mapSize {};
        Pixel spaceWidth {};
        Pixel lineHeight {};
        Pixel lineCount {};
        SpriteIndex spriteIndex {};
        PixelCache pixelCache {};
    };
    static_assert_fit_double_cacheline(ComputeParameters);

    /** @brief Metrics of a single line */
    struct alignas_half_cacheline LineMetrics
    {
        std::uint32_t charCount {};
        Pixel spaceCount {};
        Pixel totalSize {};
        Pixel totalGlyphSize {};
        bool elided {};
    };
    static_assert_fit_half_cacheline(LineMetrics);

    /** @brief Compute all glyphs from a text in packed mode */
    template<auto GetX, auto GetY>
    static void ComputeGlyph(Glyph *&out, ComputeParameters &params) noexcept;

    /** @brief Compute the metrics of a line of glyphs */
    template<auto GetX, auto GetY>
    [[nodiscard]] static LineMetrics ComputeLineMetrics(ComputeParameters &params,
            const std::string_view::iterator from, const std::string_view::iterator to,
            const Pixel yOffset) noexcept;

    /** @brief Compute a line of glyphs from a text */
    template<auto GetX, auto GetY>
    static Pixel ComputeLine(Glyph *&out, ComputeParameters &params,
            const std::string_view::iterator from, const std::string_view::iterator to,
            const LineMetrics &metrics, const Pixel yOffset) noexcept;

    /** @brief Compute the anchor of all glyphs within range */
    template<auto GetX, auto GetY>
    static void ComputeGlyphPositions(Glyph * const from, Glyph * const to,
            const ComputeParameters &params, const Size metrics) noexcept;

    /** @brief Apply the offset of all glyphs within range */
    template<auto GetX, auto GetY>
    static void ApplyGlyphOffsets(Glyph * const from, Glyph * const to,
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
        params.mapSize = fontManager.mapSizeAt(text.fontIndex);
        params.spaceWidth = fontManager.spaceWidthAt(text.fontIndex);
        params.lineHeight = fontManager.lineHeightAt(text.fontIndex);
        params.spriteIndex = fontManager.spriteAt(text.fontIndex);
        params.pixelCache.clear();

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
    Size size;

    while (it != end) {
        // Compute line metrics
        const auto metrics = ComputeLineMetrics<GetX, GetY>(params, it, end, GetY(size));

        // Compute line glyphs
        const auto lineWidth = ComputeLine<GetX, GetY>(
            out, params, it, it + metrics.charCount, metrics, GetY(size)
        );

        // Update caches
        params.pixelCache.push(lineWidth);
        it += metrics.charCount;
        GetX(size) = std::max(GetX(size), lineWidth);
        GetY(size) += params.lineHeight;

        // Stop on line elided
        if (metrics.elided) [[unlikely]]
            break;
    }

    // Position characters if any to draw
    if (begin != out) [[likely]]
        ComputeGlyphPositions<GetX, GetY>(begin, out, params, size);
}

template<auto GetX, auto GetY>
static UI::LineMetrics UI::ComputeLineMetrics(ComputeParameters &params,
        const std::string_view::iterator from, const std::string_view::iterator to, const Pixel yOffset) noexcept
{
    constexpr auto CheckFit = [](const auto &metrics, const auto textSize, const auto xFit, const auto size) {
        return !xFit | (metrics.totalSize + size <= GetX(textSize));
    };

    LineMetrics metrics;
    const auto spaceWidth = params.spaceWidth;
    const auto tabMultiplier = params.text->spacesPerTab - 1;
    const auto textSize = params.text->area.size;
    const bool lastLine = (yOffset + params.lineHeight * 2)
            > (GetY(params.text->area.pos) + GetY(params.text->area.size));
    const bool xFit = params.text->fit | params.text->elide;
    const bool xElide = params.text->elide & (lastLine | !params.text->fit);
    auto it = from;

    for (; it != to; ++it) {
        // Glyph
        if (!std::isspace(*it)) {
            const auto size = params.glyphUVs->at(params.glyphIndexSet->at(*it)).size.width;
            if (CheckFit(metrics, textSize, xFit, size)) [[likely]] {
                metrics.totalSize += size;
                metrics.totalGlyphSize += size;
            } else [[unlikely]] {
                metrics.elided = xElide & params.text->elide;
                break;
            }
        // Space
        } else if (const bool isTab = *it == '\t'; isTab | (*it == ' ')) {
            const auto spaceCount = 1.0f + tabMultiplier * static_cast<Pixel>(isTab);
            const auto size = spaceWidth * spaceCount;
            if (CheckFit(metrics, textSize, xFit, size)) [[likely]] {
                metrics.spaceCount += spaceCount;
                metrics.totalSize += size;
            } else [[unlikely]] {
                metrics.elided = xElide & params.text->elide;
                break;
            }
        // End of line
        } else {
            ++it;
            break;
        }
    }

    // Insert elided dots
    if (metrics.elided) [[unlikely]] {
        const auto elideSize = params.glyphUVs->at(params.glyphIndexSet->at('.')).size.width * ElideDotCount;
        while (!CheckFit(metrics, textSize, xFit, elideSize)) {
            // Decrement iterator if possible
            if (it != from)
                --it;
            else
                break;

            // Glyph
            if (!std::isspace(*it)) {
                const auto size = params.glyphUVs->at(params.glyphIndexSet->at(*it)).size.width;
                metrics.totalSize -= size;
                metrics.totalGlyphSize -= size;
            // Space
            } else if (const bool isTab = *it == '\t'; isTab | (*it == ' ')) {
                const auto spaceCount = 1.0f + tabMultiplier * static_cast<Pixel>(isTab);
                const auto size = spaceWidth * spaceCount;
                metrics.spaceCount += spaceCount;
                metrics.totalSize += size;
            // End of line
            } else {
                ++it;
                break;
            }
        }
    }

    // Compute character count
    metrics.charCount = Core::Distance<std::uint32_t>(from, it);
    return metrics;
}

template<auto GetX, auto GetY>
static UI::Pixel UI::ComputeLine(Glyph *&out, ComputeParameters &params,
        const std::string_view::iterator from, const std::string_view::iterator to,
        const LineMetrics &metrics, const Pixel yOffset) noexcept
{
    const auto spaceWidth = Core::BranchlessIf(
        params.text->textAlignment == TextAlignment::Justify,
        (GetX(params.text->area.size) - metrics.totalGlyphSize) / metrics.spaceCount,
        params.spaceWidth
    );
    const auto tabMultiplier = params.text->spacesPerTab - 1;
    Point pos;
    GetY(pos) = yOffset;

    for (auto it = from; it != to; ++it) {
        // Glyph
        if (!std::isspace(*it)) {
            const auto &uv = params.glyphUVs->at(params.glyphIndexSet->at(*it));
            new (out++) Glyph {
                .uv = uv,
                .pos = pos,
                .spriteIndex = params.spriteIndex,
                .color = params.text->color,
                .rotationAngle = params.text->rotationAngle,
                .vertical = static_cast<float>(params.text->vertical)
            };
            GetX(pos) += uv.size.width;
        // Space
        } else if (const bool isTab = *it == '\t'; isTab | (*it == ' ')) {
            const auto spaceCount = 1.0f + tabMultiplier * static_cast<Pixel>(isTab);
            GetX(pos) += spaceWidth * spaceCount;
        } else
            break;
    }

    if (metrics.elided) [[unlikely]] {
        const auto &uv = params.glyphUVs->at(params.glyphIndexSet->at('.'));
        for (auto i = 0u; i != ElideDotCount; ++i) {
            new (out++) Glyph {
                .uv = uv,
                .pos = pos,
                .spriteIndex = params.spriteIndex,
                .color = params.text->color,
                .rotationAngle = params.text->rotationAngle,
                .vertical = static_cast<float>(params.text->vertical)
            };
            GetX(pos) += uv.size.width;
        }
    }
    return GetX(pos);
}

template<auto GetX, auto GetY>
static void UI::ComputeGlyphPositions(Glyph * const from, Glyph * const to,
        const ComputeParameters &params, const Size metrics) noexcept
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