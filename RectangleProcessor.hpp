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
        Area area {};
        Radius radius {};
        SpriteIndex spriteIndex { NullSpriteIndex };
        FillMode fillMode {};
        Color color {};
        Color borderColor {};
        Pixel borderWidth {};
        Pixel edgeSoftness {};
    };
    static_assert_fit_cacheline(Rectangle);

    namespace PrimitiveProcessor
    {
        /** @brief Rectangle processor query model */
        template<>
        [[nodiscard]] PrimitiveProcessorModel QueryModel<Rectangle>(void) noexcept;
    }
}