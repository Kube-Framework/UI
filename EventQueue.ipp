/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Event Queue
 */

#include "EventQueue.hpp"

template<kF::UI::EventRequirements EventType>
inline kF::UI::EventQueue<EventType>::~EventQueue(void) noexcept
{
    constexpr auto DestroyList = [](auto &head) {
        while (true) {
            Node *nodePtr = ExtractNode(head);
            if (!nodePtr)
                break;
            nodePtr->~Node();
            EventAllocator::Deallocate(nodePtr, sizeof(Node), alignof(Node));
        }
    };

    DestroyList(_eventQueue);
    DestroyList(_eventFreeList);
}

template<kF::UI::EventRequirements EventType>
inline void kF::UI::EventQueue<EventType>::produce(const Range &range) noexcept
{
    // Get a node from free list
    Node *nodePtr = ExtractNode(_eventFreeList);

    // No node available, we must allocate
    if (!nodePtr)
        nodePtr = new (EventAllocator::Allocate(sizeof(Node), alignof(Node))) Node {};

    // Insert range into node
    nodePtr->batch.resize(range.begin(), range.end());

    // Insert node into queue
    InsertNode(_eventQueue, nodePtr);
}

template<kF::UI::EventRequirements EventType>
template<typename Functor>
inline void kF::UI::EventQueue<EventType>::consume(Functor &&functor) noexcept
{
    Node *consumedHead {};
    Node *consumedTail {};

    // Consume every node
    while (true) {
        // Acquire a queued node
        Node *nodePtr = ExtractNode(_eventQueue);

        // No node availabe to consume
        if (!nodePtr) [[likely]]
            break;

        // Consume it
        functor(Range { nodePtr->batch.begin(), nodePtr->batch.end() });

        // Insert node into consumed list
        nodePtr->next = consumedHead;
        consumedHead = nodePtr;

        // Only update consumed tail on the first entry
        if (!consumedTail) [[unlikely]]
            consumedTail = consumedHead;
    }

    if (!consumedTail) [[likely]]
        return;

    // Insert consumed nodes into the free list
    InsertNode(_eventFreeList, consumedTail);
}

template<kF::UI::EventRequirements EventType>
inline kF::UI::EventQueue<EventType>::Node *kF::UI::EventQueue<EventType>::ExtractNode(std::atomic<TaggedNodePtr> &head) noexcept
{
    TaggedNodePtr node { head.load(std::memory_order_acquire) };
    Node *nodePtr;

    while (true) {
        nodePtr = node.get();
        if (!nodePtr) [[likely]]
            break;
        const TaggedNodePtr top(nodePtr->next, node.tag() + 1);
        if (head.compare_exchange_weak(node, top, std::memory_order_acq_rel)) [[likely]]
            break;
    }
    return nodePtr;
}

template<kF::UI::EventRequirements EventType>
inline void kF::UI::EventQueue<EventType>::InsertNode(std::atomic<TaggedNodePtr> &head, Node * const nodePtr) noexcept
{
    auto node = head.load(std::memory_order_acquire);

    while (true) {
        nodePtr->next = node.get();
        const TaggedNodePtr top(nodePtr, node.tag() + 1);
        if (head.compare_exchange_weak(node, top, std::memory_order_acq_rel)) [[likely]]
            break;
    }
}
