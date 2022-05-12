/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UIObject
 */

#include "App.hpp"
#include "UISystem.hpp"
#include "UIObject.hpp"

using namespace kF;

UI::UISystem &UI::UIObject::Parent(void) noexcept
{
    return App::Get().executor().getSystem<UISystem>();
}
