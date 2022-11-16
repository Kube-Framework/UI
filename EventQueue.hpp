/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Event Queue
 */

#pragma once

#include <Kube/Core/SharedPtr.hpp>
#include <Kube/Core/SPSCQueue.hpp>
#include <Kube/Core/FlatVector.hpp>

#include "Events.hpp"

namespace kF::UI
{
    /** @brief Event requirementes */
    template<typename EventType>
    concept EventRequirements = std::same_as<EventType, MouseEvent>
            || std::same_as<EventType, WheelEvent>
            || std::same_as<EventType, KeyEvent>;

    template<kF::UI::EventRequirements EventType>
    class EventQueue;

    class EventSystem;

    /** @brief Shared pointer to event queue */
    template<EventRequirements EventType>
    using EventQueuePtr = Core::SharedPtr<EventQueue<EventType>, EventAllocator>;
}

/** @brief MPMC event queue bound to a specific event type */
template<kF::UI::EventRequirements EventType>
class kF::UI::EventQueue
{
public:
    /** @brief Event range */
    using Range = Core::IteratorRange<const EventType *>;

    /** @brief Event batch */
    using Batch = Core::FlatVector<EventType, EventAllocator>;


    /** @brief Destructor */
    ~EventQueue(void) noexcept = default;

    /** @brief Constructor */
    EventQueue(void) noexcept = default;


    /** @brief Insert a range of events into the queue */
    void produce(const Range &range) noexcept;

    /** @brief Consume events of the queue */
    template<typename Functor>
    void consume(Functor &&functor) noexcept;


private:
    Core::SPSCQueue<Batch, UIAllocator> _queue { 4096 / sizeof(Batch) };
};

#include "EventQueue.ipp"
