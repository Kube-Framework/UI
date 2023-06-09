/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: KeyFilter
 */

#include "KeyFilter.hpp"

template<typename ...Callbacks>
inline kF::UI::EventFlags kF::UI::KeyFilter::MatchKeyEvent(const KeyEvent &event, const Match<Callbacks> &...args) noexcept
{
    constexpr auto Match = []<typename Callback>(const auto &event, const KeyFilter::Match<Callback> &match) {
        const bool matchModifiers = (match.modifiers == Modifier::None) | Core::HasFlags(event.modifiers, match.modifiers);
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