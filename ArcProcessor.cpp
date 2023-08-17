/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Arc processor
 */

#include <Kube/IO/File.hpp>

#include "ArcProcessor.hpp"

using namespace kF;

template<>
UI::PrimitiveProcessorModel UI::PrimitiveProcessor::QueryModel<UI::Arc>(void) noexcept
{
    return PrimitiveProcessorModel {
        .computeShader = GPU::Shader(":/UI/Shaders/Arc/Arc.comp.spv"),
        .computeLocalGroupSize = 64,
        .instanceSize = sizeof(Arc),
        .instanceAlignment = alignof(Arc),
        .verticesPerInstance = 4,
        .indicesPerInstance = 6
    };
}
