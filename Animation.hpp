/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Animation
 */

#pragma once

#include <Kube/Core/Functor.hpp>

#include "Base.hpp"

namespace kF::UI
{
    /** @brief Animation repeat state */
    enum class AnimationMode
    {
        Single,
        Repeat,
        Bounce
    };

    /** @brief Animation event */
    enum class AnimationStatus
    {
        Start,
        Stop,
        Finish
    };

    /** @brief UI Animation */
    struct alignas_cacheline Animation
    {
        /** @brief Tick event */
        using TickEvent = Core::Functor<void(const float), UIAllocator, Core::CacheLineEighthSize * 3>;

        /** @brief Status event */
        using StatusEvent = Core::Functor<void(const AnimationStatus), UIAllocator, Core::CacheLineEighthSize * 3>;


        std::int64_t duration {};
        AnimationMode animationMode { AnimationMode::Single };
        bool reverse {};
        TickEvent tickEvent {};
        StatusEvent statusEvent {};
    };
    static_assert_fit_cacheline(Animation);
}