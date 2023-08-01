/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Animator
 */

#pragma once

#include <Kube/Core/SmallVector.hpp>
#include <Kube/Core/Expected.hpp>

#include "Base.hpp"
#include "Animation.hpp"

namespace kF::UI
{
    class Animator;
}


/** @brief UI Animator */
class alignas_cacheline kF::UI::Animator
{
public:
    /** @brief State of an animation */
    struct alignas_eighth_cacheline AnimationState
    {
        const Animation *animation {};
        std::int64_t elapsed {};
        std::uint32_t startCount {};
        bool reverse {};
    };
    static_assert_alignof_eighth_cacheline(AnimationState);
    static_assert_sizeof(AnimationState, Core::CacheLineEighthSize * 3);

    /** @brief Small optimized animation states */
    using AnimationStates = Core::SmallVector<AnimationState, 2, UIAllocator>;


    /** @brief Start an animation
     *  @note The animation pointer must be valid until animation stops */
    void start(const Animation &animation) noexcept;

    /** @brief Stop an animation
     *  @return Does nothing if the animation is not started */
    void stop(const Animation &animation) noexcept;

    /** @brief Check if an animation is running */
    [[nodiscard]] inline bool isRunning(const Animation &animation) const noexcept
        { return findIndex(animation).success(); }

public: // Unsafe public functions
    /** @brief Tick animator
     *  @return True if UI is invalidated */
    [[nodiscard]] inline bool tick(const std::int64_t elapsed) noexcept;

private:
    /** @brief Find an animation index */
    [[nodiscard]] Core::Expected<std::uint32_t> findIndex(const Animation &animation) const noexcept;

    /** @brief Tick implementation */
    void onTick(const std::int64_t elapsed) noexcept;


    AnimationStates _states {};
};
static_assert_fit_cacheline(kF::UI::Animator);

#include "Animator.ipp"
