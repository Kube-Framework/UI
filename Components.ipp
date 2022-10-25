/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#include "Components.hpp"

// Internal utils

template<auto Function, typename ReturnType, typename ...Args>
inline ReturnType kF::UI::Internal::MakeEventAreaFunctor(Args &&...args) noexcept
{
    using Decomposer = Core::FunctionDecomposerHelper<ReturnType>;
    static_assert(Decomposer::IndexSequence.size() >= 1 && Decomposer::IndexSequence.size() <= 2,
        "UI::Internal::MakeEventAreaFunctor: Invalid return functor format");
    using Arg1 = std::tuple_element_t<0, typename Decomposer::ArgsTuple>;

    if constexpr (Decomposer::IndexSequence.size() == 2) {
        using Arg2 = std::tuple_element_t<1, typename Decomposer::ArgsTuple>;
        return ReturnType(
            [... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                return Function(arg1, Internal::ForwardArg(args)...);
            }
        );
    } else {
        return ReturnType(
            [... args = std::forward<Args>(args)](Arg1 arg1) noexcept {
                return Function(arg1, Internal::ForwardArg(args)...);
            }
        );
    }
}

template<auto MemberFunction, typename ReturnType, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline ReturnType kF::UI::Internal::MakeEventAreaFunctor(ClassType &&instance, Args &&...args) noexcept
{
    using Decomposer = Core::FunctionDecomposerHelper<ReturnType>;
    static_assert(Decomposer::IndexSequence.size() >= 1 && Decomposer::IndexSequence.size() <= 2,
        "UI::Internal::MakeEventAreaFunctor: Invalid return functor format");
    using Arg1 = std::tuple_element_t<0, typename Decomposer::ArgsTuple>;

    if constexpr (Decomposer::IndexSequence.size() == 2) {
        using Arg2 = std::tuple_element_t<1, typename Decomposer::ArgsTuple>;
        return ReturnType(
            [instance, ... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                if constexpr (std::is_pointer_v<ClassType>)
                    return (instance->*MemberFunction)(arg1, arg2, Internal::ForwardArg(args)...);
                else
                    return (instance.*MemberFunction)(arg1, arg2, Internal::ForwardArg(args)...);
            }
        );
    } else {
        return ReturnType(
            [instance, ... args = std::forward<Args>(args)](Arg1 arg1) noexcept {
                if constexpr (std::is_pointer_v<ClassType>)
                    return (instance->*MemberFunction)(arg1, Internal::ForwardArg(args)...);
                else
                    return (instance.*MemberFunction)(arg1, Internal::ForwardArg(args)...);
            }
        );
    }
}

template<typename ReturnType, typename Functor, typename ...Args>
inline ReturnType kF::UI::Internal::MakeEventAreaFunctor(Functor &&functor, Args &&...args) noexcept
{
    using Decomposer = Core::FunctionDecomposerHelper<ReturnType>;
    static_assert(Decomposer::IndexSequence.size() >= 1 && Decomposer::IndexSequence.size() <= 2,
        "UI::Internal::MakeEventAreaFunctor: Invalid return functor format");
    using Arg1 = std::tuple_element_t<0, typename Decomposer::ArgsTuple>;

    if constexpr (Decomposer::IndexSequence.size() == 2) {
        using Arg2 = std::tuple_element_t<1, typename Decomposer::ArgsTuple>;
        return ReturnType(
            [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                return functor(arg1, arg2, Internal::ForwardArg(args)...);
            }
        );
    } else {
        return ReturnType(
            [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Arg1 arg1) noexcept {
                return functor(arg1, Internal::ForwardArg(args)...);
            }
        );
    }
}

template<typename ReturnType, typename Functor, typename ...Args>
inline ReturnType kF::UI::Internal::MakeEventAreaFunctor(Args &&...args) noexcept
{
    using Decomposer = Core::FunctionDecomposerHelper<ReturnType>;
    static_assert(Decomposer::IndexSequence.size() >= 1 && Decomposer::IndexSequence.size() <= 2,
        "UI::Internal::MakeEventAreaFunctor: Invalid return functor format");
    using Arg1 = std::tuple_element_t<0, typename Decomposer::ArgsTuple>;

    if constexpr (Decomposer::IndexSequence.size() == 2) {
        using Arg2 = std::tuple_element_t<1, typename Decomposer::ArgsTuple>;
        return ReturnType(
            [... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                return Functor{}(arg1, arg2, Internal::ForwardArg(args)...);
            }
        );
    } else {
        return ReturnType(
            [... args = std::forward<Args>(args)](Arg1 arg1) noexcept {
                return Functor{}(arg1, Internal::ForwardArg(args)...);
            }
        );
    }
}

// DropEventArea

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