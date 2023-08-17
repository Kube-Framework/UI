/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Arc processor
 */

#pragma once

#include <Kube/UI/PrimitiveProcessor.hpp>

namespace kF::UI
{
    /** @brief Arc primitive
     *  @warning Must be compliant with std140 */
    struct alignas_quarter_cacheline Arc : public PrimitiveTag<"Arc">
    {
        Point center; // Arc center
        Pixel radius; // Arc radius
        Pixel thickness; // Arc thickness
        Pixel aperture; // Arc aperture
        Color color; // Arc inner color
        Color borderColor; // Arc border color
        Pixel borderWidth; // Arc border width
        Pixel edgeSoftness; // Arc edge softness
        float rotationAngle; // Arc rotation angle
    };
    static_assert_alignof_quarter_cacheline(Arc);
    static_assert_sizeof(Arc, Core::CacheLineQuarterSize * 3);

    namespace PrimitiveProcessor
    {
        /** @brief Arc processor query pipeline */
        template<>
        [[nodiscard]] consteval GraphicPipelineName QueryGraphicPipeline<Arc>(void) noexcept { return ArcGraphicPipeline; }

        /** @brief Arc processor query model */
        template<>
        [[nodiscard]] PrimitiveProcessorModel QueryModel<Arc>(void) noexcept;
    }
}