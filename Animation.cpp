/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Animation
 */

#include "Animation.hpp"

float UI::Animation::onTick(const std::int64_t elapsed) noexcept
{
    _elapsed = std::min(_elapsed + elapsed, _duration);
    if (_elapsed != _duration) [[likely]] {
        switch (_curve) {
        case Curve::Linear:
            return ComputeLinearCurve(_elapsed, 0, _duration);
        case Curve::Exponential:
            return ComputeExponentialCurve(_elapsed, 0, _duration);
        default:
            kFAbort("UI::Animation::onTick: Unkown curve type");
        }
    } else if (!_repeat) [[likely]] {
        return static_cast<float>(_inverted) - 1.0f;
    } else {
        _elapsed = 0;
        return 0.0f;
    }
}
