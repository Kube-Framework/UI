/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#include "Components.hpp"
#include "Events.hpp"

void kF::UI::DropEventArea::onEvent(const TypeHash typeHash, const void * const data, const DropEvent &event, const Area &area) noexcept
{
    if (const auto it = _dropTypes.find(typeHash); it != _dropTypes.end())
        onEventUnsafe(data, event, area, Core::Distance<std::uint32_t>(_dropTypes.begin(), it));
}

void kF::UI::DropEventArea::onEventUnsafe(const void * const data, const DropEvent &event, const Area &area,
        const std::uint32_t dropTypeIndex) noexcept
{
    _hovered = event.type == DropEvent::Type::Enter;
    _dropFunctors[dropTypeIndex](data, event, area);
}