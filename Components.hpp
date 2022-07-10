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

#include "Base.hpp"

namespace kF::UI
{
    class Painter;
    class MouseEvent;
    class MotionEvent;
    class WheelEvent;
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
    struct Depth
    {
        DepthUnit depth {};
        DepthUnit maxChildDepth {};
    };

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
        FlowType flowType { FlowType::Stack };
        Anchor anchor { Anchor::Center };
        Anchor flexAnchor { Anchor::Center };
        SpacingType spacingType { SpacingType::Packed };
        SpacingType flexSpacingType { SpacingType::Packed };
        Pixel spacing {};
        Pixel flexSpacing {};
        Padding padding {};
    };
    static_assert_fit_half_cacheline(Layout);

    /** @brief Timer event functor */
    using TimerEvent = Core::Functor<bool(std::uint64_t), UIAllocator>;

    /** @brief Timer handler */
    struct alignas_cacheline Timer
    {
        TimerEvent event {};
        std::int64_t interval {};
        std::int64_t elapsed {};
    };
    static_assert_fit_cacheline(Timer);


    /** @brief Painter handler */
    struct alignas_half_cacheline PainterArea
    {
        /** @brief PainterArea event functor */
        using Event = Core::Functor<void(Painter &, const Area &), UIAllocator>;

        Event event {};

        /** @brief Wrap any static paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Functor, typename ...Args>
        [[nodiscard]] static PainterArea Make(Args &&...args) noexcept;

        /** @brief Wrap any non-const member paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static PainterArea Make(ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any const member paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static PainterArea Make(const ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any static paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static PainterArea Make(Functor &&functor, Args &&...args) noexcept;

        /** @brief Wrap any paint functor class within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static PainterArea Make(Args &&...args) noexcept;
    };
    static_assert_fit_half_cacheline(PainterArea);

    /** @brief Clip, only applies to children */
    struct alignas_quarter_cacheline Clip
    {
        Padding padding {};
    };
    static_assert_fit_quarter_cacheline(Clip);

    /** @brief Mouse handler */
    struct alignas_half_cacheline MouseEventArea
    {
        /** @brief MouseEventArea event functor */
        using Event = Core::Functor<EventFlags(const MouseEvent &, const Area &), UIAllocator>;

        Event event {};

        /** @brief Wrap any static mouse event functor within a mouse event area
         *  @note The functor must take 'const MouseEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Functor, typename ...Args>
        [[nodiscard]] static MouseEventArea Make(Args &&...args) noexcept;

        /** @brief Wrap any non-const member mouse event functor within a mouse event area
         *  @note The functor must take 'const MouseEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static MouseEventArea Make(ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any const member mouse event functor within a mouse event area
         *  @note The functor must take 'const MouseEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static MouseEventArea Make(const ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any static mouse event functor within a mouse event area
         *  @note The functor must take 'const MouseEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static MouseEventArea Make(Functor &&functor, Args &&...args) noexcept;

        /** @brief Wrap any mouse event functor class within a mouse event area
         *  @note The functor must take 'const MouseEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static MouseEventArea Make(Args &&...args) noexcept;
    };
    static_assert_fit_half_cacheline(MouseEventArea);

    /** @brief Motion handler */
    struct alignas_half_cacheline MotionEventArea
    {
        /** @brief MotionEventArea event functor */
        using Event = Core::Functor<EventFlags(const MotionEvent &, const Area &), UIAllocator>;

        Event event {};

        /** @brief Wrap any static motion event functor within a motion event area
         *  @note The functor must take 'const MotionEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Functor, typename ...Args>
        [[nodiscard]] static MotionEventArea Make(Args &&...args) noexcept;

        /** @brief Wrap any non-const member motion event functor within a motion event area
         *  @note The functor must take 'const MotionEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static MotionEventArea Make(ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any const member motion event functor within a motion event area
         *  @note The functor must take 'const MotionEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static MotionEventArea Make(const ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any static motion event functor within a motion event area
         *  @note The functor must take 'const MotionEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static MotionEventArea Make(Functor &&functor, Args &&...args) noexcept;

        /** @brief Wrap any motion event functor class within a motion event area
         *  @note The functor must take 'const MotionEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static MotionEventArea Make(Args &&...args) noexcept;
    };
    static_assert_fit_half_cacheline(MotionEventArea);

    /** @brief Wheel handler */
    struct alignas_half_cacheline WheelEventArea
    {
        /** @brief WheelEventArea event functor */
        using Event = Core::Functor<EventFlags(const WheelEvent &, const Area &), UIAllocator>;

        Event event {};

        /** @brief Wrap any static wheel event functor within a wheel event area
         *  @note The functor must take 'const WheelEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Functor, typename ...Args>
        [[nodiscard]] static WheelEventArea Make(Args &&...args) noexcept;

        /** @brief Wrap any non-const member wheel event functor within a wheel event area
         *  @note The functor must take 'const WheelEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static WheelEventArea Make(ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any const member wheel event functor within a wheel event area
         *  @note The functor must take 'const WheelEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static WheelEventArea Make(const ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any static wheel event functor within a wheel event area
         *  @note The functor must take 'const WheelEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static WheelEventArea Make(Functor &&functor, Args &&...args) noexcept;

        /** @brief Wrap any wheel event functor class within a wheel event area
         *  @note The functor must take 'const WheelEvent &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static WheelEventArea Make(Args &&...args) noexcept;
    };
    static_assert_fit_half_cacheline(WheelEventArea);

    /** @brief Key handler */
    struct alignas_half_cacheline KeyEventReceiver
    {
        /** @brief MouseEventArea event functor */
        using Event = Core::Functor<EventFlags(const KeyEvent &), UIAllocator>;

        Event event {};

        /** @brief Wrap any static key event functor within a key event receiver
         *  @note The functor must take 'const KeyEvent &' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto Functor, typename ...Args>
        [[nodiscard]] static KeyEventReceiver Make(Args &&...args) noexcept;

        /** @brief Wrap any non-const member key event functor within a key event receiver
         *  @note The functor must take 'const KeyEvent &' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static KeyEventReceiver Make(ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any const member key event functor within a key event receiver
         *  @note The functor must take 'const KeyEvent &' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<auto MemberFunction, typename ClassType, typename ...Args>
            requires std::is_member_function_pointer_v<decltype(MemberFunction)>
        [[nodiscard]] static KeyEventReceiver Make(const ClassType * const instance, Args &&...args) noexcept;

        /** @brief Wrap any static key event functor within a key event receiver
         *  @note The functor must take 'const KeyEvent &' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static KeyEventReceiver Make(Functor &&functor, Args &&...args) noexcept;

        /** @brief Wrap any key event functor class within a key event receiver
         *  @note The functor must take 'const KeyEvent &' as its first argument
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static KeyEventReceiver Make(Args &&...args) noexcept;
    };
    static_assert_fit_half_cacheline(KeyEventReceiver);

    /** @brief Component flags */
    enum class ComponentFlags : std::uint32_t
    {
        None                = 0b0,
        TreeNode            = 0b00000000000001,
        Area                = 0b00000000000010,
        Depth               = 0b00000000000100,
        Constraints         = 0b00000000001000,
        Layout              = 0b00000000010000,
        Transform           = 0b00000000100000,
        PainterArea         = 0b00000001000000,
        Clip                = 0b00000010000000,
        MouseEventArea      = 0b00000100000000,
        MotionEventArea     = 0b00001000000000,
        WheelEventArea      = 0b00010000000000,
        KeyEventReceiver    = 0b00100000000000,
        Timer               = 0b01000000000000,
        Animator            = 0b10000000000000
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