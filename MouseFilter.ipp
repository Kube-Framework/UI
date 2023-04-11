/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: MouseFilter
 */

#include "MouseFilter.hpp"

template<typename ...Args>
    requires ((std::is_convertible_v<Args, const kF::UI::MouseFilter::Click &
    >
        || std::is_convertible_v<Args, const kF::UI::MouseFilter::Pen &>
        || std::is_convertible_v<Args, const kF::UI::MouseFilter::Hover &>
        || std::is_convertible_v<Args, const kF::UI::MouseFilter::Drag &>
        || std::is_convertible_v<Args, const kF::UI::MouseFilter::DisableCursorChange &>
        || std::is_convertible_v<Args, const kF::UI::MouseFilter::PropagateUnusedEvents &>
    ) && ...)
inline kF::UI::EventFlags kF::UI::MouseFilter::operator()(
    const MouseEvent &event,
    const Area &area,
    const ECS::Entity entity,
    UISystem &uiSystem,
    const Args &...args
) const noexcept
{
    constexpr bool Propagate = ((std::is_same_v<Args, Hover> || std::is_same_v<Args, PropagateUnusedEvents>) || ...);
    constexpr bool AllowCursor = (!std::is_same_v<Args, DisableCursorChange> && ...);

    if constexpr (AllowCursor)
        onBeforeEvent(event, uiSystem);
    bool lock {};
    auto flags = MergeFlags(onEvent(event, area, entity, uiSystem, args, lock, Propagate)...);
    onAfterEvent(entity, uiSystem, lock);

    // Add invalidate on enter / leave when 'Hover' is not declared
    if constexpr (!Propagate) {
        if (event.type == MouseEvent::Type::Enter || event.type == MouseEvent::Type::Leave)
            flags = Core::MakeFlags(flags, EventFlags::Invalidate);
    }
    return flags;
}

template<typename ...Flags>
    requires (std::is_same_v<Flags, kF::UI::EventFlags> && ...)
inline kF::UI::EventFlags kF::UI::MouseFilter::MergeFlags(const Flags ...flags) noexcept
{
    if constexpr (sizeof...(Flags) == 1) {
        return (flags, ...);
    } else {
        EventFlags result {};
        // Is any invalidate ?
        if (Core::HasAnyFlags(EventFlags::Invalidate, flags...))
            result = Core::MakeFlags(result, EventFlags::Invalidate);
        // Is all propagate ?
        if ((Core::HasFlags(flags, EventFlags::Propagate) && ...))
            result = Core::MakeFlags(result, EventFlags::Propagate);
        return result;
    }
}