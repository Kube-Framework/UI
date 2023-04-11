/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Font
 */

#include "App.hpp"
#include "UISystem.hpp"

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

UI::Pixel UI::Font::spaceWidth(void) const noexcept
{
    return _manager->spaceWidthAt(_index);
}

UI::Pixel UI::Font::lineHeight(void) const noexcept
{
    return _manager->lineHeightAt(_index);
}

UI::Size UI::Font::computeTextMetrics(const std::string_view &text, const Pixel spacesPerTab) const noexcept
{
    return _manager->computeTextMetrics(_index, text, spacesPerTab);
}
