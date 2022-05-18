/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Font
 */

#include "FontManager.hpp"

using namespace kF;

UI::Font::~Font(void) noexcept
{
    if (_manager)
        _manager->removeUnsafe(_index);
}
