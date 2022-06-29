/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#pragma once

#include <Kube/Core/TupleUtils.hpp>
#include <Kube/Core/Functor.hpp>
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
        Stop                    = 0b00,
        Propagate               = 0b01,
        Invalidate              = 0b10,
        InvalidateAndPropagate  = 0b11
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
        Point origin {}; // Relative origin point [0, 1]
        Size scale { 1.0f, 1.0f }; // Relative scale [-inf, inf]
        Size minSize {}; // Absolute minimum size after scaling
        Point offset {}; // Absolute translation offset
        Core::Functor<void(Transform &, Area &), UIAllocator> event {}; // Runtime transform event
    };
    static_assert_fit_cacheline(Transform);

    /** @brief Layout describes the children distribution of an item */
    struct alignas_half_cacheline Layout
    {
        Padding padding {};
        Pixel spacing {};
        FlowType flowType { FlowType::Stack };
        Anchor anchor { Anchor::Center };
        SpacingType spacingType { SpacingType::Packed };
    };
    static_assert_fit_half_cacheline(Layout);

    /** @brief Timer handler */
    struct alignas_cacheline Timer
    {
        Core::Functor<bool(std::uint64_t delta), UIAllocator> event {};
        std::int64_t interval {};
        std::int64_t elapsed {};
    };
    static_assert_fit_cacheline(Timer);

    /** @brief Painter handler */
    struct alignas_half_cacheline PainterArea
    {
        Core::Functor<void(Painter &, const Area &), UIAllocator> event {};

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
        Core::Functor<EventFlags(const MouseEvent &, const Area &), UIAllocator> event {};
    };
    static_assert_fit_half_cacheline(MouseEventArea);

    /** @brief Motion handler */
    struct alignas_half_cacheline MotionEventArea
    {
        Core::Functor<EventFlags(const MotionEvent &, const Area &), UIAllocator> event {};
    };
    static_assert_fit_half_cacheline(MotionEventArea);

    /** @brief Wheel handler */
    struct alignas_half_cacheline WheelEventArea
    {
        Core::Functor<EventFlags(const WheelEvent &, const Area &), UIAllocator> event {};
    };
    static_assert_fit_half_cacheline(WheelEventArea);

    /** @brief Key handler */
    struct alignas_half_cacheline KeyEventReceiver
    {
        Core::Functor<EventFlags(const KeyEvent &), UIAllocator> event {};
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