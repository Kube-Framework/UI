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

    if constexpr (Decomposer::IndexSequence.size() == 2
            && Core::FunctionDecomposerHelper<decltype(Function)>::IndexSequence.size() != 1) {
        using Arg2 = std::tuple_element_t<1, typename Decomposer::ArgsTuple>;
        return ReturnType(
            [... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                return Function(arg1, arg2, Internal::ForwardArg(args)...);
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

    auto * const target = ConstexprTernaryRef(std::is_pointer_v<ClassType>, instance, &instance);
    if constexpr (Decomposer::IndexSequence.size() == 2
            && Core::FunctionDecomposerHelper<decltype(MemberFunction)>::IndexSequence.size() != 1) {
        using Arg2 = std::tuple_element_t<1, typename Decomposer::ArgsTuple>;
        return ReturnType(
            [target, ... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                return (target->*MemberFunction)(arg1, arg2, Internal::ForwardArg(args)...);
            }
        );
    } else {
        return ReturnType(
            [target, ... args = std::forward<Args>(args)](Arg1 arg1) noexcept {
                return (target->*MemberFunction)(arg1, Internal::ForwardArg(args)...);
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

    if constexpr (Decomposer::IndexSequence.size() == 2
            && Core::FunctionDecomposerHelper<Functor>::IndexSequence.size() != 1) {
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

    if constexpr (Decomposer::IndexSequence.size() == 2) { // @todo make this callable with 1 argument in Functor
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
inline kF::UI::DropEventArea kF::UI::DropEventArea::Make(Functors &&...functors) noexcept
{
    return DropEventArea {
        [](const std::type_identity<Functors> ...functorTypes) noexcept -> DropTypes {
            constexpr auto Forward = []<typename Functor>(const std::type_identity<Functor>) noexcept -> TypeHash {
                using Decomposer = Core::FunctionDecomposerHelper<Functor>;
                static_assert(Decomposer::IndexSequence.size() > 0, "Drop functor must have at least the catched type as first argument");
                using Type = std::remove_cvref_t<std::tuple_element_t<0, typename Decomposer::ArgsTuple>>;
                return TypeHash::Get<Type>();
            };
            return DropTypes { Forward(functorTypes)... };
        }(std::type_identity<Functors>()...),
        [](Functors &&...functors) noexcept -> DropFunctors {
            constexpr auto ForwardFunc = []<typename Functor>(Functor &&functor) noexcept -> DropFunctor {
                using Decomposer = Core::FunctionDecomposerHelper<Functor>;
                static_assert(Decomposer::IndexSequence.size() > 0, "Drop functor must have at least the catched type as first argument");
                static_assert(std::is_same_v<typename Decomposer::ReturnType, UI::EventFlags>, "Drop functor must return UI::EventFlags");
                using Type = std::remove_cvref_t<std::tuple_element_t<0, typename Decomposer::ArgsTuple>>;
                return DropFunctor(
                    [functor = std::forward<Functor>(functor)](
                        const void * const data, [[maybe_unused]] const DropEvent &dropEvent, [[maybe_unused]] const Area &area
                    ) noexcept {
                        const auto &type = *reinterpret_cast<const Type *>(data);
                        if constexpr (Decomposer::IndexSequence.size() == 1)
                            return functor(type);
                        else if constexpr (Decomposer::IndexSequence.size() == 2)
                            return functor(type, dropEvent);
                        else
                            return functor(type, dropEvent, area);
                    }
                );
            };
            DropFunctors dropFunctors;
            dropFunctors.reserve(sizeof...(Functors));
            (dropFunctors.push(ForwardFunc(functors)), ...);
            return dropFunctors;
        }(std::forward<Functors>(functors)...)
    };
}

inline kF::UI::EventFlags kF::UI::DropEventArea::event(const TypeHash typeHash, const void * const data, const DropEvent &event, const Area &area) noexcept
{
    auto index = 0u;
    for (const auto type : dropTypes) {
        if (type != typeHash) [[likely]] {
            ++index;
        } else {
            return dropFunctors[index](data, event, area);
            break;
        }
    }
    return EventFlags::Propagate;
}