/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite
 */

#include "App.hpp"
#include "UISystem.hpp"

using namespace kF;

UI::Sprite::~Sprite(void) noexcept
{
    if (_manager)
        _manager->decrementRefCount(_index);
}

UI::Sprite::Sprite(const std::string_view &path) noexcept
    : Sprite(App::Get().uiSystem().spriteManager().add(path))
{
}

UI::Sprite::Sprite(const Sprite &other) noexcept
    : _manager(other._manager), _index(other._index)
{
    _manager->incrementRefCount(_index);
}

UI::Sprite &UI::Sprite::operator=(const Sprite &other) noexcept
{
    if (_manager)
        _manager->decrementRefCount(_index);
    _manager = other._manager;
    _index = other._index;
    _manager->incrementRefCount(_index);
    return *this;
}