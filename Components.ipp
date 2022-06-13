/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#include "Components.hpp"

template<auto Functor, typename ...Args>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Args &&...args) noexcept
{
    constexpr auto ForwardArg = []<typename Arg>(Arg &&arg) -> decltype(auto) {
        if constexpr (std::is_invocable_v<Arg>)
            return arg();
        else
            return std::forward<Arg>(arg);
    };

    return PainterArea {
        [... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            Functor(painter, area, ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Functor &&functor, Args &&...args) noexcept
{
    constexpr auto ForwardArg = []<typename Arg>(Arg &&arg) -> decltype(auto)  {
        if constexpr (std::is_invocable_v<Arg>)
            return arg();
        else
            return std::forward<Arg>(arg);
    };

    return PainterArea {
        [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            functor(painter, area, ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Args &&...args) noexcept
{
    constexpr auto ForwardArg = []<typename Arg>(Arg &&arg) -> decltype(auto)  {
        if constexpr (std::is_invocable_v<Arg>)
            return arg();
        else
            return std::forward<Arg>(arg);
    };

    return PainterArea {
        [... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            Functor{}(painter, area, ForwardArg(args)...);
        }
    };
}