/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#include "Components.hpp"

template<auto Functor, typename ...Args>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Args &&...args) noexcept
{
    return PainterArea {
        [... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            Functor(painter, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(ClassType * const instance, Args &&...args) noexcept
{
    return PainterArea {
        [instance, ... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            (instance->*MemberFunction)(painter, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(const ClassType * const instance, Args &&...args) noexcept
{
    return PainterArea {
        [instance, ... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            (instance->*MemberFunction)(painter, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Functor &&functor, Args &&...args) noexcept
{
    return PainterArea {
        [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            functor(painter, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Args &&...args) noexcept
{
    return PainterArea {
        [... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            Functor{}(painter, area, Internal::ForwardArg(args)...);
        }
    };
}


template<auto Functor, typename ...Args>
inline kF::UI::MouseEventArea kF::UI::MouseEventArea::Make(Args &&...args) noexcept
{
    return MouseEventArea {
        [... args = std::forward<Args>(args)](const MouseEvent &event, const Area &area) {
            return Functor(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::MouseEventArea kF::UI::MouseEventArea::Make(ClassType * const instance, Args &&...args) noexcept
{
    return MouseEventArea {
        [instance, ... args = std::forward<Args>(args)](const MouseEvent &event, const Area &area) {
            return (instance->*MemberFunction)(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::MouseEventArea kF::UI::MouseEventArea::Make(const ClassType * const instance, Args &&...args) noexcept
{
    return MouseEventArea {
        [instance, ... args = std::forward<Args>(args)](const MouseEvent &event, const Area &area) {
            return (instance->*MemberFunction)(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::MouseEventArea kF::UI::MouseEventArea::Make(Functor &&functor, Args &&...args) noexcept
{
    return MouseEventArea {
        [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](const MouseEvent &event, const Area &area) {
            return functor(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::MouseEventArea kF::UI::MouseEventArea::Make(Args &&...args) noexcept
{
    return MouseEventArea {
        [... args = std::forward<Args>(args)](const MouseEvent &event, const Area &area) {
            return Functor{}(event, area, Internal::ForwardArg(args)...);
        }
    };
}


template<auto Functor, typename ...Args>
inline kF::UI::MotionEventArea kF::UI::MotionEventArea::Make(Args &&...args) noexcept
{
    return MotionEventArea {
        [... args = std::forward<Args>(args)](const MotionEvent &event, const Area &area) {
            return Functor(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::MotionEventArea kF::UI::MotionEventArea::Make(ClassType * const instance, Args &&...args) noexcept
{
    return MotionEventArea {
        [instance, ... args = std::forward<Args>(args)](const MotionEvent &event, const Area &area) {
            return (instance->*MemberFunction)(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::MotionEventArea kF::UI::MotionEventArea::Make(const ClassType * const instance, Args &&...args) noexcept
{
    return MotionEventArea {
        [instance, ... args = std::forward<Args>(args)](const MotionEvent &event, const Area &area) {
            return (instance->*MemberFunction)(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::MotionEventArea kF::UI::MotionEventArea::Make(Functor &&functor, Args &&...args) noexcept
{
    return MotionEventArea {
        [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](const MotionEvent &event, const Area &area) {
            return functor(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::MotionEventArea kF::UI::MotionEventArea::Make(Args &&...args) noexcept
{
    return MotionEventArea {
        [... args = std::forward<Args>(args)](const MotionEvent &event, const Area &area) {
            return Functor{}(event, area, Internal::ForwardArg(args)...);
        }
    };
}


template<auto Functor, typename ...Args>
inline kF::UI::WheelEventArea kF::UI::WheelEventArea::Make(Args &&...args) noexcept
{
    return WheelEventArea {
        [... args = std::forward<Args>(args)](const WheelEvent &event, const Area &area) {
            return Functor(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::WheelEventArea kF::UI::WheelEventArea::Make(ClassType * const instance, Args &&...args) noexcept
{
    return WheelEventArea {
        [instance, ... args = std::forward<Args>(args)](const WheelEvent &event, const Area &area) {
            return (instance->*MemberFunction)(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::WheelEventArea kF::UI::WheelEventArea::Make(const ClassType * const instance, Args &&...args) noexcept
{
    return WheelEventArea {
        [instance, ... args = std::forward<Args>(args)](const WheelEvent &event, const Area &area) {
            return (instance->*MemberFunction)(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::WheelEventArea kF::UI::WheelEventArea::Make(Functor &&functor, Args &&...args) noexcept
{
    return WheelEventArea {
        [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](const WheelEvent &event, const Area &area) {
            return functor(event, area, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::WheelEventArea kF::UI::WheelEventArea::Make(Args &&...args) noexcept
{
    return WheelEventArea {
        [... args = std::forward<Args>(args)](const WheelEvent &event, const Area &area) {
            return Functor{}(event, area, Internal::ForwardArg(args)...);
        }
    };
}


template<auto Functor, typename ...Args>
inline kF::UI::KeyEventReceiver kF::UI::KeyEventReceiver::Make(Args &&...args) noexcept
{
    return KeyEventReceiver {
        [... args = std::forward<Args>(args)](const KeyEvent &event) {
            return Functor(event, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::KeyEventReceiver kF::UI::KeyEventReceiver::Make(ClassType * const instance, Args &&...args) noexcept
{
    return KeyEventReceiver {
        [instance, ... args = std::forward<Args>(args)](const KeyEvent &event) {
            return (instance->*MemberFunction)(event, Internal::ForwardArg(args)...);
        }
    };
}

template<auto MemberFunction, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline kF::UI::KeyEventReceiver kF::UI::KeyEventReceiver::Make(const ClassType * const instance, Args &&...args) noexcept
{
    return KeyEventReceiver {
        [instance, ... args = std::forward<Args>(args)](const KeyEvent &event) {
            return (instance->*MemberFunction)(event, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::KeyEventReceiver kF::UI::KeyEventReceiver::Make(Functor &&functor, Args &&...args) noexcept
{
    return KeyEventReceiver {
        [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](const KeyEvent &event) {
            return functor(event, Internal::ForwardArg(args)...);
        }
    };
}

template<typename Functor, typename ...Args>
inline kF::UI::KeyEventReceiver kF::UI::KeyEventReceiver::Make(Args &&...args) noexcept
{
    return KeyEventReceiver {
        [... args = std::forward<Args>(args)](const KeyEvent &event) {
            return Functor{}(event, Internal::ForwardArg(args)...);
        }
    };
}