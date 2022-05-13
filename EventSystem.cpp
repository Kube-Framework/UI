/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Event system
 */

#include <SDL2/SDL_events.h>

#include <Kube/GPU/GPU.hpp>
#include <Kube/ECS/Executor.hpp>

#include "EventSystem.hpp"
#include "PresentPipeline.hpp"

using namespace kF;

UI::EventSystem::EventSystem(void) noexcept
{
}

bool UI::EventSystem::tick(void) noexcept
{
    // Clear all caches
    _mouseEvents.clear();
    _motionEvents.clear();
    _keyEvents.clear();

    // Collect all events
    collectEvents();

    // Dispatch all collected events
    dispatchEvents();

    return false;
}

void UI::EventSystem::collectEvents(void) noexcept
{
    SDL_Event events[16];

    // ::SDL_PumpEvents();
    while (true) {
        const auto count = ::SDL_PeepEvents(
            events,
            static_cast<int>(std::size(events)),
            SDL_GETEVENT,
            SDL_FIRSTEVENT,
            SDL_LASTEVENT
        );
        if (count == 0) [[likely]]
            break;
        else if (count < 0) [[unlikely]] {
            kFError("UI::EventSystem::tick: Couldn't retreive events '", SDL_GetError(), '\'');
            break;
        }
        for (auto i = 0; i != count; ++i)
            interpretEvent(events[i]);
    }
}

void UI::EventSystem::interpretEvent(const SDL_Event &event) noexcept
{
    switch (event.type) {
    case SDL_WINDOWEVENT:
        switch (event.window.event) {
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
            const GPU::Extent2D extent { static_cast<std::uint32_t>(event.window.data1), static_cast<std::uint32_t>(event.window.data2) };
            if (_resizeExtent.width == extent.width && _resizeExtent.height == extent.height)
                break;
            _resizeExtent = extent;
            kFInfo("UI::EventSystem::tick: Window resized: ", extent.width, ", ", extent.height);
            parent().sendEvent<PresentPipeline>([] { GPU::GPUObject::Parent().dispatchViewSizeChanged(); });
            break;
        }
        default:
            break;
        }
        break;
    case SDL_MOUSEMOTION:
        _motionEvents.push(MotionEvent {
            .pos = Point(static_cast<Pixel>(event.motion.x), static_cast<Pixel>(event.motion.y)),
            .motion = Point(static_cast<Pixel>(event.motion.xrel), static_cast<Pixel>(event.motion.yrel)),
            .buttons = static_cast<Button>(event.motion.state),
            .modifiers = Modifier::None,
            .timestamp = event.motion.timestamp
        });
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        _mouseEvents.push(MouseEvent {
            .pos = Point(static_cast<Pixel>(event.button.x), static_cast<Pixel>(event.button.y)),
            .button = static_cast<Button>(1u << (event.button.button - 1)),
            .state = static_cast<bool>(event.button.state),
            .modifiers = Modifier::None,
            .timestamp = event.button.timestamp
        });
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        _keyEvents.push(KeyEvent {
            .key = static_cast<Key>(event.key.keysym.sym),
            .modifiers = Modifier::None,
            .state = static_cast<bool>(event.key.state),
            .repeat = static_cast<bool>(event.key.repeat),
            .timestamp = event.key.timestamp
        });
        break;
    case SDL_QUIT:
        parent().stop();
        break;
    default:
        break;
    }
}

void kF::UI::EventSystem::dispatchEvents(void) noexcept
{
    const auto dispatch = [this](auto &queues, const auto &events) {
        // No events to dispatch
        if (events.empty()) [[likely]]
            return;

        // Dispatch each queue and mark unused
        const auto it = std::remove_if(queues.begin(), queues.end(), [this, &events](auto &queue) {
            if (queue.referenceCount() != 1) [[likely]] {
                queue->produce(Core::IteratorRange { events.begin(), events.end() });
                return false;
            } else [[unlikely]]
                return true;
        });

        // Erase all unused queues
        if (it != queues.end()) [[unlikely]]
            queues.erase(it, queues.end());
    };

    dispatch(_mouseQueues, _mouseEvents);
    dispatch(_motionQueues, _motionEvents);
    dispatch(_keyQueues, _keyEvents);
}