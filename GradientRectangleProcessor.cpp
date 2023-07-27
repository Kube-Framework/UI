/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: GradientRectangle processor
 */

#include <Kube/IO/File.hpp>

#include "GradientRectangleProcessor.hpp"

using namespace kF;

template<>
UI::PrimitiveProcessorModel UI::PrimitiveProcessor::QueryModel<UI::GradientRectangle>(void) noexcept
{
    return PrimitiveProcessorModel {
        .computeShader = GPU::Shader(IO::File(":/UI/Shaders/GradientRectangle.comp.spv").queryResource()),
        .computeLocalGroupSize = 1,
        .instanceSize = sizeof(GradientRectangle),
        .instanceAlignment = alignof(GradientRectangle),
        .verticesPerInstance = 4,
        .indicesPerInstance = 6
    };
}