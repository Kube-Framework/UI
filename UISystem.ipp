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

template<kF::UI::LockComponentRequirements Component>
inline void kF::UI::UISystem::lockEvents(const ECS::Entity entity) noexcept
{
    auto &target = [this] -> ECS::Entity & {
        if constexpr (std::is_same_v<Component, kF::UI::MouseEventArea>)
            return _eventCache.mouseLock;
        else if constexpr (std::is_same_v<Component, kF::UI::WheelEventArea>)
            return _eventCache.wheelLock;
        else if constexpr (std::is_same_v<Component, kF::UI::DropEventArea>)
            return _eventCache.dropLock;
        else if constexpr (std::is_same_v<Component, kF::UI::KeyEventReceiver>)
            return _eventCache.keyLock;
    }();

    target = entity;
}

template<kF::UI::LockComponentRequirements Component>
inline void kF::UI::UISystem::unlockEvents(const ECS::Entity entity) noexcept
{
    auto &target = [this] -> ECS::Entity & {
        if constexpr (std::is_same_v<Component, kF::UI::MouseEventArea>)
            return _eventCache.mouseLock;
        else if constexpr (std::is_same_v<Component, kF::UI::WheelEventArea>)
            return _eventCache.wheelLock;
        else if constexpr (std::is_same_v<Component, kF::UI::DropEventArea>)
            return _eventCache.dropLock;
        else if constexpr (std::is_same_v<Component, kF::UI::KeyEventReceiver>)
            return _eventCache.keyLock;
    }();

    if (target == entity)
        target = ECS::NullEntity;
}


template<typename ...Components>
inline void kF::UI::UISystem::onDettach(const ECS::Entity entity) noexcept
{
    if constexpr ((std::is_same_v<Components, MouseEventArea> || ...))
        onMouseEventAreaRemovedUnsafe(entity);
    if constexpr ((std::is_same_v<Components, WheelEventArea> || ...))
        onWheelEventAreaRemovedUnsafe(entity);
    if constexpr ((std::is_same_v<Components, DropEventArea> || ...))
        onDropEventAreaRemovedUnsafe(entity);
    if constexpr ((std::is_same_v<Components, KeyEventReceiver> || ...))
        onKeyEventReceiverRemovedUnsafe(entity);
}

inline void kF::UI::UISystem::onMouseEventAreaRemovedUnsafe(const ECS::Entity entity) noexcept
{
    // Check if deleted entity is currently locked
    if (_eventCache.mouseLock == entity)
        _eventCache.mouseLock = ECS::NullEntity;

    // Check if deleted entity is currently hovered
    const auto it = _eventCache.mouseHoveredEntities.find(entity);
    if (it != _eventCache.mouseHoveredEntities.end())
        _eventCache.mouseHoveredEntities.erase(it);
}

inline void kF::UI::UISystem::onWheelEventAreaRemovedUnsafe(const ECS::Entity entity) noexcept
{
    // Check if deleted entity is currently locked
    if (_eventCache.wheelLock == entity)
        _eventCache.wheelLock = ECS::NullEntity;
}

inline void kF::UI::UISystem::onDropEventAreaRemovedUnsafe(const ECS::Entity entity) noexcept
{
    if (!isDragging())
        return;

    // Check if deleted entity is currently locked
    if (_eventCache.dropLock == entity)
        _eventCache.dropLock = ECS::NullEntity;

    // Check if deleted entity is currently hovered
    const auto it = _eventCache.dropHoveredEntities.find(entity);
    if (it != _eventCache.dropHoveredEntities.end())
        _eventCache.dropHoveredEntities.erase(it);
}

inline void kF::UI::UISystem::onKeyEventReceiverRemovedUnsafe(const ECS::Entity entity) noexcept
{
    // Check if deleted entity is currently locked
    if (_eventCache.keyLock == entity)
        _eventCache.keyLock = ECS::NullEntity;
}