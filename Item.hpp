/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Item
 */

#pragma once

#include <Kube/ECS/Base.hpp>
#include <Kube/Core/SmallVector.hpp>

#include "Components.hpp"

namespace kF::UI
{
    class Item;
    class UISystem;

    /** @brief Unique pointer over items */
    using ItemPtr = Core::UniquePtr<Item, UIAllocator>;

    /** @brief Unique pointer over items */
    template<typename Derived>
        requires std::derived_from<Derived, Item>
    using DerivedItemPtr = Core::UniquePtr<Derived, UIAllocator>;
}

/** @brief An item represents the base of any UI components
 *  This class and their derived are not efficient
 *  To draw complex drawing you should either batch within an item or create a new custom primitive */
class alignas_cacheline kF::UI::Item
{
public:
    /** @brief Small optimized children list */
    using Children = Core::TinySmallVector<ItemPtr, Core::CacheLineQuarterSize / sizeof(ItemPtr), UIAllocator>;


    /** @brief Destructor */
    virtual ~Item(void) noexcept;

    /** @brief Constructor */
    Item(void) noexcept;


    /** @brief Get UI system */
    [[nodiscard]] inline UISystem &uiSystem(void) const noexcept { return *_uiSystem; }


    /** @brief Get the component flags property */
    [[nodiscard]] inline ComponentFlags componentFlags(void) const noexcept { return _componentFlags; }


    /** @brief Get the list of children */
    [[nodiscard]] inline const Children &children(void) noexcept { return _children; }

    /** @brief Add a child to item children list */
    template<typename Derived, typename ...Args>
        requires std::derived_from<Derived, kF::UI::Item>
    inline Derived &addChild(Args &&...args) noexcept
        { return reinterpret_cast<Derived &>(addChild(DerivedItemPtr<Derived>::Make(std::forward<Args>(args)...))); }

    /** @brief Insert a child at position of item children list */
    template<typename Derived, typename ...Args>
        requires std::derived_from<Derived, kF::UI::Item>
    inline Derived &insertChild(const std::uint32_t index, Args &&...args) noexcept
        { return reinterpret_cast<Derived &>(insertChild(index, DerivedItemPtr<Derived>::Make(std::forward<Args>(args)...))); }


    /** @brief Remove a child from children list using its address */
    void removeChild(const Item * const target) noexcept;

    /** @brief Remove a child from children list using its index */
    void removeChild(const std::uint32_t index) noexcept;

    /** @brief Swap two children's position */
    void swapChildren(const std::uint32_t source, const std::uint32_t destination) noexcept
        { std::swap(_children[source], _children[destination]); }


    /** @brief Attach components to Item */
    template<typename ...Components>
        requires ComponentRequirements<Components...>
    Item &attach(Components &&...components) noexcept;

    /** @brief Try to attach components to Item
     *  @note Any component is already attached will be overwrited */
    template<typename ...Components>
        requires ComponentRequirements<Components...>
    Item &attachUpdate(Components &&...components) noexcept;

    /** @brief Try to update components of Item
     *  @note If a component doesn't exists, it is created */
    template<typename ...Functors>
    Item &attachUpdate(Functors &&...functors) noexcept;


    /** @brief Detach components from Item */
    template<typename ...Components>
        requires ComponentRequirements<Components...>
    void dettach(void) noexcept;

    /** @brief Detach components from Item */
    template<typename ...Components>
        requires ComponentRequirements<Components...>
    void tryDettach(void) noexcept;


    /** @brief Get a component from Item using its type */
    template<typename Component>
    [[nodiscard]] inline Component &get(void) noexcept;
    template<typename Component>
    [[nodiscard]] inline const Component &get(void) const noexcept;


    /** @brief Unsafe entity getter
     *  @note You must not use this entity index to attach or dettach any components ! */
    [[nodiscard]] static inline ECS::Entity GetEntity(const Item &item) noexcept { return item._entity; }

private:
    // vtable pointer
    UISystem *_uiSystem {};
    Item *_parent {};
    ComponentFlags _componentFlags {};
    ECS::Entity _entity {};
    Children _children {};


    /** @brief Add a child to item children list */
    Item &addChild(ItemPtr &&item) noexcept;

    /** @brief Insert a child to item children list */
    Item &insertChild(const std::uint32_t index, ItemPtr &&item) noexcept;

    /** @brief Mark components as being attached */
    template<typename ...Components>
    inline void markComponents(void) noexcept;

    /** @brief Unmark components as being dettached */
    template<typename ...Components>
    inline void unmarkComponents(void) noexcept;
};
static_assert_fit_cacheline(kF::UI::Item);
