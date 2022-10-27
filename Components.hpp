/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#pragma once

#include <Kube/Core/TupleUtils.hpp>
#include <Kube/Core/Functor.hpp>
#include <Kube/Core/SmallVector.hpp>
#include <Kube/Core/Vector.hpp>
#include <Kube/ECS/Base.hpp>

#include "Animator.hpp"

namespace kF::UI
{
    class Painter;
    class MouseEvent;
    class MotionEvent;
    class WheelEvent;
    class DropEvent;
    class KeyEvent;
    enum class ComponentFlags : std::uint32_t;

    namespace Internal
    {
        /** @brief Utility to bind an event area functor with a free function */
        template<auto Function, typename ReturnType, typename ...Args>
        [[nodiscard]] ReturnType MakeEventAreaFunctor(Args &&...args) noexcept;

        /** @brief Utility to bind an event area functor with a member function */
        template<auto MemberFunction, typename ReturnType, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] ReturnType MakeEventAreaFunctor(ClassType &&instance, Args &&...args) noexcept;

        /** @brief Utility to bind an event area functor with a functor instance */
        template<typename ReturnType, typename Functor, typename ...Args>
        [[nodiscard]] ReturnType MakeEventAreaFunctor(Functor &&functor, Args &&...args) noexcept;

        /** @brief Utility to bind an event area functor with a functor type */
        template<typename ReturnType, typename Functor, typename ...Args>
        [[nodiscard]] ReturnType MakeEventAreaFunctor(Args &&...args) noexcept;
    }


    /** @brief Flags used as return type to indicate propagation and frame invalidation of an event */
    enum class EventFlags : std::uint32_t
    {
        Stop                    = 0b000,
        Propagate               = 0b001,
        Invalidate              = 0b010,
        InvalidateAndPropagate  = 0b011,
        Lock                    = 0b100,
        InvalidateAndLock       = 0b110
    };


    /** @brief Depth unit */
    using DepthUnit = std::uint32_t;

    /** @brief Depth cache */
    struct alignas_eighth_cacheline Depth
    {
        DepthUnit depth {};
        DepthUnit maxChildDepth {};
    };
    static_assert_fit_eighth_cacheline(Depth);


    /** @brief Tree Node Type */
    struct alignas_half_cacheline TreeNode
    {
        /** @brief Small optimized children vector */
        using Children = Core::SmallVector<ECS::Entity,
            (Core::CacheLineQuarterSize - sizeof(ECS::Entity) - sizeof(ComponentFlags)) / sizeof(ECS::Entity),
            UIAllocator
        >;

        Children children {};
        ECS::Entity parent {};
        ComponentFlags componentFlags {};
    };
    static_assert_fit_half_cacheline(TreeNode);


    /** @brief Transform describes a 2D space transformation */
    struct alignas_cacheline Transform
    {
        /** @brief Transform event functor */
        using Event = Core::Functor<void(Transform &, Area &), UIAllocator>;

        Point origin {}; // Relative origin point [0, 1]
        Size scale { 1.0f, 1.0f }; // Relative scale [-inf, inf]
        Size minSize {}; // Absolute minimum size after scaling
        Point offset {}; // Absolute translation offset
        Event event {}; // Runtime transform event
    };
    static_assert_fit_cacheline(Transform);


    /** @brief Layout describes the children distribution of an item */
    struct alignas_half_cacheline Layout
    {
        FlowType flowType {};
        Anchor anchor {};
        Anchor flexAnchor {};
        SpacingType spacingType {};
        SpacingType flexSpacingType {};
        Pixel spacing {};
        Pixel flexSpacing {};
        Padding padding {};
    };
    static_assert_fit_half_cacheline(Layout);


    /** @brief Timer handler */
    struct alignas_cacheline Timer
    {
        /** @brief Timer event functor */
        using Event = Core::Functor<bool(const std::uint64_t), UIAllocator>;

        Event event {};
        std::int64_t interval {};
        // Runtime state
        std::int64_t elapsedTimeState {};


        /** @brief Bind a static timer functor within a timer
         *  @note The functor must take 'const std::uint64_t' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Function, typename ...Args>
        [[nodiscard]] static inline Timer Make(const std::int64_t interval, Args &&...args) noexcept
            { return Timer(Internal::MakeEventAreaFunctor<Function, Timer::Event>(std::forward<Args>(args)...), interval); }

        /** @brief Bind a member timer functor within a timer
         *  @note The functor must take 'const std::uint64_t' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static inline Timer Make(const std::int64_t interval, ClassType &&instance, Args &&...args) noexcept
            { return Timer(Internal::MakeEventAreaFunctor<MemberFunction, Timer::Event>(std::forward<ClassType>(instance), std::forward<Args>(args)...), interval); }

        /** @brief Bind a static timer functor within a timer
         *  @note The functor must take 'const std::uint64_t' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline Timer Make(const std::int64_t interval, Functor &&functor, Args &&...args) noexcept
            { return Timer(Internal::MakeEventAreaFunctor<Timer::Event, Functor>(std::forward<Functor>(functor), std::forward<Args>(args)...), interval); }

        /** @brief Bind a timer functor within a timer
         *  @note The functor must take 'const std::uint64_t' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline Timer Make(const std::int64_t interval, Args &&...args) noexcept
            { return Timer(Internal::MakeEventAreaFunctor<Timer::Event, Functor>(std::forward<Args>(args)...), interval); }
    };
    static_assert_fit_cacheline(Timer);


    /** @brief Clip, only applies to children */
    struct alignas_quarter_cacheline Clip
    {
        Padding padding {};
    };
    static_assert_fit_quarter_cacheline(Clip);


    /** @brief Painter handler */
    struct alignas_half_cacheline PainterArea
    {
        /** @brief PainterArea event functor */
        using Event = Core::Functor<void(Painter &, const Area &), UIAllocator>;

        Event event {};


        /** @brief Bind a static paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Function, typename ...Args>
        [[nodiscard]] static inline PainterArea Make(Args &&...args) noexcept
            { return PainterArea(Internal::MakeEventAreaFunctor<Function, PainterArea::Event>(std::forward<Args>(args)...)); }

        /** @brief Bind a member paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static inline PainterArea Make(ClassType &&instance, Args &&...args) noexcept
            { return PainterArea(Internal::MakeEventAreaFunctor<MemberFunction, PainterArea::Event>(std::forward<ClassType>(instance), std::forward<Args>(args)...)); }

        /** @brief Bind a static paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline PainterArea Make(Functor &&functor, Args &&...args) noexcept
            { return PainterArea(Internal::MakeEventAreaFunctor<PainterArea::Event, Functor>(std::forward<Functor>(functor), std::forward<Args>(args)...)); }

        /** @brief Bind a paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline PainterArea Make(Args &&...args) noexcept
            { return PainterArea(Internal::MakeEventAreaFunctor<PainterArea::Event, Functor>(std::forward<Args>(args)...)); }
    };
    static_assert_fit_half_cacheline(PainterArea);


    /** @brief Mouse handler */
    struct alignas_half_cacheline MouseEventArea
    {
        /** @brief MouseEventArea event functor */
        using Event = Core::Functor<EventFlags(const MouseEvent &, const Area &), UIAllocator>;

        Event event {};


        /** @brief Bind a static mouse functor within a mouse event area
         *  @note The functor must take 'const MouseEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Function, typename ...Args>
        [[nodiscard]] static inline MouseEventArea Make(Args &&...args) noexcept
            { return MouseEventArea(Internal::MakeEventAreaFunctor<Function, MouseEventArea::Event>(std::forward<Args>(args)...)); }

        /** @brief Bind a member mouse functor within a mouse event area
         *  @note The functor must take 'const MouseEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static inline MouseEventArea Make(ClassType &&instance, Args &&...args) noexcept
            { return MouseEventArea(Internal::MakeEventAreaFunctor<MemberFunction, MouseEventArea::Event>(std::forward<ClassType>(instance), std::forward<Args>(args)...)); }

        /** @brief Bind a static mouse functor within a mouse event area
         *  @note The functor must take 'const MouseEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline MouseEventArea Make(Functor &&functor, Args &&...args) noexcept
            { return MouseEventArea(Internal::MakeEventAreaFunctor<MouseEventArea::Event, Functor>(std::forward<Functor>(functor), std::forward<Args>(args)...)); }

        /** @brief Bind a mouse functor within a mouse event area
         *  @note The functor must take 'const MouseEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline MouseEventArea Make(Args &&...args) noexcept
            { return MouseEventArea(Internal::MakeEventAreaFunctor<MouseEventArea::Event, Functor>(std::forward<Args>(args)...)); }
    };
    static_assert_fit_half_cacheline(MouseEventArea);


    /** @brief Motion handler */
    struct alignas_cacheline MotionEventArea
    {
        /** @brief MotionEventArea event functor */
        using Event = Core::Functor<EventFlags(const MotionEvent &, const Area &), UIAllocator>;

        Event event {};
        // Runtime state
        bool hovered {};


        /** @brief Bind a static motion functor within a motion event area
         *  @note The functor must take 'const MotionEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Function, typename ...Args>
        [[nodiscard]] static inline MotionEventArea Make(Args &&...args) noexcept
            { return MotionEventArea(Internal::MakeEventAreaFunctor<Function, MotionEventArea::Event>(std::forward<Args>(args)...)); }

        /** @brief Bind a member motion functor within a motion event area
         *  @note The functor must take 'const MotionEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static inline MotionEventArea Make(ClassType &&instance, Args &&...args) noexcept
            { return MotionEventArea(Internal::MakeEventAreaFunctor<MemberFunction, MotionEventArea::Event>(std::forward<ClassType>(instance), std::forward<Args>(args)...)); }

        /** @brief Bind a static motion functor within a motion event area
         *  @note The functor must take 'const MotionEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline MotionEventArea Make(Functor &&functor, Args &&...args) noexcept
            { return MotionEventArea(Internal::MakeEventAreaFunctor<MotionEventArea::Event, Functor>(std::forward<Functor>(functor), std::forward<Args>(args)...)); }

        /** @brief Bind a motion functor within a motion event area
         *  @note The functor must take 'const MotionEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline MotionEventArea Make(Args &&...args) noexcept
            { return MotionEventArea(Internal::MakeEventAreaFunctor<MotionEventArea::Event, Functor>(std::forward<Args>(args)...)); }
    };
    static_assert_fit_cacheline(MotionEventArea);


    /** @brief Wheel handler */
    struct alignas_half_cacheline WheelEventArea
    {
        /** @brief WheelEventArea event functor */
        using Event = Core::Functor<EventFlags(const WheelEvent &, const Area &), UIAllocator>;

        Event event {};


        /** @brief Bind a static wheel functor within a wheel event area
         *  @note The functor must take 'const WheelEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Function, typename ...Args>
        [[nodiscard]] static inline WheelEventArea Make(Args &&...args) noexcept
            { return WheelEventArea(Internal::MakeEventAreaFunctor<Function, WheelEventArea::Event>(std::forward<Args>(args)...)); }

        /** @brief Bind a member wheel functor within a wheel event area
         *  @note The functor must take 'const WheelEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static inline WheelEventArea Make(ClassType &&instance, Args &&...args) noexcept
            { return WheelEventArea(Internal::MakeEventAreaFunctor<MemberFunction, WheelEventArea::Event>(std::forward<ClassType>(instance), std::forward<Args>(args)...)); }

        /** @brief Bind a static wheel functor within a wheel event area
         *  @note The functor must take 'const WheelEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline WheelEventArea Make(Functor &&functor, Args &&...args) noexcept
            { return WheelEventArea(Internal::MakeEventAreaFunctor<WheelEventArea::Event, Functor>(std::forward<Functor>(functor), std::forward<Args>(args)...)); }

        /** @brief Bind a wheel functor within a wheel event area
         *  @note The functor must take 'const WheelEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline WheelEventArea Make(Args &&...args) noexcept
            { return WheelEventArea(Internal::MakeEventAreaFunctor<WheelEventArea::Event, Functor>(std::forward<Args>(args)...)); }
    };
    static_assert_fit_half_cacheline(WheelEventArea);


    /** @brief Drop handler */
    struct alignas_cacheline DropEventArea
    {
        /** @brief List of opaque types managed by the drop area */
        using DropTypes = Core::SmallVector<TypeHash, 1, UIAllocator>;

        /** @brief Drag functor */
        using DropFunctor = Core::Functor<EventFlags(const void * const, const DropEvent &, const Area &), UIAllocator>;

        /** @brief List of drop functors managed by the drop area */
        using DropFunctors = Core::Vector<DropFunctor, UIAllocator>;


        DropTypes dropTypes {};
        DropFunctors dropFunctors {};
        bool hovered {};


        /** @brief Construct a drop event area with a list of drop functors that determines handled drop types
         *  @note Each drop functor must have the following arguments:
         *  template<DropType>(const DropType &, const DropEvent &, const Area &) */
        template<typename ...Functors>
            requires (sizeof...(Functors) > 0)
        [[nodiscard]] static DropEventArea Make(Functors &&...functors) noexcept;


        /** @brief Process drop event */
        EventFlags event(const TypeHash typeHash, const void * const data, const DropEvent &event, const Area &area) noexcept;
    };
    static_assert_fit_cacheline(DropEventArea);


    /** @brief Key handler */
    struct alignas_half_cacheline KeyEventReceiver
    {
        /** @brief KeyEventArea event functor */
        using Event = Core::Functor<EventFlags(const KeyEvent &), UIAllocator>;

        Event event {};


        /** @brief Bind a static key functor within a key event receiver
         *  @note The functor must take 'const KeyEvent &' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Function, typename ...Args>
        [[nodiscard]] static inline KeyEventReceiver Make(Args &&...args) noexcept
            { return KeyEventReceiver(Internal::MakeEventAreaFunctor<Function, KeyEventReceiver::Event>(std::forward<Args>(args)...)); }

        /** @brief Bind a member key functor within a key event receiver
         *  @note The functor must take 'const KeyEvent &' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static inline KeyEventReceiver Make(ClassType &&instance, Args &&...args) noexcept
            { return KeyEventReceiver(Internal::MakeEventAreaFunctor<MemberFunction, KeyEventReceiver::Event>(std::forward<ClassType>(instance), std::forward<Args>(args)...)); }

        /** @brief Bind a static key functor within a key event receiver
         *  @note The functor must take 'const KeyEvent &' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline KeyEventReceiver Make(Functor &&functor, Args &&...args) noexcept
            { return KeyEventReceiver(Internal::MakeEventAreaFunctor<KeyEventReceiver::Event, Functor>(std::forward<Functor>(functor), std::forward<Args>(args)...)); }

        /** @brief Bind a key functor within a key event receiver
         *  @note The functor must take 'const KeyEvent &' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static inline KeyEventReceiver Make(Args &&...args) noexcept
            { return KeyEventReceiver(Internal::MakeEventAreaFunctor<KeyEventReceiver::Event, Functor>(std::forward<Args>(args)...)); }
    };
    static_assert_fit_half_cacheline(KeyEventReceiver);


    /** @brief Component flags */
    enum class ComponentFlags : std::uint32_t
    {
        None                = 0b0,
        TreeNode            = 0b000000000000001,
        Area                = 0b000000000000010,
        Depth               = 0b000000000000100,
        Constraints         = 0b000000000001000,
        Layout              = 0b000000000010000,
        Transform           = 0b000000000100000,
        PainterArea         = 0b000000001000000,
        Clip                = 0b000000010000000,
        MouseEventArea      = 0b000000100000000,
        MotionEventArea     = 0b000001000000000,
        WheelEventArea      = 0b000010000000000,
        DropEventArea       = 0b000100000000000,
        KeyEventReceiver    = 0b001000000000000,
        Timer               = 0b010000000000000,
        Animator            = 0b100000000000000
    };

    /** @brief Component types */
    using ComponentsTuple = std::tuple<
        TreeNode,
        Area,
        Depth,
        Constraints,
        Layout,
        Transform,
        PainterArea,
        Clip,
        MouseEventArea,
        MotionEventArea,
        WheelEventArea,
        DropEventArea,
        KeyEventReceiver,
        Timer,
        Animator
    >;

    /** @brief Concept of an UI component */
    template<typename ...Components>
    concept ComponentRequirements = (... || Core::TupleContainsElement<std::remove_cvref_t<Components>, ComponentsTuple>);

    /** @brief Get component type flag */
    template<ComponentRequirements Component>
    [[nodiscard]] constexpr ComponentFlags GetComponentFlag(void) noexcept
        { return static_cast<ComponentFlags>(1u << Core::TupleElementIndex<std::remove_cvref_t<Component>, ComponentsTuple, std::uint32_t>); }
}

#include "Components.ipp"