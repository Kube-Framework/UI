/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#pragma once

#include <Kube/Core/Functor.hpp>
#include <Kube/Core/SmallVector.hpp>
#include <Kube/ECS/Base.hpp>

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
        TreeNode            = 0b000000001,
        Area                = 0b000000010,
        Depth               = 0b000000100,
        Constraints         = 0b000001000,
        Layout              = 0b000010000,
        Timer               = 0b000100000,
        PainterArea         = 0b001000000,
        MouseEventArea      = 0b010000000,
        KeyEventReceiver    = 0b100000000
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
        using Children = Core::TinySmallVector<ECS::Entity,
            (Core::CacheLineQuarterSize - sizeof(ECS::Entity) - sizeof(ComponentFlags)) / sizeof(ECS::Entity),
            UIAllocator
        >;

        Children children {};
        ECS::Entity parent {};
        ComponentFlags componentFlags { ComponentFlags::None };
    };
    static_assert_fit_half_cacheline(TreeNode);

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
         *      Additional arguments (Args...) must match remaining functor's arguments */
        template<auto Functor, typename ...Args>
            requires std::is_invocable_v<decltype(Functor), kF::UI::Painter &, const kF::UI::Area &, std::remove_cvref_t<Args> &...>
        [[nodiscard]] static PainterArea Make(Args &&...args) noexcept;

        /** @brief Wrap any static paint functor within a painter area
         *  @note The functor must take 'Painter &, const Area &' as its first two arguments
         *      Additional arguments (Args...) must match remaining functor's arguments */
        template<typename Functor, typename ...Args>
            requires std::is_invocable_v<Functor, kF::UI::Painter &, const kF::UI::Area &, std::remove_cvref_t<Args> &...>
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
            || std::same_as<Components, Layout>
            || std::same_as<Components, Timer>
            || std::same_as<Components, PainterArea>
            || std::same_as<Components, MouseEventArea>
            || std::same_as<Components, KeyEventReceiver>
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
        else
            return ComponentFlags::None;
    }
}

#include "Components.ipp"