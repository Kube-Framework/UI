/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite
 */

#include "Sprite.hpp"

inline kF::UI::Sprite::Sprite(Sprite &&other) noexcept
{
    _manager = other._manager;
    _index = other._index;
    other._manager = nullptr;
}

inline kF::UI::Sprite &kF::UI::Sprite::operator=(Sprite &&other) noexcept
{
    std::swap(_manager, other._manager);
    std::swap(_index, other._index);
    return *this;
}
