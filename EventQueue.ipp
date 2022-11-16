/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Event Queue
 */

#include "EventQueue.hpp"

template<kF::UI::EventRequirements EventType>
inline void kF::UI::EventQueue<EventType>::produce(const Range &range) noexcept
{
    while (!_queue.push(Batch { range.begin(), range.end() }));
}

template<kF::UI::EventRequirements EventType>
template<typename Functor>
inline void kF::UI::EventQueue<EventType>::consume(Functor &&functor) noexcept
{
    Batch batch;

    // Extract batch
    while (_queue.pop(batch)) {
        // Consume it
        functor(Range { batch.begin(), batch.end() });
    }
}