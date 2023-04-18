/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: CubicBezier processor
 */

#pragma once

#include <Kube/UI/PrimitiveProcessor.hpp>

namespace kF::UI
{
    /** @brief CubicBezier primitive
     *  @warning Must be compliant with std140 */
    struct alignas_cacheline CubicBezier : public PrimitiveTag<"CubicBezier">
    {
        Area area {}; // Render area
        Point p0 {}; // Cubic bezier point 0
        Point p1 {}; // Cubic bezier point 1
        Point p2 {}; // Cubic bezier point 2
        Point p3 {}; // Cubic bezier point 3
        Color color {}; // Fill color
        Pixel thickness {}; // Width of the line
        Pixel edgeSoftness {}; // Edge softness in pixels
        std::uint32_t _padding;
    };
    static_assert_fit_cacheline(CubicBezier);

    namespace PrimitiveProcessor
    {
        /** @brief CubicBezier processor query pipeline */
        template<>
        [[nodiscard]] consteval GraphicPipelineName QueryGraphicPipeline<CubicBezier>(void) noexcept { return CubicBezierGraphicPipeline; }

        /** @brief CubicBezier processor query model */
        template<>
        [[nodiscard]] PrimitiveProcessorModel QueryModel<CubicBezier>(void) noexcept;
    }
}