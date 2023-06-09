/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Base
 */

#include "Base.hpp"

#include <Kube/Core/SmallString.hpp>
#include <Kube/Core/Vector.hpp>

#include <SDL2/SDL.h>

#include <tinyfiledialogs/tinyfiledialogs.h>

#include <ostream>


using namespace kF;

bool UI::OpenUrl(const std::string_view &url) noexcept
{
    std::string str(url);
    return SDL_OpenURL(str.c_str()) == 0;
}

std::string_view UI::OpenSingleFilePicker(
    const std::string_view &title,
    const std::string_view &defaultPath,
    const Core::IteratorRange<const std::string_view *> &filters
) noexcept
{
    Core::SmallString<UIAllocator> titleStr(title);
    Core::SmallString<UIAllocator> defaultPathStr(defaultPath);
    Core::Vector<Core::SmallString<UIAllocator>, UIAllocator> filtersStrs(filters.begin(), filters.end());
    Core::Vector<const char *, UIAllocator> filtersCstrs(filtersStrs.begin(), filtersStrs.end(), [](const Core::SmallString<UIAllocator> &str) { return str.c_str(); });

    return std::string_view(::tinyfd_openFileDialog(
        titleStr.c_str(),
        defaultPathStr.c_str(),
        static_cast<int>(filtersCstrs.size()),
        filtersCstrs.data(),
        nullptr,
        false
    ));
}

namespace kF::UI
{
    std::ostream &operator<<(std::ostream &lhs, const kF::UI::Color &rhs) noexcept
    {
        return lhs << "(r: " << std::uint32_t(rhs.r) << ", g: " << std::uint32_t(rhs.g) << ", b: " << std::uint32_t(rhs.b) << ", a: " << std::uint32_t(rhs.a) << ')';
    }

    std::ostream &operator<<(std::ostream &lhs, const kF::UI::Point &rhs) noexcept
    {
        return lhs << "(x: " << rhs.x << ", y: " << rhs.y << ')';
    }

    std::ostream &operator<<(std::ostream &lhs, const kF::UI::Size &rhs) noexcept
    {
        return lhs << "(width: " << rhs.width << ", height: " << rhs.height << ')';
    }

    std::ostream &operator<<(std::ostream &lhs, const kF::UI::Area &rhs) noexcept
    {
        return lhs << "{ pos: " << rhs.pos << ", size: " << rhs.size << " }";
    }

    std::ostream &operator<<(std::ostream &lhs, const kF::UI::Constraints &rhs) noexcept
    {
        return lhs << "{ minSize: " << rhs.minSize << ", maxSize: " << rhs.maxSize << " }";
    }

    std::ostream &operator<<(std::ostream &lhs, const kF::UI::Padding &rhs) noexcept
    {
        return lhs << "{ left: " << rhs.left << ", right: " << rhs.right << ", top: " << rhs.top << ", bottom: " << rhs.bottom << " }";
    }

    std::ostream &operator<<(std::ostream &lhs, const kF::UI::Radius &rhs) noexcept
    {
        return lhs << "{ topLeft: " << rhs.topLeft << ", topRight: " << rhs.topRight << ", bottomLeft: " << rhs.bottomLeft << ", bottomRight: " << rhs.bottomRight << " }";
    }
}