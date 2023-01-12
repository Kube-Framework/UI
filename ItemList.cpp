/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ItemList
 */

#include <Kube/Core/Assert.hpp>

#include "ItemList.hpp"

using namespace kF;

void UI::ItemList::reset(void) noexcept
{
    // Reset members
    _delegate.release();
    _listModel = nullptr;
    _dispatcherSlot = {};
    _modelSize = 0u;

    // Remove all previous children
    clearChildren();
}

void UI::ItemList::onListModelEvent(const ListModelEvent &event) noexcept
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

void UI::ItemList::onInsert(const ListModelEvent::Insert &data) noexcept
{
    kFAssert(data.from < data.to,
        "UI::ItemList::onInsert: Invalid event range (", data.from, ", ", data.to, ")");

    _modelSize += data.to - data.from;
    for (auto it = data.from; it != data.to; ++it) {
        _delegate(*this, _listModel, it);
    }
}

void UI::ItemList::onErase(const ListModelEvent::Erase &data) noexcept
{
    kFAssert(data.from < data.to,
        "UI::ItemList::onErase: Invalid event range (", data.from, ", ", data.to, ")");

    _modelSize -= data.to - data.from;
    removeChild(data.from, data.to);
}

void UI::ItemList::onUpdate(const ListModelEvent::Update &data) noexcept
{
    kFAssert(data.from < data.to,
        "UI::ItemList::onUpdate: Invalid event range (", data.from, ", ", data.to, ")");

    onErase(ListModelEvent::Erase { data.from, data.to });
    onInsert(ListModelEvent::Insert { data.from, data.to });
}

void UI::ItemList::onResize(const ListModelEvent::Resize &data) noexcept
{
    if (_modelSize)
        onErase(ListModelEvent::Erase { 0, _modelSize });
    if (data.count)
        onInsert(ListModelEvent::Insert { 0, data.count });
}

void UI::ItemList::onMove(const ListModelEvent::Move &data) noexcept
{
    kFAssert(data.from < data.to && (data.out < data.from || data.out >= data.to),
        "UI::ItemList::onMove: Invalid event range [", data.from, ", ", data.to, "[ -> ", data.out);
    moveChild(data.from, data.to, data.out);
}