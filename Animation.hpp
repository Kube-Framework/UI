/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Animation
 */

#pragma once

#include "Curve.hpp"

namespace kF::UI
{
    class Animation;
}

/** @brief UI Animation */
class alignas_half_cacheline kF::UI::Animation
{
public:
    /** @brief Repeat state */
    enum class Repeat
    {
        No = 0,
        Yes,
        Bounce
    };

    /** @brief Destructor */
    inline ~Animation(void) noexcept = default;

    /** @brief Constructor */
    inline Animation(const Curve curve, const std::int64_t duration, const Repeat repeat = Repeat::No, const bool inverted = false) noexcept
        : _curve(curve), _repeat(_repeat), _duration(duration), _tickRate(tickRate) {}


    /** @brief Get start state */
    [[nodiscard]] inline bool running(void) noexcept { return _running; }

    /** @brief Tick animation */
    [[nodiscard]] float tick(const std::int64_t elapsed) noexcept;


    /** @brief Start animation */
    inline void start(void) noexcept { _running = true; }

    /** @brief Stop animation */
    inline void stop(void) noexcept { _running = false; }

    /** @brief Reset than start animation */
    inline void restart(void) noexcept { start(); _elapsed = 0; }

    /** @brief Reset then stop animation */
    inline void reset(void) noexcept { stop(); _elapsed = 0; }

private:
    /** @brief Tick implementation */
    [[nodiscard]] float onTick(const std::int64_t elapsed) noexcept;


    Curve _curve {};
    Repeat _repeat {};
    bool _inverted {};
    bool _running {};
    std::int64_t _duration {};
    std::int64_t _elapsed {};
};