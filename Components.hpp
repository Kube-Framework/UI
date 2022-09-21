/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#pragma once

#include <Kube/Core/TupleUtils.hpp>
#include <Kube/Core/Functor.hpp>
#include <Kube/Core/TrivialFunctor.hpp>
#include <Kube/Core/SmallVector.hpp>
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


        /** @brief Bind a member functor within a transform  */
        template<auto MemberFunction, typename ClassType>
        [[nodiscard]] static inline Transform Make(ClassType &&instance) noexcept
            { return Transform { Event::Make<MemberFunction>(std::forward<ClassType>(instance)) }; }

        /** @brief Bind a static functor within a transform  */
        template<auto Function>
        [[nodiscard]] static inline Transform Make(void) noexcept
            { return Transform { Event::Make<Function>() }; }
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
        using Event = Core::Functor<bool(std::uint64_t), UIAllocator>;

        Event event {};
        std::int64_t interval {};
        // Runtime state
        std::int64_t elapsedTimeState {};


        /** @brief Bind a member functor within a timer  */
        template<auto MemberFunction, typename ClassType>
        [[nodiscard]] static inline Timer Make(ClassType &&instance, const std::int64_t interval) noexcept
            { return Timer { Event::Make<MemberFunction>(std::forward<ClassType>(instance)), interval }; }

        /** @brief Bind a static functor within a timer  */
        template<auto Function>
        [[nodiscard]] static inline Timer Make(const std::int64_t interval) noexcept
            { return Timer { Event::Make<Function>(), interval }; }
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
        template<auto Functor, typename ...Args>
        [[nodiscard]] static PainterArea Make(Args &&...args) noexcept;

        /** @brief Bind a non-const member paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static PainterArea Make(ClassType * const instance, Args &&...args) noexcept;

        /** @brief Bind a const member paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static PainterArea Make(const ClassType * const instance, Args &&...args) noexcept;

        /** @brief Bind a static paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static PainterArea Make(Functor &&functor, Args &&...args) noexcept;

        /** @brief Bind a paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static PainterArea Make(Args &&...args) noexcept;
    };
    static_assert_fit_half_cacheline(PainterArea);


    /** @brief Mouse handler */
    struct alignas_half_cacheline MouseEventArea
    {
        /** @brief MouseEventArea event functor */
        using Event = Core::Functor<EventFlags(const MouseEvent &, const Area &), UIAllocator>;

        Event event {};


        /** @brief Bind a member functor within a mouse event area */
        template<auto MemberFunction, typename ClassType>
        [[nodiscard]] static inline MouseEventArea Make(ClassType &&instance) noexcept
            { return MouseEventArea { Event::Make<MemberFunction>(std::forward<ClassType>(instance)) }; }

        /** @brief Bind a static functor within a mouse event area */
        template<auto Function>
        [[nodiscard]] static inline MouseEventArea Make(void) noexcept
            { return MouseEventArea { Event::Make<Function>() }; }
    };
    static_assert_fit_half_cacheline(MouseEventArea);


    /** @brief Motion handler */
    struct alignas_cacheline MotionEventArea
    {
        /** @brief MotionEventArea event functor */
        using Event = Core::Functor<EventFlags(const MotionEvent &, const Area &), UIAllocator>;

        /** @brief MotionEventArea hover event functor */
        using HoverEvent = Core::Functor<void(const bool), UIAllocator, Core::CacheLineEighthSize * 3>;

        Event event {};
        HoverEvent hoverEvent {};
        bool invalidateOnHoverChanged { true };
        // Runtime state
        bool hovered {};


        /** @brief Bind a member functor within a motion event area */
        template<auto MemberFunction, typename ClassType>
        [[nodiscard]] static inline MotionEventArea Make(ClassType &&instance) noexcept
            { return MotionEventArea { Event::Make<MemberFunction>(std::forward<ClassType>(instance)) }; }

        /** @brief Bind a static functor within a motion event area */
        template<auto Function>
        [[nodiscard]] static inline MotionEventArea Make(void) noexcept
            { return MotionEventArea { Event::Make<Function>() }; }
    };
    static_assert_fit_cacheline(MotionEventArea);


    /** @brief Wheel handler */
    struct alignas_half_cacheline WheelEventArea
    {
        /** @brief WheelEventArea event functor */
        using Event = Core::Functor<EventFlags(const WheelEvent &, const Area &), UIAllocator>;

        Event event {};


        /** @brief Bind a member functor within a wheel event area */
        template<auto MemberFunction, typename ClassType>
        [[nodiscard]] static inline WheelEventArea Make(ClassType &&instance) noexcept
            { return WheelEventArea { Event::Make<MemberFunction>(std::forward<ClassType>(instance)) }; }

        /** @brief Bind a static functor within a wheel event area */
        template<auto Function>
        [[nodiscard]] static inline WheelEventArea Make(void) noexcept
            { return WheelEventArea { Event::Make<Function>() }; }
    };
    static_assert_fit_half_cacheline(WheelEventArea);


    /** @brief Drop handler */
    struct alignas_double_cacheline DropEventArea
    {
        /** @brief List of opaque types managed by the drop area */
        using DropTypes = Core::SmallVector<TypeHash, 2, UIAllocator>;

        /** @brief Drag functor */
        using DropFunctor = Core::Functor<void(const void *, const DropEvent &, const Area &), UIAllocator>;

        /** @brief List of drop functors managed by the drop area */
        using DropFunctors = Core::SmallVector<DropFunctor, 2, UIAllocator>;


        /** @brief Destructor */
        ~DropEventArea(void) noexcept = default;

        /** @brief Construct a drop event area with a list of drop functors that determines handled drop types
         *  @note Each drop functor must have the following arguments:
         *  template<DropType>(const DropType &, const DropEvent &, const Area &) */
        template<typename ...Functors>
        DropEventArea(Functors &&...functors) noexcept;

        /** @brief Copy constructor */
        inline DropEventArea(const DropEventArea &other) noexcept = default;

        /** @brief Move constructor */
        inline DropEventArea(DropEventArea &&other) noexcept = default;

        /** @brief Copy assignment */
        inline DropEventArea &operator=(const DropEventArea &other) noexcept = default;

        /** @brief Move assignment */
        inline DropEventArea &operator=(DropEventArea &&other) noexcept = default;


        /** @brief Get hovered state */
        [[nodiscard]] inline bool hovered(void) const noexcept { return _hovered; }


        /** @brief Process drop event */
        void onEvent(const TypeHash typeHash, const void * const data, const DropEvent &event, const Area &area) noexcept;

    private:
        DropTypes _types {};
        bool _hovered {};
        DropFunctors _functors {};
    };
    static_assert_fit_double_cacheline(DropEventArea);


    /** @brief Key handler */
    struct alignas_half_cacheline KeyEventReceiver
    {
        /** @brief KeyEventArea event functor */
        using Event = Core::Functor<EventFlags(const KeyEvent &), UIAllocator>;

        Event event {};


        /** @brief Bind a member functor within a key event receiver */
        template<auto MemberFunction, typename ClassType>
        [[nodiscard]] static inline KeyEventReceiver Make(ClassType &&instance) noexcept
            { return KeyEventReceiver { Event::Make<MemberFunction>(std::forward<ClassType>(instance)) }; }

        /** @brief Bind a static functor within a key event receiver */
        template<auto Function>
        [[nodiscard]] static inline KeyEventReceiver Make(void) noexcept
            { return KeyEventReceiver { Event::Make<Function>() }; }
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