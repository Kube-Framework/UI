/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Rectangle processor
 */

#include <Kube/IO/File.hpp>

#include "RectangleProcessor.hpp"

using namespace kF;

template<>
UI::PrimitiveProcessorModel UI::PrimitiveProcessor::QueryModel<UI::Rectangle>(void) noexcept
{
    return PrimitiveProcessorModel {
        .computeShader = GPU::Shader(IO::File(":/UI/Shaders/Rectangle.comp.spv").queryResource()),
        .computeLocalGroupSize = 1,
        .instanceSize = sizeof(Rectangle),
        .instanceAlignment = alignof(Rectangle),
        .verticesPerInstance = 4,
        .indicesPerInstance = 6
    };
}