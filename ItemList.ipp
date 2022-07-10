/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ItemList
 */

#include <Kube/Core/Log.hpp>

#include "ItemList.hpp"

template<typename ListModelType, typename Delegate, typename ...Args>
    requires kF::UI::ItemListDelegateRequirements<ListModelType, Delegate, Args...>
inline void kF::UI::ItemList::setup(ListModelType &listModel, Delegate &&delegate, Args &&...args) noexcept
{
    // Query delegate's first argument
    using ItemType = ItemListDelegateType<Delegate>;

    // Setup delegate
    _delegate = [delegate = std::forward<Delegate>(delegate), ...args = std::forward<Args>(args)](ItemList &parent, void * const model, const std::uint32_t index) {
        ItemType *child {};
        auto &modelData = (*reinterpret_cast<ListModelType * const>(model))[static_cast<ListModelType::Range>(index)];

        // #1 model, args...
        if constexpr (std::is_constructible_v<ItemType, typename ListModelType::Type &, Args...>) {
            child = &parent.insertChild<ItemType>(index, modelData, args...);
        // #2 args..., model
        } else if constexpr (std::is_constructible_v<ItemType, Args..., typename ListModelType::Type &>) {
            child = &parent.insertChild<ItemType>(index, args..., modelData);
        // #3 Model is dereferencable (ex: raw / unique / shared pointers)
        } else if constexpr (Core::IsDereferencable<typename ListModelType::Type>) {
            // #3A *model, args...
            if constexpr (std::is_constructible_v<ItemType, decltype(*modelData), Args...>) {
                child = &parent.insertChild<ItemType>(index, *modelData, args...);
            // #3B args..., *model
            } else if constexpr (std::is_constructible_v<ItemType, Args..., decltype(*modelData)>) {
                child = &parent.insertChild<ItemType>(index, args..., *modelData);
            }
        // #4 args...
        } else if constexpr (std::is_constructible_v<ItemType, Args...>) {
            child = &parent.insertChild<ItemType>(index, args...);
        }
        delegate(*child, modelData);
    };

    // Setup list model & connect to its event dispatcher
    _listModel = &listModel;
    _disconnect = [](void * const listModel, const std::uint32_t handle) noexcept {
        reinterpret_cast<ListModelType * const>(listModel)->eventDispatcher().remove(handle);
    };
    _dispatcherHandle = listModel.eventDispatcher().template add<&ItemList::onListModelEvent>(this);
    _modelSize = listModel.size();

    // Insert list model items
    if (_modelSize) [[likely]]
        onInsert(ListModelEvent::Insert { 0, _modelSize });
}

template<typename ItemType, typename ListModelType, typename ...Args>
    requires std::derived_from<ItemType, kF::UI::Item>
inline void kF::UI::ItemList::setup(ListModelType &listModel, Args &&...args) noexcept
{
    constexpr auto NullDelegate = [](ItemType &, const typename ListModelType::Type &) {};

    setup(listModel, NullDelegate, std::forward<Args>(args)...);
}

template<typename Functor>
inline void kF::UI::ItemList::traverseItemList(Functor &&functor) noexcept
{
    // Query functor's first argument
    using ItemType = ItemListDelegateType<Functor>;
    static_assert(std::is_base_of_v<Item, ItemType>, "UI::ItemList::traverseItemList: Functor's first argument is not an Item-derived class");

    for (auto &child : children()) {
        functor(*reinterpret_cast<ItemType *>(const_cast<Item *>(child.get())));
    }
}