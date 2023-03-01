#include "Shapes.hpp"

#include <Kube/UI/UISystem.hpp>
#include <Kube/UI/RectangleProcessor.hpp>
#include <Kube/UI/TextProcessor.hpp>

using namespace Lexo;

void Shapes::Rectangle::operator()(
    UI::Painter &painter,
    const UI::Area &area,
    const UI::Color color,
    const UI::Radius &radius
) noexcept
{
    painter.draw(UI::Rectangle {
        .area = area,
        .radius = radius,
        .color = color,
        .edgeSoftness = 2.0f
    });
}