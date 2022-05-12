/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UIObject
 */

#pragma once

#include <Kube/Core/Utils.hpp>

namespace kF::UI
{
    class UISystem;
    class UIObject;
    class UICachedObject;
}

/** @brief Sprite class manager the lifecycle of a SpriteManager instance */
class kF::UI::UIObject
{
public:
    /** @brief Get UI system */
    [[nodiscard]] inline UISystem &parent(void) const noexcept { return Parent(); }

    /** @brief Get UI system */
    [[nodiscard]] static UISystem &Parent(void) noexcept;
};

/** @brief Sprite class manager the lifecycle of a SpriteManager instance */
class alignas_eighth_cacheline kF::UI::UICachedObject : public UIObject
{
public:
    /** @brief Get UI system (cached version) */
    [[nodiscard]] inline UISystem &parent(void) const noexcept { return *_uiSystem; }

private:
    UISystem *_uiSystem { &Parent() };
};
static_assert_fit_eighth_cacheline(kF::UI::UICachedObject);