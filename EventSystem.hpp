/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Event system
 */

#pragma once

#include <Kube/ECS/System.hpp>

#include "EventQueue.hpp"

union SDL_Event;

namespace kF::UI
{
    class EventSystem;

    /** @brief Event pipeline, encapsulate event systems */
    using EventPipeline = ECS::PipelineTag<"EventPipeline">;
}

/** @brief Event system, responsible for user input dispatching */
class alignas_double_cacheline kF::UI::EventSystem : public ECS::System<"EventSystem", EventPipeline>
{
public:
    /** @brief Virtual destructor */
    virtual ~EventSystem(void) noexcept = default;

    /** @brief Constructor */
    EventSystem(void) noexcept;


    /** @brief Virtual tick callback */
    [[nodiscard]] virtual bool tick(void) noexcept override;


    /** @brief Add an event queue of a specific event type */
    template<kF::UI::EventRequirements EventType>
    [[nodiscard]] EventQueuePtr<EventType> addEventQueue(void) noexcept;


private:
    /** @brief Collect all events */
    void collectEvents(void) noexcept;

    /** @brief Interpret a single event */
    void interpretEvent(const SDL_Event &event) noexcept;

    /** @brief Dispatch all collected events */
    void dispatchEvents(void) noexcept;


    // Cacheline N
    Core::TinyVector<MouseEvent, EventAllocator> _mouseEvents {};
    Core::TinyVector<MotionEvent, EventAllocator> _motionEvents {};
    Core::TinyVector<KeyEvent, EventAllocator> _keyEvents {};
    Core::TinyVector<EventQueuePtr<MouseEvent>, EventAllocator> _mouseQueues {};
    // Cacheline N + 1
    Core::TinyVector<EventQueuePtr<MotionEvent>, EventAllocator> _motionQueues {};
    Core::TinyVector<EventQueuePtr<KeyEvent>, EventAllocator> _keyQueues {};
    GPU::Extent2D _resizeExtent {};
};

#include "EventSystem.ipp"