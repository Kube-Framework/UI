/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Renderer
 */

#include "Renderer.hpp"

template<kF::UI::PrimitiveKind Primitive>
inline void kF::UI::Renderer::registerPrimitive(void) noexcept
{
    registerPrimitive(Primitive::Hash, PrimitiveProcessor::QueryGraphicPipeline<Primitive>(), &PrimitiveProcessor::QueryModel<Primitive>);
}