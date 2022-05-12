/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Animation
 */

#include "Animation.hpp"

inline float kF::UI::Animation::tick(const std::int64_t elapsed) noexcept
{
    if (_running) [[likely]]
        return onTick(elapsed);
    else
        return 0.0f;
}
