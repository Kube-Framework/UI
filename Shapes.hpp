#pragma once

#include "Base.hpp"

#include <Kube/UI/Components.hpp>

namespace Lexo::Shapes
{
    // --- Rectangle based shapes ---

    /** @brief Rectangle shape */
    struct Rectangle
    {
        /** @brief Draw a colored rectangle with an optional radius */
        void operator()(
            UI::Painter &painter,
            const UI::Area &area,
            const UI::Color color,
            const UI::Radius &radius = {}
        ) noexcept;
    };
}
