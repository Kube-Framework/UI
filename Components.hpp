/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#pragma once

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
    class KeyEvent;

    /** @brief Depth */
    using Depth = std::uint32_t;

    /** @brief Component flags */
    enum class ComponentFlags : std::uint32_t
    {
        None                = 0b0,
        TreeNode            = 0b00000000001,
        Area                = 0b00000000010,
        Depth               = 0b00000000100,
        Constraints         = 0b00000001000,
        Transform           = 0b00000010000,
        Layout              = 0b00000100000,
        Timer               = 0b00001000000,
        PainterArea         = 0b00010000000,
        MouseEventArea      = 0b00100000000,
        KeyEventReceiver    = 0b01000000000,
        Animator            = 0b10000000000
    };


    /** @brief Flags used as return type to indicate propagation and frame invalidation of an event */
    enum class EventFlags : std::uint32_t
    {
        Stop                    = 0b00,
        Propagate               = 0b01,
        Invalidate              = 0b10,
        InvalidateAndPropagate  = 0b11,
    };


    /** @brief Tree Node Type */
    struct alignas_half_cacheline TreeNode
    {
        /** @brief Children vector is small optimized to fit half cacheline */
        using Children = Core::SmallVector<ECS::Entity,
            (Core::CacheLineQuarterSize - sizeof(ECS::Entity) - sizeof(ComponentFlags)) / sizeof(ECS::Entity),
            UIAllocator
        >;

        Children children {};
        ECS::Entity parent {};
        ComponentFlags componentFlags { ComponentFlags::None };
    };
    static_assert_fit_half_cacheline(TreeNode);

    /** @brief Transform describes a 2D space transformation */
    struct alignas_half_cacheline Transform
    {
        Point origin {}; // Relative origin point [0, 1]
        Size scale {}; // Relative scale [-inf, inf]
        Size minSize {}; // Absolute minimum size after scaling
        Point offset {}; // Absolute translation offset
    };
    static_assert_fit_half_cacheline(Transform);

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
        // bool singleShot {}; ??
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

        /** @brief Wrap any static paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) should match remaining functor's arguments
         *      If an argument doesn't match, it must be a functor that returns the matching argument */
        template<typename Functor, typename ...Args>
        [[nodiscard]] static PainterArea Make(Functor &&functor, Args &&...args) noexcept;
    };
    static_assert_fit_half_cacheline(PainterArea);

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

    /** @brief Key handler */
    struct alignas_half_cacheline KeyEventReceiver
    {
        Core::Functor<EventFlags(const KeyEvent &), UIAllocator> event {};
    };
    static_assert_fit_half_cacheline(KeyEventReceiver);


    /** @brief Concept of an UI component */
    template<typename ...Components>
    concept ComponentRequirements = (
        (
            std::same_as<Components, TreeNode>
            || std::same_as<Components, Area>
            || std::same_as<Components, Constraints>
            || std::same_as<Components, Transform>
            || std::same_as<Components, Layout>
            || std::same_as<Components, Timer>
            || std::same_as<Components, PainterArea>
            || std::same_as<Components, MouseEventArea>
            || std::same_as<Components, KeyEventReceiver>
            || std::same_as<Components, Animator>
        ) && ...
    );


    /** @brief Get component type flag */
    template<typename Component>
    [[nodiscard]] constexpr ComponentFlags GetComponentFlag(void) noexcept
    {
        if constexpr (std::is_same_v<Component, TreeNode>)
            return ComponentFlags::TreeNode;
        else if constexpr (std::is_same_v<Component, Area>)
            return ComponentFlags::Area;
        else if constexpr (std::is_same_v<Component, Constraints>)
            return ComponentFlags::Constraints;
        else if constexpr (std::is_same_v<Component, Transform>)
            return ComponentFlags::Transform;
        else if constexpr (std::is_same_v<Component, Layout>)
            return ComponentFlags::Layout;
        else if constexpr (std::is_same_v<Component, Timer>)
            return ComponentFlags::Timer;
        else if constexpr (std::is_same_v<Component, PainterArea>)
            return ComponentFlags::PainterArea;
        else if constexpr (std::is_same_v<Component, MouseEventArea>)
            return ComponentFlags::MouseEventArea;
        else if constexpr (std::is_same_v<Component, KeyEventReceiver>)
            return ComponentFlags::KeyEventReceiver;
        else if constexpr (std::is_same_v<Component, Animator>)
            return ComponentFlags::Animator;
        else
            return ComponentFlags::None;
    }
}

#include "Components.ipp"