/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Font
 */

#include "FontManager.hpp"

using namespace kF;

UI::Font::~Font(void) noexcept
{
    if (_manager)
        _manager->decrementRefCount(_index);
}

UI::Font::Font(const Font &other) noexcept
    : _manager(other._manager), _index(other._index)
{
    _manager->incrementRefCount(_index);
}

UI::Font &UI::Font::operator=(const Font &other) noexcept
{
    if (_manager)
        _manager->decrementRefCount(_index);
    _manager = other._manager;
    _index = other._index;
    _manager->incrementRefCount(_index);
    return *this;
}
