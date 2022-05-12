/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Item
 */

#include <Kube/Core/FunctionDecomposer.hpp>

// These definitions need UISystem include
#include "UISystem.hpp"

template<typename ...Components>
    requires ComponentRequirements<Components...>
inline kF::UI::Item &kF::UI::Item::attach(Components &&...components) noexcept
{
    static_assert(((!std::is_same_v<Components, TreeNode> && !std::is_same_v<Components, Area>) && ...),
        "UI::Item::attach: 'TreeNode' and 'Area' must not be attached, they are implicitly attached by Item's constructor");

    uiSystem().attach(_entity, std::forward<Components>(components)...);
    markComponents<Components...>();
    return *this;
}

template<typename ...Components>
    requires ComponentRequirements<Components...>
inline kF::UI::Item &kF::UI::Item::attachUpdate(Components &&...components) noexcept
{
    static_assert(((!std::is_same_v<Components, TreeNode> && !std::is_same_v<Components, Area>) && ...),
        "UI::Item::attachUpdate: 'TreeNode' and 'Area' must not be attached, they are implicitly attached by Item's constructor");

    uiSystem().attachUpdate(_entity, std::forward<Components>(components)...);
    markComponents<Components...>();
    return *this;
}

template<typename ...Functors>
inline kF::UI::Item &kF::UI::Item::attachUpdate(Functors &&...functors) noexcept
{
    static_assert(
        ([]<typename Functor>(Functor *) -> bool {
            using Component = std::remove_cvref_t<std::tuple_element_t<0, Core::Utils::FunctionDecomposerHelper<Functors>::ArgsTuple>>;
            return !std::is_same_v<Component, TreeNode> && !std::is_same_v<Component, Area>;
        }(std::declval<Functors *>()) && ...),
        "UI::Item::attachUpdate: 'TreeNode' and 'Area' must not be attached, they are implicitly attached by Item's constructor"
    );

    uiSystem().attachUpdate(_entity, std::forward<Functors>(functors)...);
    markComponents<
        std::remove_cvref_t<std::tuple_element_t<0, Core::Utils::FunctionDecomposerHelper<Functors>::ArgsTuple>>...
    >();
    return *this;
}

template<typename ...Components>
    requires ComponentRequirements<Components...>
inline void kF::UI::Item::dettach(void) noexcept
{
    static_assert(((!std::is_same_v<Components, TreeNode> && !std::is_same_v<Components, Area>) && ...),
        "UI::Item::dettach: 'TreeNode' and 'Area' must not be dettached, they are implicitly dettacged by Item's destructor");

    uiSystem().dettach<Components...>(_entity);
    unmarkComponents<Components...>();
}

template<typename ...Components>
    requires ComponentRequirements<Components...>
inline void kF::UI::Item::tryDettach(void) noexcept
{
    static_assert(((!std::is_same_v<Components, TreeNode> && !std::is_same_v<Components, Area>) && ...),
        "UI::Item::tryDettach: 'TreeNode' and 'Area' must not be dettached, they are implicitly dettacged by Item's destructor");

    uiSystem().tryDettach<Area>(_entity);
    unmarkComponents<Components...>();
}

template<typename Component>
inline Component &kF::UI::Item::get(void) noexcept
{
    return uiSystem().get<Component>(_entity);
}

template<typename Component>
inline const Component &kF::UI::Item::get(void) const noexcept
{
    return uiSystem().get<Component>(_entity);
}

template<typename ...Components>
inline void kF::UI::Item::markComponents(void) noexcept
{
    const auto old = _componentFlags;
    _componentFlags = Core::Utils::MakeFlags(_componentFlags, GetComponentFlag<Components>()...);
    if (old != _componentFlags && Core::Utils::HasFlags(_componentFlags, ComponentFlags::TreeNode)) [[likely]] {
        get<TreeNode>().componentFlags = _componentFlags;
    }
}

template<typename ...Components>
inline void kF::UI::Item::unmarkComponents(void) noexcept
{
    const auto old = _componentFlags;
    _componentFlags = Core::Utils::RemoveFlags(_componentFlags, GetComponentFlag<Components>()...);
    if (old != _componentFlags && Core::Utils::HasFlags(_componentFlags, ComponentFlags::TreeNode)) [[likely]] {
        get<TreeNode>().componentFlags = _componentFlags;
    }
}