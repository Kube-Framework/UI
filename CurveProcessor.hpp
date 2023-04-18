/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Curve processor
 */

#pragma once

#include <Kube/UI/PrimitiveProcessor.hpp>

namespace kF::UI
{
    /** @brief Curve primitive
     *  @warning Must be compliant with std140 */
    struct alignas_cacheline Curve : public PrimitiveTag<"Curve">
    {
        Area area {}; // Render area
        Point left {}; // Curve left point
        Point control {}; // Curve control point
        Point right {}; // Curve right point
        Color color {}; // Fill color
        Pixel thickness {}; // Width of the line
        Pixel edgeSoftness {}; // Edge softness in pixels
        std::uint32_t _padding;
    };
    static_assert_fit_cacheline(Curve);

    namespace PrimitiveProcessor
    {
        /** @brief Curve processor query pipeline */
        template<>
        [[nodiscard]] consteval GraphicPipelineName QueryGraphicPipeline<Curve>(void) noexcept { return QuadraticBezierGraphicPipeline; }

        /** @brief Curve processor query model */
        template<>
        [[nodiscard]] PrimitiveProcessorModel QueryModel<Curve>(void) noexcept;
    }
}