/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite
 */

#include "Sprite.hpp"
#include "SpriteManager.hpp"

using namespace kF;

UI::Sprite::~Sprite(void) noexcept
{
    if (_manager)
        _manager->decrementRefCount(_index);
}

UI::Sprite::Sprite(SpriteManager &manager, const std::string_view &path, const float removeDelaySeconds) noexcept
    : Sprite(manager.add(path, removeDelaySeconds))
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

UI::Size UI::Sprite::spriteSize(void) const noexcept
{
    return _manager->spriteSizeAt(_index);
}