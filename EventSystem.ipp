/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Event system
 */

#include "EventSystem.hpp"

template<kF::UI::EventRequirements EventType>
inline kF::UI::EventQueuePtr<EventType> kF::UI::EventSystem::addEventQueue(void) noexcept
{
    if constexpr (std::is_same_v<EventType, MouseEvent>) {
        return _mouseQueues.push(EventQueuePtr<EventType>::Make());
    } else if constexpr (std::is_same_v<EventType, WheelEvent>) {
        return _wheelQueues.push(EventQueuePtr<EventType>::Make());
    } else if constexpr (std::is_same_v<EventType, KeyEvent>) {
        return _keyQueues.push(EventQueuePtr<EventType>::Make());
    } else if constexpr (std::is_same_v<EventType, TextEvent>) {
        return _textQueues.push(EventQueuePtr<EventType>::Make());
    }
}