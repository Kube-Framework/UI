/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ItemListModel
 */

#include "ItemListModel.hpp"

template<typename ListModelType, typename Delegate>
inline void kF::UI::ItemListModel::setup(ListModelType &listModel, Delegate &&delegate) noexcept
{
    // Setup delegate
    _delegate = [delegate = std::forward<Delegate>(delegate)](ItemListModel &parent, void * const model, const std::uint32_t index) {
        using Decomposer = Core::FunctionDecomposerHelper<Delegate>;
        using DelegateItemType = std::remove_cvref_t<std::tuple_element_t<0, Decomposer::ArgsTuple>>;
        auto &child = parent.insertChild<Item>(index);
        delegate(child, (*reinterpret_cast<ListModelType * const>(model))[static_cast<ListModelType::Range>(index)]);
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