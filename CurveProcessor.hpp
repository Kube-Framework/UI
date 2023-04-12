/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Curve processor
 */

#pragma once

#include <Kube/UI/PrimitiveProcessor.hpp>
#include <Kube/UI/Sprite.hpp>

namespace kF::UI
{
    /** @brief Curve primitive
     *  @warning Must be compliant with std140 */
    struct alignas_quarter_cacheline Curve : public PrimitiveTag<"Curve">
    {
        Point left {}; // Curve left point
        Point right {}; // Curve right point
        Point control {}; // Curve control point
        Color color {}; // Fill color
        Pixel thickness {}; // Width of the line
        Pixel edgeSoftness {}; // Edge softness in pixels
        std::uint32_t _padding;
    };
    static_assert_sizeof(Curve, Core::CacheLineQuarterSize * 3);
    static_assert_alignof_quarter_cacheline(Curve);

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