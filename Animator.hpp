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
class kF::UI::Animator
{
public:
    /** @brief State of an animation */
    struct alignas_quarter_cacheline AnimationState
    {
        Core::TaggedPtr<const Animation> animation {};
        std::int64_t elapsed {};

        /** @brief Get reverse state */
        [[nodiscard]] inline bool reverse(void) const noexcept { return static_cast<bool>(animation.tag()); }

        /** @brief Set reverse state */
        inline void setReverse(const bool value) noexcept { animation.setTag(value); }
    };
    static_assert_fit_quarter_cacheline(AnimationState);

    /** @brief Compute the optimized value count */
    static constexpr auto OptimizedCount = Core::CacheLineQuarterSize / sizeof(AnimationState);
    static_assert(OptimizedCount != 0, "Invalid small optimization");

    /** @brief Small optimized animation states */
    using AnimationStates = Core::SmallVector<AnimationState, OptimizedCount, UIAllocator>;

    ~Animator(void) noexcept { if (_guard) *_guard = false; }


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
    bool *_guard {};
};

#include "Animator.ipp"
