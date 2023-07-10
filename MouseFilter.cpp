/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: MouseFilter
 */

#include "MouseFilter.hpp"
#include "App.hpp"
#include "UISystem.hpp"

using namespace kF;

namespace kF::UI
{
    /** @brief Try to match event */
    static inline bool MatchEvent(const MouseEvent &event, const Button button, const Modifier modifierWhiteList, const Modifier modifierBlackList) noexcept
    {
        return (event.button == button)
                & ((modifierWhiteList == UI::Modifier {}) | Core::HasAnyFlags(event.modifiers, modifierWhiteList))
                & !Core::HasAnyFlags(event.modifiers, modifierBlackList);
    }
}

void UI::MouseFilter::onBeforeEvent(const MouseEvent &event, UISystem &uiSystem) const noexcept
{
    switch (event.type) {
    case MouseEvent::Type::Enter:
        uiSystem.setCursor(Cursor::Hand);
        break;
    case MouseEvent::Type::Leave:
        uiSystem.setCursor(Cursor::Arrow);
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
    const bool propagate
) const noexcept
{
    switch (event.type) {
    case MouseEvent::Type::Press:
        if (MatchEvent(event, click.button, click.modifierWhiteList, click.modifierBlackList)) {
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
    default:
        break;
    }
    return propagate ? EventFlags::Propagate : EventFlags::Stop;
}

UI::EventFlags UI::MouseFilter::onEvent(
    const MouseEvent &event,
    const Area &area,
    const ECS::Entity entity,
    UISystem &uiSystem,
    const Pen &pen,
    bool &lock,
    const bool propagate
) const noexcept
{
    switch (event.type) {
    case MouseEvent::Type::Motion:
        if (uiSystem.lockedEntity<MouseEventArea>() == entity && Core::HasFlags(event.activeButtons, pen.button)) {
            if (pen.motion)
                pen.motion(event, area);
            lock = true;
            return propagate ? EventFlags::InvalidateAndPropagate : EventFlags::Invalidate;
        }
        break;
    case MouseEvent::Type::Press:
        if (MatchEvent(event, pen.button, pen.modifierWhiteList, pen.modifierBlackList)) {
            if (pen.pressed)
                pen.pressed(event, area);
            lock = true;
            return EventFlags::Invalidate;
        }
        break;
    case MouseEvent::Type::Release:
        if (uiSystem.lockedEntity<MouseEventArea>() == entity && event.button == pen.button) {
            if (pen.released)
                pen.released(event, area);
            lock = true;
            return EventFlags::Invalidate;
        }
        break;
    default:
        break;
    }
    return propagate ? EventFlags::Propagate : EventFlags::Stop;
}

UI::EventFlags UI::MouseFilter::onEvent(
    const MouseEvent &event,
    const Area &area,
    const ECS::Entity,
    UISystem &,
    const Hover &hover,
    bool &,
    const bool propagate
) const noexcept
{
    switch (event.type) {
    case MouseEvent::Type::Motion:
        if (hover.hover)
            return hover.hover(event, area);
        else
            return UI::EventFlags::Stop;
    case MouseEvent::Type::Enter:
    case MouseEvent::Type::Leave:
        if (hover.hoverChanged)
            return hover.hoverChanged(event.type == MouseEvent::Type::Enter);
        break;
    default:
        break;
    }
    return propagate ? EventFlags::Propagate : EventFlags::Stop;
}

UI::EventFlags UI::MouseFilter::onEvent(
    const MouseEvent &event,
    const Area &area,
    const ECS::Entity entity,
    UISystem &uiSystem,
    const Drag &drag,
    bool &lock,
    const bool propagate
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
    case MouseEvent::Type::Press:
        if (MatchEvent(event, drag.button, drag.modifierWhiteList, drag.modifierBlackList)) {
            lock = !drag.testHit || drag.testHit(event, area);
            return EventFlags::Invalidate;
        }
        break;
    default:
        break;
    }
    return propagate ? EventFlags::Propagate : EventFlags::Stop;
}