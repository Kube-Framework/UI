/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: GradientRectangle processor
 */

#pragma once

#include <Kube/UI/PrimitiveProcessor.hpp>
#include <Kube/UI/Sprite.hpp>

namespace kF::UI
{
    /** @brief GradientRectangle primitive
     *  @warning Must be compliant with std140 */
    struct GradientRectangle : public PrimitiveTag<"GradientRectangle">
    {
        Area area {}; // GradientRectangle area
        Radius radius {}; // Border radius in pixels
        SpriteIndex spriteIndex { NullSpriteIndex }; // Sprite index
        FillMode fillMode {}; // Sprite fill mode
        Color topLeftColor {}; // Top left color
        Color topRightColor {}; // Top right color
        Color bottomLeftColor {}; // Bottom left color
        Color bottomRightColor {}; // Bottom right color
        Color topLeftBorderColor {}; // Top left border color
        Color topRightBorderColor {}; // Top right border color
        Color bottomLeftBorderColor {}; // Bottom left border color
        Color bottomRightBorderColor {}; // Bottom right border color
        Pixel borderWidth {}; // Width of border color
        Pixel edgeSoftness {}; // Edge softness in pixels
        float rotationAngle {}; // Rotation in radians
        std::uint32_t _padding0;
        std::uint32_t _padding1;
        std::uint32_t _padding2;
    };

    namespace PrimitiveProcessor
    {
        /** @brief GradientRectangle processor query pipeline */
        template<>
        [[nodiscard]] consteval GraphicPipelineName QueryGraphicPipeline<GradientRectangle>(void) noexcept { return FilledQuadGraphicPipeline; }

        /** @brief GradientRectangle processor query model */
        template<>
        [[nodiscard]] PrimitiveProcessorModel QueryModel<GradientRectangle>(void) noexcept;
    }
}