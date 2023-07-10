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

    static_assert(
        (std::is_copy_constructible_v<std::remove_cvref_t<Args>> && ...),
        "UI::ItemList::setup: Arguments passed must be copy-constructible, use std::reference_wrapper if you need a reference"
    );

    // Setup delegate
    _delegate = [delegate = std::forward<Delegate>(delegate), ...args = std::forward<Args>(args)](ItemList &parent, const void * const opaqueModel, const std::uint32_t index) {
        // Query model data
        const auto model = [opaqueModel] {
            if constexpr (std::is_const_v<ListModelType>)
                return opaqueModel;
            else
                return const_cast<void *>(opaqueModel);
        }();
        auto &modelData = (*reinterpret_cast<ListModelType *>(model))[static_cast<ListModelType::Range>(index)];
        using ModelDataRef = decltype(modelData);
        using ModelData = std::remove_reference_t<ModelDataRef>;

        // Static assertions
        static_assert(
            ItemListConstructible<ItemType, Args...>
            || ItemListConstructible<ItemType, ModelDataRef, Args...>
            || ItemListConstructible<ItemType, Args..., ModelDataRef>
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

        ItemType *child {};
        // #1 args...
        if constexpr (ItemListConstructible<ItemType, Args...>) {
            child = &parent.insertChild<ItemType>(index, Internal::ForwardArg(args)...);
        // #2 model, args...
        } if constexpr (ItemListConstructible<ItemType, ModelDataRef, Args...>) {
            child = &parent.insertChild<ItemType>(index, modelData, Internal::ForwardArg(args)...);
        // #3 args..., model
        } else if constexpr (ItemListConstructible<ItemType, Args..., ModelDataRef>) {
            child = &parent.insertChild<ItemType>(index, Internal::ForwardArg(args)..., modelData);
        // #4 Model is dereferencable (ex: raw / unique / shared pointers)
        } else if constexpr (Core::IsDereferencable<ModelData>) {
            // #4A *model, args...
            if constexpr (ItemListConstructible<ItemType, decltype(*modelData), Args...>) {
                child = &parent.insertChild<ItemType>(index, *modelData, Internal::ForwardArg(args)...);
            // #4B args..., *model
            } else if constexpr (ItemListConstructible<ItemType, Args..., decltype(*modelData)>) {
                child = &parent.insertChild<ItemType>(index, Internal::ForwardArg(args)..., *modelData);
            }
        }

        constexpr bool IsInvocableModelIndex = std::is_invocable_v<Delegate, decltype(*child), decltype(modelData), std::uint32_t>;
        constexpr bool IsInvocableModel = std::is_invocable_v<Delegate, decltype(*child), decltype(modelData)>;
        constexpr bool IsInvocableDerefencedModelIndex = [] {
            if constexpr (Core::IsDereferencable<ModelData>) {
                return std::is_invocable_v<Delegate, decltype(*child), decltype(*modelData), std::uint32_t>;
            } else
                return false;
        }();
        constexpr bool IsInvocableDerefencedModel = [] {
            if constexpr (Core::IsDereferencable<ModelData>) {
                return std::is_invocable_v<Delegate, decltype(*child), decltype(*modelData)>;
            } else
                return false;
        }();
        constexpr bool IsInvocableIndex = std::is_invocable_v<Delegate, decltype(*child), std::uint32_t>;
        constexpr bool IsInvocableVoid = std::is_invocable_v<Delegate, decltype(*child)>;
        static_assert(IsInvocableModelIndex || IsInvocableModel || IsInvocableDerefencedModelIndex || IsInvocableDerefencedModel || IsInvocableIndex || IsInvocableVoid,
            "ItemList::setup: Delegate is not invocable");

        if constexpr (IsInvocableModelIndex)
            delegate(*child, modelData, index);
        else if constexpr (IsInvocableDerefencedModel)
            delegate(*child, *modelData, index);
        else if constexpr (IsInvocableModel)
            delegate(*child, modelData);
        else if constexpr (IsInvocableDerefencedModel)
            delegate(*child, *modelData);
        else if constexpr (IsInvocableIndex)
            delegate(*child, index);
        else
            delegate(*child);
    };

    // Setup list model & connect to its event dispatcher
    _listModel = &listModel;
    _dispatcherSlot = listModel.dispatcher().template add<&ItemList::onListModelEvent>(this);
    _modelSize = 0;

    // Insert list model items
    if (const auto modelSize = listModel.size(); modelSize) [[likely]]
        onInsert(ListModelEvent::Insert { 0, modelSize });
}

template<typename ItemType, typename ListModelType, typename ...Args>
    requires std::derived_from<ItemType, kF::UI::Item>
inline void kF::UI::ItemList::setup(ListModelType &listModel, Args &&...args) noexcept
{
    constexpr auto NullDelegate = [](ItemType &) {};

    setup(listModel, NullDelegate, std::forward<Args>(args)...);
}

template<typename Functor>
inline void kF::UI::ItemList::traverseItemList(Functor &&functor) noexcept
{
    // Query functor's first argument
    using ItemType = ItemListDelegateType<Functor>;

    for (auto index = 0u; index != _modelSize; ++index) {
        functor(childAt<ItemType>(index));
    }
}