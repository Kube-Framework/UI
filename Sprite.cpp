/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Sprite
 */

#include "SpriteManager.hpp"

using namespace kF;

UI::Sprite::~Sprite(void) noexcept
{
    if (_manager)
        _manager->removeUnsafe(_index);
}
