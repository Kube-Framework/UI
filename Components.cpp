/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#include "Components.hpp"
#include "Events.hpp"

void kF::UI::DropEventArea::onEvent(const TypeHash typeHash, const void * const data, const DropEvent &event, const Area &area) noexcept
{
    auto index = 0u;
    for (const auto type : _types) {
        if (type != typeHash) [[likely]] {
            ++index;
        } else {
            _hovered = event.type == DropEvent::Type::Enter;
            _functors[index](data, event, area);
            break;
        }
    }
}