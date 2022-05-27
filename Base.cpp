/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Base
 */

#include <ostream>

#include "Base.hpp"

using namespace kF;

std::wostream &operator<<(std::wostream &lhs, const UI::Color &rhs) noexcept
{
    return lhs << "(r: " << rhs.r << ", g: " << rhs.g << ", b: " << rhs.b << ", a: " << rhs.a << ')';
}

std::wostream &operator<<(std::wostream &lhs, const UI::Point &rhs) noexcept
{
    return lhs << "(x: " << rhs.x << ", y: " << rhs.y << ')';
}

std::wostream &operator<<(std::wostream &lhs, const UI::Size &rhs) noexcept
{
    return lhs << "(width: " << rhs.width << ", height: " << rhs.height << ')';
}

std::wostream &operator<<(std::wostream &lhs, const UI::Area &rhs) noexcept
{
    return lhs << "{ pos: " << rhs.pos << ", size: " << rhs.size << " }";
}

std::wostream &operator<<(std::wostream &lhs, const UI::Constraints &rhs) noexcept
{
    return lhs << "{ minSize: " << rhs.minSize << ", maxSize: " << rhs.maxSize << " }";
}

std::wostream &operator<<(std::wostream &lhs, const UI::Padding &rhs) noexcept
{
    return lhs << "{ left: " << rhs.left << ", right: " << rhs.right << ", top: " << rhs.top << ", bottom: " << rhs.bottom << " }";
}

std::wostream &operator<<(std::wostream &lhs, const UI::Radius &rhs) noexcept
{
    return lhs << "{ topLeft: " << rhs.topLeft << ", topRight: " << rhs.topRight << ", bottomLeft: " << rhs.bottomLeft << ", bottomRight: " << rhs.bottomRight << " }";
}