/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Rectangle processor
 */

#pragma once

#include <Kube/UI/Base.hpp>
#include <Kube/UI/Sprite.hpp>
#include <Kube/UI/PrimitiveProcessor.hpp>

namespace kF::UI
{
    /** @brief Rectangle primitive */
    struct alignas_cacheline Rectangle : public PrimitiveTag<"Rectangle">
    {
        Area area {}; // Rectangle area
        Radius radius {}; // Border radius in pixels
        SpriteIndex spriteIndex { NullSpriteIndex }; // Sprite index
        FillMode fillMode {}; // Sprite fill mode
        Color color {}; // Fill color
        Color borderColor {}; // Border color
        Pixel borderWidth {}; // Width of border color
        Pixel edgeSoftness {}; // Edge softness in pixels
        float rotationAngle {}; // Rotation in radians
    };
    static_assert_fit_cacheline(Rectangle);

    namespace PrimitiveProcessor
    {
        /** @brief Rectangle processor query model */
        template<>
        [[nodiscard]] PrimitiveProcessorModel QueryModel<Rectangle>(void) noexcept;
    }
}