/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System
 */

#include <Kube/GPU/GPU.hpp>

#include "PresentSystem.hpp"

using namespace kF;

UI::PresentSystem::PresentSystem(void) noexcept
{
}

bool UI::PresentSystem::tick(void) noexcept
{
    GPU::GPUObject::Parent().commandDispatcher().presentFrame();
    return false;
}