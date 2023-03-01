/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: KeyFilter
 */

#pragma once

#include "Components.hpp"
#include "Events.hpp"

namespace kF::UI
{
    struct KeyFilter;
}

/** @brief Utility structure used to implement common key events easily */
struct kF::UI::KeyFilter
{
    /** @brief A single key match */
    template<typename Callback>
    struct Match
    {
        UI::Key key {};
        UI::Modifier modifiers {};
        bool onlyPressed { true };
        bool allowRepeat { false };
        Callback callback {};
    };

    /** @brief Try to match a key event with a list of Match instances */
    template<typename ...Callbacks>
    [[nodiscard]] static inline UI::EventFlags MatchKeyEvent(const UI::KeyEvent &event, const Match<Callbacks> &...args) noexcept;


    /** @brief Filter a incoming event using list of Match instances */
    template<typename ...Callbacks>
    [[nodiscard]] EventFlags operator()(
        const KeyEvent &event,
        const Match<Callbacks> &...args
    ) const noexcept
        { return MatchKeyEvent(event, args...); }
};

#include "KeyFilter.ipp"