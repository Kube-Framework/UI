/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: MouseFilter
 */

#pragma once

#include "Components.hpp"
#include "Events.hpp"

namespace kF::UI
{
    struct MouseFilter;
}

/** @brief Utility structure used to implement common mouse events easily
 *  @note Use the nested structures to custom the filter behavior (Click, Pen, Hover, Drag, DisableCursorChange & PropagateUnusedEvents).
 *  @example Simple action when a click has been released inside the item area:
 *  ```c++
 *  item.attach(
 *      MouseEventArea::Make<MouseFilter>(
 *          MouseFilter::Click { .released = [] { onReleased(); } }
 *      )
 *  );
 * ```
*/
struct kF::UI::MouseFilter
{
    /** @brief Action callback */
    using Callback = Core::Functor<void(const MouseEvent &event, const Area &area), UIAllocator>;

    /** @brief Hover callback
     *  @return Invalidates when 'true' is returned */
    using HoverCallback = Core::Functor<EventFlags(const MouseEvent &event, const Area &area), UIAllocator>;

    /** @brief Hover callback
     *  @return Invalidates when 'true' is returned */
    using HoverChangedCallback = Core::Functor<EventFlags(const bool entered), UIAllocator>;

    /** @brief Test callback */
    using TestCallback = Core::Functor<bool(const MouseEvent &event, const Area &area), UIAllocator>;

    /** @brief Click event (pressed / released) */
    struct Click
    {
        Button button { Button::Left };
        Modifier modifierWhiteList {};
        Modifier modifierBlackList {};
        Callback pressed {};
        Callback released {};
    };

    /** @brief Pen event (press and hold during motion) */
    struct Pen
    {
        Button button { Button::Left };
        Modifier modifierWhiteList {};
        Modifier modifierBlackList {};
        Callback pressed {};
        Callback motion {};
        Callback released {};
    };

    /** @brief Hover event (motion only) */
    struct Hover
    {
        HoverCallback hover {};
        HoverChangedCallback hoverChanged {};
    };

    /** @brief Drag event (press and hold during motion) */
    struct Drag
    {
        Button button { Button::Left };
        Modifier modifierWhiteList {};
        Modifier modifierBlackList {};
        Callback drag {};
        TestCallback testHit {};
    };

    /** @brief Prevent any automatic cursor change */
    struct DisableCursorChange {};

    /** @brief Will propagate every unused events */
    struct PropagateUnusedEvents {};


    /** @brief Filter a incoming event using list of nested structures (Click, Pen, Hover, Drag, DisableCursorChange, PropagateUnusedEvents) */
    template<typename ...Args>
        requires ((std::is_convertible_v<Args, const kF::UI::MouseFilter::Click &>
            || std::is_convertible_v<Args, const kF::UI::MouseFilter::Pen &>
            || std::is_convertible_v<Args, const kF::UI::MouseFilter::Hover &>
            || std::is_convertible_v<Args, const kF::UI::MouseFilter::Drag &>
            || std::is_convertible_v<Args, const kF::UI::MouseFilter::DisableCursorChange &>
            || std::is_convertible_v<Args, const kF::UI::MouseFilter::PropagateUnusedEvents &>
        ) && ...)
    [[nodiscard]] EventFlags operator()(
        const MouseEvent &event,
        const Area &area,
        const ECS::Entity entity,
        UISystem &uiSystem,
        const Args &...args
    ) const noexcept;

private:
    /** @brief To call before onEvent, return if the entity is locked */
    [[nodiscard]] bool onBeforeEvent(const MouseEvent &event, const ECS::Entity entity, UISystem &uiSystem, const bool allowCursor) const noexcept;

    /** @brief Set the lock state of target entity */
    void setLockState(const ECS::Entity entity, UISystem &uiSystem, const bool lock) const noexcept;


    /** @brief Handle click event */
    [[nodiscard]] EventFlags onEvent(
        const MouseEvent &event,
        const Area &area,
        const ECS::Entity entity,
        UISystem &uiSystem,
        const Click &click,
        bool &lock,
        const bool propagate
    ) const noexcept;

    /** @brief Handle pen event */
    [[nodiscard]] EventFlags onEvent(
        const MouseEvent &event,
        const Area &area,
        const ECS::Entity entity,
        UISystem &uiSystem,
        const Pen &pen,
        bool &lock,
        const bool propagate
    ) const noexcept;

    /** @brief Handle motion event */
    [[nodiscard]] EventFlags onEvent(
        const MouseEvent &event,
        const Area &area,
        const ECS::Entity entity,
        UISystem &uiSystem,
        const Hover &hover,
        bool &lock,
        const bool propagate
    ) const noexcept;

    /** @brief Handle drag event */
    [[nodiscard]] EventFlags onEvent(
        const MouseEvent &event,
        const Area &area,
        const ECS::Entity entity,
        UISystem &uiSystem,
        const Drag &drag,
        bool &lock,
        const bool propagate
    ) const noexcept;

    /** @brief Handle disable cursor change */
    [[nodiscard]] inline EventFlags onEvent(
        const MouseEvent &,
        const Area &,
        const ECS::Entity,
        UISystem &,
        const DisableCursorChange &,
        bool &,
        const bool
    ) const noexcept
        { return EventFlags::Propagate; }

    /** @brief Handle propagate unused events */
    [[nodiscard]] inline EventFlags onEvent(
        const MouseEvent &,
        const Area &,
        const ECS::Entity,
        UISystem &,
        const PropagateUnusedEvents &,
        bool &,
        const bool
    ) const noexcept
        { return EventFlags::Propagate; }

    /** @brief Merge flags */
    template<typename ...Flags>
        requires (std::is_same_v<Flags, kF::UI::EventFlags> && ...)
    [[nodiscard]] static EventFlags MergeFlags(const Flags ...flags) noexcept;
};

#include "MouseFilter.ipp"