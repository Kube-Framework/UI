/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Font
 */

#include "Font.hpp"

inline kF::UI::Font::Font(Font &&other) noexcept
{
    _manager = other._manager;
    _index = other._index;
    other._manager = nullptr;
}

inline kF::UI::Font &kF::UI::Font::operator=(Font &&other) noexcept
{
    std::swap(_manager, other._manager);
    std::swap(_index, other._index);
    return *this;
}
