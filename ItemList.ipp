/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ItemList
 */

#include <Kube/Core/Log.hpp>

#include "ItemList.hpp"

template<typename ListModelType, typename Delegate, typename ...Args>
inline void kF::UI::ItemList::setup(ListModelType &listModel, Delegate &&delegate, Args &&...args) noexcept
{
    // Query delegate's first argument
    using ItemType = ItemListDelegateType<Delegate>;

    // Setup delegate
    _delegate = [delegate = std::forward<Delegate>(delegate), ...args = std::forward<Args>(args)](ItemList &parent, void * const model, const std::uint32_t index) {
        ItemType *child {};
        auto &modelData = (*reinterpret_cast<ListModelType * const>(model))[static_cast<ListModelType::Range>(index)];

        // Static assertions
        static_assert(
            ItemListConstructible<ItemType, Args...>
            || ItemListConstructible<ItemType, typename ListModelType::Type &, Args...>
            || ItemListConstructible<ItemType, Args..., typename ListModelType::Type &>
            || [] {
                if constexpr (Core::IsDereferencable<typename ListModelType::Type>) {
                    return ItemListConstructible<ItemType, decltype(*modelData), Args...>
                        || ItemListConstructible<ItemType, Args..., decltype(*modelData)>;
                } else
                    return false;
            }()
            || ItemListConstructible<ItemType, Args...>,
            "ItemList::setup: Child item is not constructible"
        );
        static_assert(
            std::is_invocable_v<Delegate, decltype(*child), decltype(modelData)>
            || [] {
                if constexpr (Core::IsDereferencable<typename ListModelType::Type>) {
                    return std::is_invocable_v<Delegate, decltype(*child), decltype(*modelData)>;
                } else
                    return false;
            }()
            || std::is_invocable_v<Delegate, decltype(*child)>,
            "ItemList::setup: Delegate is not invocable"
        );

        // #1 args...
        if constexpr (ItemListConstructible<ItemType, Args...>) {
            child = &parent.insertChild<ItemType>(index, Internal::ForwardArg(args)...);
        // #2 model, args...
        } if constexpr (ItemListConstructible<ItemType, typename ListModelType::Type &, Args...>) {
            child = &parent.insertChild<ItemType>(index, modelData, Internal::ForwardArg(args)...);
        // #3 args..., model
        } else if constexpr (ItemListConstructible<ItemType, Args..., typename ListModelType::Type &>) {
            child = &parent.insertChild<ItemType>(index, Internal::ForwardArg(args)..., modelData);
        // #4 Model is dereferencable (ex: raw / unique / shared pointers)
        } else if constexpr (Core::IsDereferencable<typename ListModelType::Type>) {
            // #4A *model, args...
            if constexpr (ItemListConstructible<ItemType, decltype(*modelData), Args...>) {
                child = &parent.insertChild<ItemType>(index, *modelData, Internal::ForwardArg(args)...);
            // #4B args..., *model
            } else if constexpr (ItemListConstructible<ItemType, Args..., decltype(*modelData)>) {
                child = &parent.insertChild<ItemType>(index, Internal::ForwardArg(args)..., *modelData);
            }
        }

        if constexpr (std::is_invocable_v<Delegate, decltype(*child), decltype(modelData)>)
            delegate(*child, modelData);
        else if constexpr (std::is_invocable_v<Delegate, decltype(*child), decltype(*modelData)>)
            delegate(*child, *modelData);
        else
            delegate(*child);
    };

    // Setup list model & connect to its event dispatcher
    _listModel = &listModel;
    _dispatcherSlot = listModel.eventDispatcher().template add<&ItemList::onListModelEvent>(this);
    _modelSize = 0;

    // Insert list model items
    if (const auto modelSize = listModel.size(); modelSize) [[likely]]
        onInsert(ListModelEvent::Insert { 0, modelSize });
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