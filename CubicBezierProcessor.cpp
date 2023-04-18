/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: CubicBezier processor
 */

#include <Kube/IO/File.hpp>

#include "CubicBezierProcessor.hpp"

using namespace kF;

template<>
UI::PrimitiveProcessorModel UI::PrimitiveProcessor::QueryModel<UI::CubicBezier>(void) noexcept
{
    return PrimitiveProcessorModel {
        .computeShader = GPU::Shader(IO::File(":/UI/Shaders/CubicBezier.comp.spv").queryResource()),
        .computeLocalGroupSize = 1,
        .instanceSize = sizeof(CubicBezier),
        .instanceAlignment = alignof(CubicBezier),
        .verticesPerInstance = 4,
        .indicesPerInstance = 6
    };
}
