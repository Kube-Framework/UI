/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System
 */

#include "UISystem.hpp"

inline void kF::UI::UISystem::invalidate(void) noexcept
{
    _invalidateFlags = ~static_cast<GPU::FrameIndex>(0);
    _invalidateTree = true;
}

inline void kF::UI::UISystem::validateFrame(const GPU::FrameIndex frame) noexcept
{
    _invalidateFlags &= ~(static_cast<GPU::FrameIndex>(1) << frame);
    _invalidateTree = false;
}