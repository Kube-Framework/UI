/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#include "Components.hpp"
#include "Events.hpp"

using namespace kF;

UI::EventFlags UI::DropEventArea::event(const TypeHash typeHash, const void * const data, const DropEvent &event, const Area &area) noexcept
{
    auto index = 0u;
    for (const auto type : dropTypes) {
        if (type != typeHash) [[likely]] {
            ++index;
        } else {
            return dropFunctors[index](data, event, area);
            break;
        }
    }
    return EventFlags::Propagate;
}