/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: MouseFilter
 */

#include "MouseFilter.hpp"
#include "App.hpp"
#include "UISystem.hpp"

using namespace kF;

void UI::MouseFilter::onBeforeEvent(const MouseEvent &event) const noexcept
{
    switch (event.type) {
    case MouseEvent::Type::Enter:
        App::Get().setCursor(Cursor::Hand);
        break;
    case MouseEvent::Type::Leave:
        App::Get().setCursor(Cursor::Arrow);
        break;
    default:
        break;
    }
}

void UI::MouseFilter::onAfterEvent(const ECS::Entity entity, UISystem &uiSystem, const bool lock) const noexcept
{
    if (!lock)
        uiSystem.unlockEvents<MouseEventArea>(entity);
    else if (uiSystem.exists<MouseEventArea>(entity))
        uiSystem.lockEvents<MouseEventArea>(entity);
}

UI::EventFlags UI::MouseFilter::onEvent(
    const MouseEvent &event,
    const Area &area,
    const ECS::Entity,
    UISystem &,
    const Click &click,
    bool &,
    const bool propagateMotion
) const noexcept
{
    switch (event.type) {
    case MouseEvent::Type::Motion:
        return propagateMotion ? EventFlags::Propagate : EventFlags::Stop;
    case MouseEvent::Type::Enter:
    case MouseEvent::Type::Leave:
        return propagateMotion ? EventFlags::InvalidateAndPropagate : EventFlags::Invalidate;
    case MouseEvent::Type::Press:
        if (event.button == click.button) {
            if (click.pressed)
                click.pressed(event, area);
            return EventFlags::Invalidate;
        }
        break;
    case MouseEvent::Type::Release:
        if (event.button == click.button) {
            if (click.released)
                click.released(event, area);
            return EventFlags::Invalidate;
        }
        break;
    }
    return EventFlags::Stop;
}

UI::EventFlags UI::MouseFilter::onEvent(
    const MouseEvent &event,
    const Area &area,
    const ECS::Entity entity,
    UISystem &uiSystem,
    const Pen &pen,
    bool &lock,
    const bool propagateMotion
) const noexcept
{
    switch (event.type) {
    case MouseEvent::Type::Motion:
        if (uiSystem.lockedEntity<MouseEventArea>() == entity && Core::HasFlags(event.activeButtons, pen.button)) {
            if (pen.motion)
                pen.motion(event, area);
            lock = true;
            return propagateMotion ? EventFlags::InvalidateAndPropagate : EventFlags::Invalidate;
        }
        break;
    case MouseEvent::Type::Enter:
    case MouseEvent::Type::Leave:
        return propagateMotion ? EventFlags::Propagate : EventFlags::Stop;
    case MouseEvent::Type::Press:
        if (event.button == pen.button) {
            if (pen.pressed)
                pen.pressed(event, area);
            lock = true;
            return EventFlags::Invalidate;
        }
        break;
    case MouseEvent::Type::Release:
        if (event.button == pen.button) {
            if (pen.released)
                pen.released(event, area);
            lock = true;
            return EventFlags::Invalidate;
        }
        break;
    default:
        break;
    }
    return EventFlags::Stop;
}

UI::EventFlags UI::MouseFilter::onEvent(
    const MouseEvent &event,
    const Area &area,
    const ECS::Entity,
    UISystem &,
    const Hover &hover,
    bool &,
    const bool
) const noexcept
{
    switch (event.type) {
    case MouseEvent::Type::Motion:
        return hover.hover ? hover.hover(event, area) : EventFlags::Stop;
    case MouseEvent::Type::Enter:
        return hover.hoverChanged ? hover.hoverChanged(true) : EventFlags::Stop;
    case MouseEvent::Type::Leave:
        return hover.hoverChanged ? hover.hoverChanged(false) : EventFlags::Stop;
    default:
        return EventFlags::Stop;
    }
}

UI::EventFlags UI::MouseFilter::onEvent(
    const MouseEvent &event,
    const Area &area,
    const ECS::Entity entity,
    UISystem &uiSystem,
    const Drag &drag,
    bool &lock,
    const bool propagateMotion
) const noexcept
{
    switch (event.type) {
    case MouseEvent::Type::Motion:
        if (uiSystem.lockedEntity<MouseEventArea>() == entity && Core::HasFlags(event.activeButtons, drag.button)) {
            uiSystem.unlockEvents<MouseEventArea>();
            if (drag.drag)
                drag.drag(event, area);
            return EventFlags::Invalidate;
        }
        break;
    case MouseEvent::Type::Enter:
    case MouseEvent::Type::Leave:
        return propagateMotion ? EventFlags::Propagate : EventFlags::Stop;
    case MouseEvent::Type::Press:
        if (event.button == drag.button) {
            lock = !drag.testHit || drag.testHit(event, area);
            return EventFlags::Invalidate;
        }
        break;
    default:
        break;
    }
    return EventFlags::Stop;
}
