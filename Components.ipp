/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#include <Kube/Core/TupleUtils.hpp>

#include "Components.hpp"

// Internal utils

namespace kF::UI::Internal
{
    /** @brief Detect event argument count of 'MakeEventAreaFunctor' */
    template<std::size_t Index, typename EventArgsTuple, typename UserArgsTuple, typename Functor>
    [[nodiscard]] constexpr std::size_t DetectEventArgCount(void) noexcept
    {
        using ConcatTuple = kF::Core::ConcatenateTuple<EventArgsTuple, UserArgsTuple>;
        if constexpr (kF::Core::IsApplicable<Functor, ConcatTuple>)
            return Index;
        else if constexpr (Index == 0)
            return SIZE_MAX;
        else
            return DetectEventArgCount<Index - 1, Core::RemoveTupleElement<Index - 1, EventArgsTuple>, UserArgsTuple, Functor>();
    }
}

template<auto Function, typename ReturnType, typename ...Args>
inline ReturnType kF::UI::Internal::MakeEventAreaFunctor(Args &&...args) noexcept
{
    using Functor = decltype(Function);
    using EventDecomposer = Core::FunctionDecomposerHelper<ReturnType>;
    static_assert(EventDecomposer::IndexSequence.size() <= 4,
        "UI::Internal::MakeEventAreaFunctor: Invalid event functor signature");

    using EventArgsTuple = typename EventDecomposer::ArgsTuple;
    using UserArgsTuple = std::tuple<decltype(Internal::ForwardArg(args))...>;

    constexpr auto ArgumentCount = Internal::DetectEventArgCount<std::tuple_size_v<EventArgsTuple>, EventArgsTuple, UserArgsTuple, Functor>();
    static_assert(ArgumentCount != SIZE_MAX,
        "UI::Internal::MakeEventAreaFunctor: Invalid user functor signature");

    if constexpr (ArgumentCount == 0) {
        return ReturnType([... args = std::forward<Args>(args)](void) noexcept {
            return Function(Internal::ForwardArg(args)...);
        });
    } else {
        using Arg1 = std::tuple_element_t<0, EventArgsTuple>;
        if constexpr (ArgumentCount == 1) {
            return ReturnType([... args = std::forward<Args>(args)](Arg1 arg1) noexcept {
                return Function(arg1, Internal::ForwardArg(args)...);
            });
        } else {
            using Arg2 = std::tuple_element_t<1, EventArgsTuple>;
            if constexpr (ArgumentCount == 2) {
                return ReturnType([... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                    return Function(arg1, arg2, Internal::ForwardArg(args)...);
                });
            } else {
                using Arg3 = std::tuple_element_t<2, EventArgsTuple>;
                if constexpr (ArgumentCount == 3) {
                    return ReturnType([... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2, Arg3 arg3) noexcept {
                        return Function(arg1, arg2, arg3, Internal::ForwardArg(args)...);
                    });
                } else {
                    using Arg4 = std::tuple_element_t<3, EventArgsTuple>;
                    return ReturnType([... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) noexcept {
                        return Function(arg1, arg2, arg3, arg4, Internal::ForwardArg(args)...);
                    });
                }
            }
        }
    }
}

template<auto MemberFunction, typename ReturnType, typename ClassType, typename ...Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunction)>
inline ReturnType kF::UI::Internal::MakeEventAreaFunctor(ClassType &&instance, Args &&...args) noexcept
{
    using Functor = decltype(MemberFunction);
    using EventDecomposer = Core::FunctionDecomposerHelper<ReturnType>;
    static_assert(EventDecomposer::IndexSequence.size() <= 4,
        "UI::Internal::MakeEventAreaFunctor: Invalid event functor signature");

    auto * const target = ConstexprTernaryRef(std::is_pointer_v<ClassType>, instance, &instance);

    using EventArgsTuple = typename EventDecomposer::ArgsTuple;
    using InstanceEventArgsTuple = Core::ConcatenateTuple<std::tuple<decltype(target)>, EventArgsTuple>;
    using UserArgsTuple = std::tuple<decltype(Internal::ForwardArg(args))...>;

    constexpr auto ArgumentCount = Internal::DetectEventArgCount<std::tuple_size_v<InstanceEventArgsTuple>, InstanceEventArgsTuple, UserArgsTuple, Functor>();
    static_assert(ArgumentCount != SIZE_MAX,
        "UI::Internal::MakeEventAreaFunctor: Invalid user functor signature");

    if constexpr (ArgumentCount == 1) {
        return ReturnType([target, ... args = std::forward<Args>(args)](void) noexcept {
            return (target->*MemberFunction)(Internal::ForwardArg(args)...);
        });
    } else {
        using Arg1 = std::tuple_element_t<0, EventArgsTuple>;
        if constexpr (ArgumentCount == 2) {
            return ReturnType([target, ... args = std::forward<Args>(args)](Arg1 arg1) noexcept {
                return (target->*MemberFunction)(arg1, Internal::ForwardArg(args)...);
            });
        } else {
            using Arg2 = std::tuple_element_t<1, EventArgsTuple>;
            if constexpr (ArgumentCount == 3) {
                return ReturnType([target, ... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                    return (target->*MemberFunction)(arg1, arg2, Internal::ForwardArg(args)...);
                });
            } else {
                using Arg3 = std::tuple_element_t<2, EventArgsTuple>;
                if constexpr (ArgumentCount == 4) {
                    return ReturnType([target, ... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2, Arg3 arg3) noexcept {
                        return (target->*MemberFunction)(arg1, arg2, arg3, Internal::ForwardArg(args)...);
                    });
                } else {
                    using Arg4 = std::tuple_element_t<3, EventArgsTuple>;
                    return ReturnType([target, ... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) noexcept {
                        return (target->*MemberFunction)(arg1, arg2, arg3, arg4, Internal::ForwardArg(args)...);
                    });
                }
            }
        }
    }
}

template<typename ReturnType, typename Functor, typename ...Args>
inline ReturnType kF::UI::Internal::MakeEventAreaFunctor(Functor &&functor, Args &&...args) noexcept
{
    using EventDecomposer = Core::FunctionDecomposerHelper<ReturnType>;
    static_assert(EventDecomposer::IndexSequence.size() <= 4,
        "UI::Internal::MakeEventAreaFunctor: Invalid event functor signature");

    using EventArgsTuple = typename EventDecomposer::ArgsTuple;
    using UserArgsTuple = std::tuple<decltype(Internal::ForwardArg(args))...>;

    constexpr auto ArgumentCount = Internal::DetectEventArgCount<std::tuple_size_v<EventArgsTuple>, EventArgsTuple, UserArgsTuple, Functor>();
    static_assert(ArgumentCount != SIZE_MAX,
        "UI::Internal::MakeEventAreaFunctor: Invalid user functor signature");

    if constexpr (ArgumentCount == 0) {
        return ReturnType([functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](void) noexcept {
            return functor(Internal::ForwardArg(args)...);
        });
    } else {
        using Arg1 = std::tuple_element_t<0, EventArgsTuple>;
        if constexpr (ArgumentCount == 1) {
            return ReturnType([functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Arg1 arg1) noexcept {
                return functor(arg1, Internal::ForwardArg(args)...);
            });
        } else {
            using Arg2 = std::tuple_element_t<1, EventArgsTuple>;
            if constexpr (ArgumentCount == 2) {
                return ReturnType([functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                    return functor(arg1, arg2, Internal::ForwardArg(args)...);
                });
            } else {
                using Arg3 = std::tuple_element_t<2, EventArgsTuple>;
                if constexpr (ArgumentCount == 3) {
                    return ReturnType([functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2, Arg3 arg3) noexcept {
                        return functor(arg1, arg2, arg3, Internal::ForwardArg(args)...);
                    });
                } else {
                    using Arg4 = std::tuple_element_t<3, EventArgsTuple>;
                    return ReturnType([functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) noexcept {
                        return functor(arg1, arg2, arg3, arg4, Internal::ForwardArg(args)...);
                    });
                }
            }
        }
    }
}

template<typename ReturnType, typename Functor, typename ...Args>
inline ReturnType kF::UI::Internal::MakeEventAreaFunctor(Args &&...args) noexcept
{
    using EventDecomposer = Core::FunctionDecomposerHelper<ReturnType>;
    static_assert(EventDecomposer::IndexSequence.size() <= 4,
        "UI::Internal::MakeEventAreaFunctor: Invalid event functor signature");

    using EventArgsTuple = typename EventDecomposer::ArgsTuple;
    using UserArgsTuple = std::tuple<decltype(Internal::ForwardArg(args))...>;

    constexpr auto ArgumentCount = Internal::DetectEventArgCount<std::tuple_size_v<EventArgsTuple>, EventArgsTuple, UserArgsTuple, Functor>();
    static_assert(ArgumentCount != SIZE_MAX,
        "UI::Internal::MakeEventAreaFunctor: Invalid user functor signature");

    if constexpr (ArgumentCount == 0) {
        return ReturnType([... args = std::forward<Args>(args)](void) noexcept {
            return Functor{}(Internal::ForwardArg(args)...);
        });
    } else {
        using Arg1 = std::tuple_element_t<0, EventArgsTuple>;
        if constexpr (ArgumentCount == 1) {
            return ReturnType([... args = std::forward<Args>(args)](Arg1 arg1) noexcept {
                return Functor{}(arg1, Internal::ForwardArg(args)...);
            });
        } else {
            using Arg2 = std::tuple_element_t<1, EventArgsTuple>;
            if constexpr (ArgumentCount == 2) {
                return ReturnType([... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2) noexcept {
                    return Functor{}(arg1, arg2, Internal::ForwardArg(args)...);
                });
            } else {
                using Arg3 = std::tuple_element_t<2, EventArgsTuple>;
                if constexpr (ArgumentCount == 3) {
                    return ReturnType([... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2, Arg3 arg3) noexcept {
                        return Functor{}(arg1, arg2, arg3, Internal::ForwardArg(args)...);
                    });
                } else {
                    using Arg4 = std::tuple_element_t<3, EventArgsTuple>;
                    return ReturnType([... args = std::forward<Args>(args)](Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) noexcept {
                        return Functor{}(arg1, arg2, arg3, arg4, Internal::ForwardArg(args)...);
                    });
                }
            }
        }
    }
}

// DropEventArea

template<typename ...Functors>
    requires (sizeof...(Functors) > 0)
inline kF::UI::DropEventArea kF::UI::DropEventArea::Make(Functors &&...functors) noexcept
{
    constexpr auto ForwardTypes = [](const std::type_identity<Functors> ...functorTypes) noexcept -> DropTypes {
        constexpr auto Forward = []<typename Functor>(const std::type_identity<Functor>) noexcept -> TypeHash {
            using Decomposer = Core::FunctionDecomposerHelper<Functor>;
            static_assert(Decomposer::IndexSequence.size() > 0, "Drop functor must have at least the catched type as first argument");
            using Type = std::remove_cvref_t<std::tuple_element_t<0, typename Decomposer::ArgsTuple>>;
            return TypeHash::Get<Type>();
        };
        return DropTypes { Forward(functorTypes)... };
    };
    constexpr auto ForwardFuncs = []<typename ...Functors_FixMSVCPlz>(Functors_FixMSVCPlz &&...functors) noexcept -> DropFunctors {
        constexpr auto ForwardFunc = []<typename Functor>(Functor &&functor) noexcept -> DropFunctor {
            using Decomposer = Core::FunctionDecomposerHelper<Functor>;
            static_assert(Decomposer::IndexSequence.size() > 0, "Drop functor must have at least the catched type as first argument");
            static_assert(std::is_same_v<typename Decomposer::ReturnType, UI::EventFlags>, "Drop functor must return UI::EventFlags");
            using Type = std::remove_cvref_t<std::tuple_element_t<0, typename Decomposer::ArgsTuple>>;
            return DropFunctor(
                [functor = std::forward<Functor>(functor)](
                    const void * const data,
                    const DropEvent &dropEvent,
                    const Area &area,
                    const ECS::Entity entity,
                    UISystem &uiSystem
                ) noexcept -> EventFlags {
                    auto &type = *const_cast<Type *>(reinterpret_cast<const Type *>(data));
                    return Core::Invoke(functor, type, dropEvent, area, entity, uiSystem);
                }
            );
        };
        DropFunctors dropFunctors;
        dropFunctors.reserve(sizeof...(Functors));
        (dropFunctors.push(ForwardFunc(functors)), ...);
        return dropFunctors;
    };

    return DropEventArea {
        ForwardTypes(std::type_identity<Functors>()...),
        ForwardFuncs(std::forward<Functors>(functors)...)
    };
}

inline kF::UI::EventFlags kF::UI::DropEventArea::event(
    const TypeHash typeHash,
    const void * const data,
    const DropEvent &event,
    const Area &area,
    const ECS::Entity entity,
    UISystem &uiSystem
) noexcept
{
    auto index = 0u;
    for (const auto type : dropTypes) {
        if (type != typeHash) [[likely]]
            ++index;
        else
            return dropFunctors[index](data, event, area, entity, uiSystem);
    }
    return EventFlags::Propagate;
}