/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Curve processor
 */

#include <Kube/IO/File.hpp>

#include "CurveProcessor.hpp"

using namespace kF;

template<>
UI::PrimitiveProcessorModel UI::PrimitiveProcessor::QueryModel<UI::Curve>(void) noexcept
{
    return PrimitiveProcessorModel {
        .computeShader = GPU::Shader(":/UI/Shaders/QuadraticBezier/Curve.comp.spv"),
        .computeLocalGroupSize = 64,
        .instanceSize = sizeof(Curve),
        .instanceAlignment = alignof(Curve),
        .verticesPerInstance = 4,
        .indicesPerInstance = 6
    };
}
