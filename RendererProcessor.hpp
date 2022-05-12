/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Renderer Processor
 */

#pragma once

#include "PrimitiveProcessor.hpp"

namespace kF::UI
{
    template<kF::UI::PrimitiveKind Primitive>
    class RendererProcessor;
}

template<kF::UI::PrimitiveKind Processor>
class kF::UI::RendererProcessor : public Processor
{
public:
    /** @brief Processor type */
    using Primitive = Processor;

    /** @brief Primitive type */
    using PrimitiveType = Processor::PrimitiveType;

private:

};