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
    /** @brief Key filter specifiers */
    enum class Specifiers
    {
        OnlyPressed         = 0b000,
        PressedAndRelease   = 0b001,
        AllowRepeat         = 0b010
    };

    /** @brief A single key match */
    template<typename Callback>
    struct Match
    {
        UI::Key key {};
        UI::Modifier modifiers {};
        Specifiers specifiers {};
        Callback callback {};

        /** @brief Destructor */
        inline ~Match(void) noexcept = default;

        /** @brief Constructor */
        inline Match(
            const UI::Key key_,
            const UI::Modifier modifiers_,
            const Specifiers specifiers_,
            Callback callback_
        ) noexcept : key(key_), modifiers(modifiers_), specifiers(specifiers_), callback(std::move(callback_)) {}

        /** @brief Constructor */
        inline Match(
            const UI::Key key_,
            const UI::Modifier modifiers_,
            Callback callback_
        ) noexcept : key(key_), modifiers(modifiers_), callback(std::move(callback_)) {}

        /** @brief Constructor */
        inline Match(
            const UI::Key key_,
            const Specifiers specifiers_,
            Callback callback_
        ) noexcept : key(key_), specifiers(specifiers_), callback(std::move(callback_)) {}

        /** @brief Constructor */
        inline Match(
            const UI::Key key_,
            Callback callback_
        ) noexcept : key(key_), callback(std::move(callback_)) {}
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