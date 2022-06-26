/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ItemListModel
 */

#include <Kube/Core/Log.hpp>

#include "ItemListModel.hpp"

template<typename ListModelType, typename Delegate, typename ...Args>
inline void kF::UI::ItemListModel::setup(ListModelType &listModel, Delegate &&delegate, Args &&...args) noexcept
{
    // Setup delegate
    _delegate = [delegate = std::forward<Delegate>(delegate), ...args = std::forward<Args>(args)](ItemListModel &parent, void * const model, const std::uint32_t index) {
        // Query first argument of the delegate using FunctionDecomposer
        using Decomposer = Core::FunctionDecomposerHelper<Delegate>;
        using ModelData = std::remove_cvref_t<std::tuple_element_t<0, typename Decomposer::ArgsTuple>>;

        ModelData *child {};
        auto &modelData = (*reinterpret_cast<ListModelType * const>(model))[static_cast<ListModelType::Range>(index)];
        if constexpr (std::is_constructible_v<ModelData, typename ListModelType::Type &, Args...>)
            child = &parent.insertChild<ModelData>(index, modelData, args...);
        else if constexpr (std::is_constructible_v<ModelData, Args..., typename ListModelType::Type &>)
            child = &parent.insertChild<ModelData>(index, args..., modelData);
        else
            child = &parent.insertChild<ModelData>(index);
        delegate(*child, modelData);
    };

    // Setup list model & connect to its event dispatcher
    _listModel = &listModel;
    _disconnect = [](void * const listModel, const std::uint32_t handle) noexcept {
        reinterpret_cast<ListModelType * const>(listModel)->eventDispatcher().remove(handle);
    };
    _dispatcherHandle = listModel.eventDispatcher().add([this](const auto &event) { onListModelEvent(event); });
    _modelSize = listModel.size();

    // Insert list model items
    if (_modelSize) [[likely]]
        onInsert(ListModelEvent::Insert { 0, _modelSize });
}

template<typename ItemType, typename ListModelType, typename ...Args>
inline void kF::UI::ItemListModel::setup(ListModelType &listModel, Args &&...args) noexcept
{
    setup(listModel, [](ItemType &, const ListModelType::Type &) {}, std::forward<Args>(args)...);
}