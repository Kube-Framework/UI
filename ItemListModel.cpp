/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ItemListModel
 */

#include <Kube/Core/Assert.hpp>

#include "ItemListModel.hpp"

using namespace kF;

void kF::UI::ItemListModel::reset(void) noexcept
{
    // Disconnect from dispatcher
    if (_listModel)
        _disconnect(_listModel, _dispatcherHandle);

    // Reset members
    _delegate.release();
    _listModel = nullptr;
    _modelSize = 0u;

    // Remove all previous children
    clearChildren();
}

void UI::ItemListModel::onListModelEvent(const ListModelEvent &event) noexcept
{
    switch (event.type) {
    case ListModelEvent::Type::Insert:
        return onInsert(event.insert);
    case ListModelEvent::Type::Erase:
        return onErase(event.erase);
    case ListModelEvent::Type::Update:
        return onUpdate(event.update);
    case ListModelEvent::Type::Resize:
        return onResize(event.resize);
    case ListModelEvent::Type::Move:
        return onMove(event.move);
    default:
        break;
    }
}

void UI::ItemListModel::onInsert(const ListModelEvent::Insert &data) noexcept
{
    kFAssert(data.from < data.to,
        "UI::ItemListModel::onInsert: Invalid event range (", data.from, ", ", data.to, ")");

    _modelSize += data.to - data.from;
    for (auto it = data.from; it != data.to; ++it) {
        _delegate(*this, _listModel, it);
    }
}

void UI::ItemListModel::onErase(const ListModelEvent::Erase &data) noexcept
{
    kFAssert(data.from < data.to,
        "UI::ItemListModel::onErase: Invalid event range (", data.from, ", ", data.to, ")");

    _modelSize -= data.to - data.from;
    removeChild(data.from, data.to);
}

void UI::ItemListModel::onUpdate(const ListModelEvent::Update &data) noexcept
{
    kFAssert(data.from < data.to,
        "UI::ItemListModel::onUpdate: Invalid event range (", data.from, ", ", data.to, ")");

    onErase(ListModelEvent::Erase { data.from, data.to });
    onInsert(ListModelEvent::Insert { data.from, data.to });
}

void UI::ItemListModel::onResize(const ListModelEvent::Resize &data) noexcept
{
    onErase(ListModelEvent::Erase { 0, _modelSize });
    onInsert(ListModelEvent::Insert { 0, data.count });
}

void UI::ItemListModel::onMove(const ListModelEvent::Move &data) noexcept
{
    kFAssert(data.from < data.to && (data.out < data.from || data.out > data.to),
        "UI::ItemListModel::onMove: Invalid event range (", data.from, ", ", data.to, ")");
    moveChild(data.from, data.to, data.out);
}