/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#include "Components.hpp"

template<auto Functor, typename ...Args>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Args &&...args) noexcept
{
    return PainterArea {
        [... args = std::forward<Args>(args)](Painter &painter, const Area &area) noexcept {
            Functor(painter, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(ClassType * const instance, Args &&...args) noexcept
{
    return PainterArea {
        [instance, ... args = std::forward<Args>(args)](Painter &painter, const Area &area) noexcept {
            (instance->*MemberFunction)(painter, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(const ClassType * const instance, Args &&...args) noexcept
{
    return PainterArea {
        [instance, ... args = std::forward<Args>(args)](Painter &painter, const Area &area) noexcept {
            (instance->*MemberFunction)(painter, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Functor &&functor, Args &&...args) noexcept
{
    return PainterArea {
        [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Painter &painter, const Area &area) noexcept {
            functor(painter, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Args &&...args) noexcept
{
    return PainterArea {
        [... args = std::forward<Args>(args)](Painter &painter, const Area &area) noexcept {
            Functor{}(painter, area, Internal::ForwardArg(args)...);
        }
    };
}