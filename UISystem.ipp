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
inline void kF::UI::UISystem::drag(const Type &type, const Size &size, PainterArea &&painterArea) noexcept
{
    onDrag(TypeHash::Get<Type>(), &type, size, std::move(painterArea));
}
