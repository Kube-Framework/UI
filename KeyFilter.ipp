/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: KeyFilter
 */

#include "KeyFilter.hpp"

template<typename ...Callbacks>
inline kF::UI::EventFlags kF::UI::KeyFilter::MatchKeyEvent(const KeyEvent &event, const Match<Callbacks> &...args) noexcept
{
    constexpr auto Match = []<typename Callback>(const auto &event, const KeyFilter::Match<Callback> &match) {
        const bool matchModifiers
            = (Core::HasFlags(event.modifiers, Modifier::Shift) == Core::HasFlags(match.modifiers, Modifier::Shift))
            & (Core::HasFlags(event.modifiers, Modifier::Ctrl) == Core::HasFlags(match.modifiers, Modifier::Ctrl))
            & (Core::HasFlags(event.modifiers, Modifier::Alt) == Core::HasFlags(match.modifiers, Modifier::Alt))
            & (Core::HasFlags(event.modifiers, Modifier::Super) == Core::HasFlags(match.modifiers, Modifier::Super));
        if ((event.key != match.key) | !matchModifiers) [[likely]]
            return EventFlags::Propagate;
        const auto pressedAndRelease = Core::HasFlags(match.specifiers, Specifiers::PressedAndRelease);
        const auto blockRepeat = !Core::HasFlags(match.specifiers, Specifiers::AllowRepeat);
        if ((!pressedAndRelease & !event.state) | (blockRepeat & event.repeat))
            return EventFlags::Stop;
        if constexpr (std::is_invocable_v<decltype(match.callback), bool>)
            match.callback(event.state);
        else
            match.callback();
        return EventFlags::Invalidate;
    };
    if constexpr (sizeof...(Callbacks) != 0) {
        if (EventFlags flags; (((flags = Match(event, args)) != EventFlags::Propagate) || ...))
            return flags;
    }
    return EventFlags::Propagate;
}