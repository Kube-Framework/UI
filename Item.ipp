/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Item
 */

#include <Kube/Core/FunctionDecomposer.hpp>

// These definitions need UISystem include
#include "UISystem.hpp"

template<typename ...Components>
    requires kF::UI::ComponentRequirements<Components...>
bool kF::UI::Item::exists(void) const noexcept
{
    return uiSystem().exists<Components...>(_entity);
}

template<typename ...Components>
    requires kF::UI::ComponentRequirements<Components...>
inline kF::UI::Item &kF::UI::Item::attach(Components &&...components) noexcept
{
    static_assert(((!IsBaseItemComponent<Components>) && ...),
        "UI::Item::attach: 'TreeNode' and 'Area' must not be attached, they are implicitly attached by Item's constructor");

    uiSystem().attach(_entity, std::forward<Components>(components)...);
    markComponents<Components...>();
    return *this;
}

template<typename ...Components>
    requires kF::UI::ComponentRequirements<Components...>
inline kF::UI::Item &kF::UI::Item::tryAttach(Components &&...components) noexcept
{
    static_assert(((!IsBaseItemComponent<Components>) && ...),
        "UI::Item::tryAttach: 'TreeNode' and 'Area' must not be attached, they are implicitly attached by Item's constructor");

    uiSystem().tryAttach(_entity, std::forward<Components>(components)...);
    markComponents<Components...>();
    return *this;
}

template<typename ...Functors>
inline kF::UI::Item &kF::UI::Item::tryAttach(Functors &&...functors) noexcept
{
    static_assert(
        ([]<typename Functor>(Functor *) -> bool {
            using Component = std::remove_cvref_t<std::tuple_element_t<0, typename Core::FunctionDecomposerHelper<Functors>::ArgsTuple>>;
            return !IsBaseItemComponent<Component>;
        }(std::declval<Functors *>()) && ...),
        "UI::Item::tryAttach: 'TreeNode', 'Area' and 'Depth' must not be attached, they are implicitly attached by Item's constructor"
    );

    uiSystem().tryAttach(_entity, std::forward<Functors>(functors)...);
    markComponents<
        std::remove_cvref_t<std::tuple_element_t<0, typename Core::FunctionDecomposerHelper<Functors>::ArgsTuple>>...
    >();
    return *this;
}

template<typename ...Components>
    requires kF::UI::ComponentRequirements<Components...>
inline void kF::UI::Item::dettach(void) noexcept
{
    static_assert(((!IsBaseItemComponent<Components>) && ...),
        "UI::Item::dettach: 'TreeNode' and 'Area' must not be dettached, they are implicitly dettached by Item's destructor");

    uiSystem().onDettach<Components...>(_entity);
    uiSystem().dettach<Components...>(_entity);
    unmarkComponents<Components...>();
}

template<typename ...Components>
    requires kF::UI::ComponentRequirements<Components...>
inline void kF::UI::Item::tryDettach(void) noexcept
{
    static_assert(((!IsBaseItemComponent<Components>) && ...),
        "UI::Item::tryDettach: 'TreeNode' and 'Area' must not be dettached, they are implicitly dettached by Item's destructor");

    uiSystem().onDettach<Components...>(_entity);
    uiSystem().tryDettach<Components...>(_entity);
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

template<typename Callback>
inline void kF::UI::Item::delayToTickEnd(Callback &&callback) noexcept
{
    uiSystem().delayToTickEnd(std::forward<Callback>(callback));
}

inline bool kF::UI::Item::isHovered(void) const noexcept
{
    return uiSystem().isHovered(_entity);
}

inline bool kF::UI::Item::isDropHovered(void) const noexcept
{
    return uiSystem().isDropHovered(_entity);
}

template<typename ...Components>
inline void kF::UI::Item::markComponents(void) noexcept
{
    const auto old = _componentFlags;
    _componentFlags = Core::MakeFlags(_componentFlags, GetComponentFlag<Components>()...);
    if (old != _componentFlags && Core::HasFlags(_componentFlags, ComponentFlags::TreeNode)) [[likely]] {
        get<TreeNode>().componentFlags = _componentFlags;
    }
}

template<typename ...Components>
inline void kF::UI::Item::unmarkComponents(void) noexcept
{
    const auto old = _componentFlags;
    _componentFlags = Core::RemoveFlags(_componentFlags, GetComponentFlag<Components>()...);
    if (old != _componentFlags && Core::HasFlags(_componentFlags, ComponentFlags::TreeNode)) [[likely]] {
        get<TreeNode>().componentFlags = _componentFlags;
    }
}