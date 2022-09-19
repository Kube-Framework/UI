/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System
 */

#include "UISystem.hpp"

inline void kF::UI::UISystem::invalidate(void) noexcept
{
    _cache.invalidateFlags = ~static_cast<GPU::FrameIndex>(0);
    _cache.invalidateTree = true;
}

inline void kF::UI::UISystem::validateFrame(const GPU::FrameIndex frame) noexcept
{
    _cache.invalidateFlags &= ~(static_cast<GPU::FrameIndex>(1) << frame);
    _cache.invalidateTree = false;
}

template<typename Type>
inline void kF::UI::UISystem::drag(const Type &type, const PainterArea &painterArea) noexcept
{
    _eventCache.drag.type = TypeHash::Get<Type>();
    _eventCache.drag.data = &type;
    _eventCache.drag.painterArea = painterArea;
}
