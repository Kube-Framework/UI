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
        .computeShader = GPU::Shader(IO::File(":/UI/Shaders/Curve.comp.spv").queryResource()),
        .computeLocalGroupSize = 1,
        .instanceSize = sizeof(Curve),
        .instanceAlignment = alignof(Curve),
        .verticesPerInstance = 3,
        .indicesPerInstance = 3
    };
}
