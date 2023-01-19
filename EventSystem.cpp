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
    _wheelEvents.clear();
    _keyEvents.clear();
    _textEvents.clear();

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
    case SDL_QUIT:
        parent().stop();
        break;
    case SDL_WINDOWEVENT:
        switch (event.window.event) {
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
            const GPU::Extent2D extent { static_cast<std::uint32_t>(event.window.data1), static_cast<std::uint32_t>(event.window.data2) };
            if (_resizeExtent.width == extent.width && _resizeExtent.height == extent.height)
                break;
            _resizeExtent = extent;
            kFInfo("[UI] Window resized: ", extent.width, ", ", extent.height);
            parent().sendEvent<PresentPipeline>([] { GPU::GPUObject::Parent().dispatchViewSizeChanged(); });
            break;
        }
        default:
            break;
        }
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        _modifiers = static_cast<Modifier>(SDL_GetModState());
        _keyEvents.push(KeyEvent {
            .key = static_cast<Key>(event.key.keysym.sym),
            .modifiers = _modifiers,
            .state = static_cast<bool>(event.key.state),
            .repeat = static_cast<bool>(event.key.repeat),
            .timestamp = event.key.timestamp
        });
        break;
    case SDL_TEXTINPUT:
        _textEvents.push(TextEvent {
            .text = std::string_view(reinterpret_cast<const char *>(event.edit.text)),
            .timestamp = event.edit.timestamp
        });
        break;
    case SDL_MOUSEMOTION:
    {
        const Point mousePos(static_cast<Pixel>(event.motion.x), static_cast<Pixel>(event.motion.y));
        _mouseEvents.push(MouseEvent {
            .pos = mousePos,
            .motion = mousePos - _lastMousePosition,
            .type = MouseEvent::Type::Motion,
            .activeButtons = static_cast<Button>(event.motion.state),
            .modifiers = _modifiers,
            .timestamp = event.motion.timestamp
        });
        _lastMousePosition = mousePos;
        break;
    }
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        _mouseEvents.push(MouseEvent {
            .pos = Point(static_cast<Pixel>(event.button.x), static_cast<Pixel>(event.button.y)),
            .type = event.button.state ? MouseEvent::Type::Press : MouseEvent::Type::Release,
            .button = static_cast<Button>(1u << (event.button.button - 1)),
            .activeButtons = static_cast<Button>(SDL_GetMouseState(nullptr, nullptr)),
            .modifiers = _modifiers,
            .timestamp = event.button.timestamp
        });
        break;
    case SDL_MOUSEWHEEL:
        _wheelEvents.push(WheelEvent {
            .pos = _lastMousePosition,
            .offset = Point(event.wheel.preciseX, event.wheel.preciseY),
            .modifiers = _modifiers,
            .timestamp = event.wheel.timestamp
        });
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
    dispatch(_wheelQueues, _wheelEvents);
    dispatch(_keyQueues, _keyEvents);
    dispatch(_textQueues, _textEvents);
}