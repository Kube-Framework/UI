/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Event Queue
 */

#pragma once

#include <Kube/Core/SharedPtr.hpp>

#include "Events.hpp"

namespace kF::UI
{
    /** @brief Event requirementes */
    template<typename EventType>
    concept EventRequirements = std::same_as<EventType, MouseEvent>
            || std::same_as<EventType, MotionEvent>
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
class alignas_quarter_cacheline kF::UI::EventQueue
{
public:
    /** @brief Linked list node */
    struct alignas_half_cacheline Node
    {
        Core::Vector<EventType, EventAllocator> batch {};
        Node *next {};
    };
    static_assert_fit_half_cacheline(Node);

    /** @brief Tagged node */
    using TaggedNodePtr = Core::TaggedPtr<Node>;

    /** @brief Event range */
    using Range = Core::IteratorRange<const EventType *>;


    /** @brief Destructor */
    ~EventQueue(void) noexcept;

    /** @brief Constructor */
    EventQueue(void) noexcept = default;


    /** @brief Insert a range of events into the queue */
    void produce(const Range &range) noexcept;

    /** @brief Consume events of the queue */
    template<typename Functor>
        requires std::is_invocable_v<Functor, const kF::UI::EventQueue<EventType>::Range &>
    void consume(Functor &&functor) noexcept;


private:
    /** @brief Extract a node from an atomic list */
    [[nodiscard]] static inline Node *ExtractNode(std::atomic<TaggedNodePtr> &head) noexcept;

    /** @brief Insert a node into an atomic list */
    static inline void InsertNode(std::atomic<TaggedNodePtr> &head, Node * const nodePtr) noexcept;

    std::atomic<TaggedNodePtr> _eventQueue {};
    std::atomic<TaggedNodePtr> _eventFreeList {};
};

#include "EventQueue.ipp"