/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Item
 */

#pragma once

#include <Kube/ECS/Base.hpp>
#include <Kube/Core/SmallVector.hpp>
#include <Kube/Core/UniquePtr.hpp>

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

    /** @brief Check if a component is a base component */
    template<typename Component>
    constexpr bool IsBaseItemComponent = std::is_same_v<Component, TreeNode> || std::is_same_v<Component, Area> || std::is_same_v<Component, Depth>;
}

/** @brief An item represents the base of any UI components
 *  This class and their derived are not efficient
 *  To draw complex drawing you should either batch within an item or create a new custom primitive */
class alignas_cacheline kF::UI::Item
{
public:
    /** @brief Small optimized children list */
    using Children = Core::SmallVector<ItemPtr, Core::CacheLineQuarterSize / sizeof(ItemPtr), UIAllocator>;


    /** @brief Destructor */
    virtual ~Item(void) noexcept;

    /** @brief Constructor */
    Item(void) noexcept;

    /** @brief Item is not copiable */
    Item(const Item &other) noexcept = delete;
    Item &operator=(const Item &other) noexcept = delete;


    /** @brief Get UI system */
    [[nodiscard]] inline UISystem &uiSystem(void) const noexcept { return *_uiSystem; }


    /** @brief Get the component flags property */
    [[nodiscard]] inline ComponentFlags componentFlags(void) const noexcept { return _componentFlags; }


    /** @brief Get the parent Item as ItemType */
    template<typename ItemType = Item>
        requires std::derived_from<ItemType, kF::UI::Item>
    [[nodiscard]] ItemType &parent(void) const noexcept;

    /** @brief Get the list of children */
    [[nodiscard]] inline const Children &children(void) const noexcept { return _children; }

    /** @brief Get child at index */
    template<typename ItemType = Item>
        requires std::derived_from<ItemType, kF::UI::Item>
    [[nodiscard]] inline ItemType &childAt(const std::uint32_t index) noexcept
        { return const_cast<ItemType &>(std::as_const(*this).childAt<ItemType>(index)); }
    template<typename ItemType = Item>
        requires std::derived_from<ItemType, kF::UI::Item>
    [[nodiscard]] inline const ItemType &childAt(const std::uint32_t index) const noexcept;


    /** @brief Add a child to item children list */
    template<typename Derived, typename ...Args>
        requires std::derived_from<Derived, kF::UI::Item> && std::is_constructible_v<Derived, Args...>
    inline Derived &addChild(Args &&...args) noexcept
        { return reinterpret_cast<Derived &>(addChild(DerivedItemPtr<Derived>::Make(std::forward<Args>(args)...))); }

    /** @brief Insert a child at position of item children list */
    template<typename Derived, typename ...Args>
        requires std::derived_from<Derived, kF::UI::Item> && std::is_constructible_v<Derived, Args...>
    inline Derived &insertChild(const std::uint32_t index, Args &&...args) noexcept
        { return reinterpret_cast<Derived &>(insertChild(index, DerivedItemPtr<Derived>::Make(std::forward<Args>(args)...))); }


    /** @brief Remove a child from children list using its address */
    void removeChild(const Item * const target) noexcept;

    /** @brief Remove a child from children list using its index */
    void removeChild(const std::uint32_t index) noexcept;

    /** @brief Remove a range of children */
    void removeChild(const std::uint32_t from, const std::uint32_t to) noexcept;


    /** @brief Remove all children */
    void clearChildren(void) noexcept;


    /** @brief Swap two children's position */
    void swapChild(const std::uint32_t source, const std::uint32_t output) noexcept;

    /** @brief Move children */
    inline void moveChild(const std::uint32_t source, const std::uint32_t output) noexcept
        { return moveChild(source, source + 1u, output); }

    /** @brief Move children [from, to[ into out */
    void moveChild(const std::uint32_t from, const std::uint32_t to, const std::uint32_t output) noexcept;


    /** @brief Check Item has 'Components' */
    template<typename ...Components>
        requires kF::UI::ComponentRequirements<Components...>
    [[nodiscard]] bool exists(void) const noexcept;


    /** @brief Attach components to Item */
    template<typename ...Components>
        requires kF::UI::ComponentRequirements<Components...>
    Item &attach(Components &&...components) noexcept;

    /** @brief Try to attach components to Item
     *  @note Any component is already attached will be overwrited */
    template<typename ...Components>
        requires kF::UI::ComponentRequirements<Components...>
    Item &tryAttach(Components &&...components) noexcept;

    /** @brief Try to update components of Item
     *  @note If a component doesn't exists, it is created */
    template<typename ...Functors>
    Item &tryAttach(Functors &&...functors) noexcept;


    /** @brief Detach components from Item */
    template<typename ...Components>
        requires kF::UI::ComponentRequirements<Components...>
    void dettach(void) noexcept;

    /** @brief Detach components from Item */
    template<typename ...Components>
        requires kF::UI::ComponentRequirements<Components...>
    void tryDettach(void) noexcept;


    /** @brief Get a component from Item using its type */
    template<typename Component>
    [[nodiscard]] inline Component &get(void) noexcept;
    template<typename Component>
    [[nodiscard]] inline const Component &get(void) const noexcept;


    /** @brief Check if an entity is hovered */
    [[nodiscard]] bool isHovered(void) const noexcept;

    /** @brief Check if an entity is drop hovered */
    [[nodiscard]] bool isDropHovered(void) const noexcept;


    /** @brief Delay a callback to the end of current tick */
    template<typename Callback>
    void delayToTickEnd(Callback &&callback) noexcept;


    /** @brief Unsafe entity getter
     *  @note You must not use this entity index to attach or dettach any components ! */
    [[nodiscard]] static inline ECS::Entity GetEntity(const Item &item) noexcept { return item._entity; }

protected:
    /** @brief Get an unsafe mutable list of children */
    [[nodiscard]] inline Children &childrenUnsafe(void) noexcept { return _children; }


private:
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

    /** @brief Dispatch dettach callbacks */
    template<typename ...Components>
    void onDettach(void) noexcept;

    // vtable pointer
    UISystem *_uiSystem {};
    Item *_parent {};
    ComponentFlags _componentFlags {};
    ECS::Entity _entity {};
    Children _children {};
};
static_assert_fit_cacheline(kF::UI::Item);


template<typename ItemType>
    requires std::derived_from<ItemType, kF::UI::Item>
inline ItemType &kF::UI::Item::parent(void) const noexcept
{
    const auto ptr = dynamic_cast<ItemType *>(_parent) != nullptr;
    kFEnsure(ptr, "UI::Item::parent<ParentType>: ParentType is not the type of this' parent");
    return *ptr;
}

template<typename ItemType>
    requires std::derived_from<ItemType, kF::UI::Item>
inline const ItemType &kF::UI::Item::childAt(const std::uint32_t index) const noexcept
{
    const auto ptr = dynamic_cast<const ItemType *>(_children.at(index).get());
    kFEnsure(ptr, "kF::UI::Item::childAt: Child at index '", index, "' is not derived from ItemType");
    return *ptr;
}