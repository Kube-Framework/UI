/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Curve
 */

#pragma once

#include <Kube/Core/Utils.hpp>

namespace kF::UI
{
    /** @brief Curve types */
    enum class Curve
    {
        Linear,
        Exponential
    };

    /** @brief Linear curve */
    template<typename Input, typename Output = float>
    [[nodiscard]] Output ComputeLinearCurve(const Input value, const Input min, const Input max) noexcept;

    /** @brief Exponential curve */
    template<typename Input, typename Output = float>
    [[nodiscard]] Output ComputeExponentialCurve(const Input value, const Input min, const Input max) noexcept;
}

#include "Curve.ipp"