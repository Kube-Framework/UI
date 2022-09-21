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

template<typename ...Functors>
    requires (sizeof...(Functors) > 0)
inline kF::UI::DropEventArea::DropEventArea(Functors &&...functors) noexcept
    : _dropTypes(
        [](Functors &&...functors)
        {
            constexpr auto Forward = []<typename Functor>(Functor &&)
            {
                using Decomposer = Core::FunctionDecomposerHelper<Functor>;
                static_assert(Decomposer::IndexSequence.size() > 0, "Drop functor must have at least the catched type as first argument");
                using Type = std::remove_cvref_t<std::tuple_element_t<0, typename Decomposer::ArgsTuple>>;
                return TypeHash::Get<Type>();
            };
            return DropTypes { Forward(std::forward<Functors>(functors))... };
        }(std::forward<Functors>(functors)...)
    )
    , _dropFunctors(
        [](Functors &&...functors)
        {
            constexpr auto Forward = []<typename Functor>(Functor &&functor)
            {
                using Decomposer = Core::FunctionDecomposerHelper<Functor>;
                static_assert(Decomposer::IndexSequence.size() > 0, "Drop functor must have at least the catched type as first argument");
                using Type = std::remove_cvref_t<std::tuple_element_t<0, typename Decomposer::ArgsTuple>>;
                return DropFunctor(
                    [functor = std::forward<Functor>(functor)](const void * const data,
                            [[maybe_unused]] const DropEvent &dropEvent, [[maybe_unused]] const Area &area)
                    {
                        const auto &type = *reinterpret_cast<const Type * const>(data);
                        if constexpr (Decomposer::IndexSequence.size() == 1)
                            functor(type);
                        else if constexpr (Decomposer::IndexSequence.size() == 2)
                            functor(type, dropEvent);
                        else
                            functor(type, dropEvent, area);
                    }
                );
            };
            return DropFunctors { Forward(std::forward<Functors>(functors))... };
        }(std::forward<Functors>(functors)...)
    )
{
}