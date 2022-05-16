#include "Animator.hpp"


inline bool kF::UI::Animator::tick(const std::int64_t elapsed) noexcept
{
    if (_states.empty()) [[likely]] {
        return false;
    } else {
        onTick(elapsed);
        return true;
    }
}