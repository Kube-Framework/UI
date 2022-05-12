/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Curve
 */

#include "Curve.hpp"

template<typename Input, typename Output>
inline Output kF::UI::ComputeLinearCurve(const Input value, const Input min, const Input max) noexcept
{
    return static_cast<Output>(value - min) / static_cast<Output>(max - min);
}

template<typename Input, typename Output>
inline Output ComputeExponentialCurve(const Input value, const Input min, const Input max) noexcept
{
    const auto x = ComputeLinearCurve(value, min, max);
    return x * x;
}