/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Base
 */

#include "Base.hpp"

namespace kF::UI
{
    /** @brief Explicit conversions */
    [[nodiscard]] constexpr Size Point::toSize(void) const noexcept
        { return Size(x, y); }
    [[nodiscard]] constexpr Point Size::toPoint(void) const noexcept
        { return Point(width, height); }

    /** @brief Point binary operators */
    [[nodiscard]] constexpr Point operator+(const Point &lhs, const Point &rhs) noexcept
        { return Point(lhs.x + rhs.x, lhs.y + rhs.y); }
    [[nodiscard]] constexpr Point operator-(const Point &lhs, const Point &rhs) noexcept
        { return Point(lhs.x - rhs.x, lhs.y - rhs.y); }
    [[nodiscard]] constexpr Point operator*(const Point &lhs, const Point &rhs) noexcept
        { return Point(lhs.x * rhs.x, lhs.y * rhs.y); }
    [[nodiscard]] constexpr Point operator/(const Point &lhs, const Point &rhs) noexcept
        { return Point(lhs.x / rhs.x, lhs.y / rhs.y); }

    /** @brief Point binary assignments */
    constexpr Point &operator+=(Point &lhs, const Point &rhs) noexcept
        { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
    constexpr Point &operator-=(Point &lhs, const Point &rhs) noexcept
        { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
    constexpr Point &operator*=(Point &lhs, const Point &rhs) noexcept
        { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
    constexpr Point &operator/=(Point &lhs, const Point &rhs) noexcept
        { lhs.x /= rhs.x; lhs.y /= rhs.y; return lhs; }


    /** @brief Point and Pixel binary operators */
    [[nodiscard]] constexpr Point operator+(const Point &lhs, const Pixel rhs) noexcept
        { return Point(lhs.x + rhs, lhs.y + rhs); }
    [[nodiscard]] constexpr Point operator-(const Point &lhs, const Pixel rhs) noexcept
        { return Point(lhs.x - rhs, lhs.y - rhs); }
    [[nodiscard]] constexpr Point operator*(const Point &lhs, const Pixel rhs) noexcept
        { return Point(lhs.x * rhs, lhs.y * rhs); }
    [[nodiscard]] constexpr Point operator/(const Point &lhs, const Pixel rhs) noexcept
        { return Point(lhs.x / rhs, lhs.y / rhs); }

    /** @brief Point and Pixel binary assignments */
    constexpr Point &operator+=(Point &lhs, const Pixel rhs) noexcept
        { lhs.x += rhs; lhs.y += rhs; return lhs; }
    constexpr Point &operator-=(Point &lhs, const Pixel rhs) noexcept
        { lhs.x -= rhs; lhs.y -= rhs; return lhs; }
    constexpr Point &operator*=(Point &lhs, const Pixel rhs) noexcept
        { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
    constexpr Point &operator/=(Point &lhs, const Pixel rhs) noexcept
        { lhs.x /= rhs; lhs.y /= rhs; return lhs; }


    /** @brief Point and Size binary operators */
    [[nodiscard]] constexpr Point operator+(const Point &lhs, const Size &rhs) noexcept
        { return Point(lhs.x + rhs.width, lhs.y + rhs.height); }
    [[nodiscard]] constexpr Point operator-(const Point &lhs, const Size &rhs) noexcept
        { return Point(lhs.x - rhs.width, lhs.y - rhs.height); }
    [[nodiscard]] constexpr Point operator*(const Point &lhs, const Size &rhs) noexcept
        { return Point(lhs.x * rhs.width, lhs.y * rhs.height); }
    [[nodiscard]] constexpr Point operator/(const Point &lhs, const Size &rhs) noexcept
        { return Point(lhs.x / rhs.width, lhs.y / rhs.height); }

    /** @brief Point and Size binary assignments */
    constexpr Point &operator+=(Point &lhs, const Size &rhs) noexcept
        { lhs.x += rhs.width; lhs.y += rhs.height; return lhs; }
    constexpr Point &operator-=(Point &lhs, const Size &rhs) noexcept
        { lhs.x -= rhs.width; lhs.y -= rhs.height; return lhs; }
    constexpr Point &operator*=(Point &lhs, const Size &rhs) noexcept
        { lhs.x *= rhs.width; lhs.y *= rhs.height; return lhs; }
    constexpr Point &operator/=(Point &lhs, const Size &rhs) noexcept
        { lhs.x /= rhs.width; lhs.y /= rhs.height; return lhs; }


    /** @brief Size binary operators */
    [[nodiscard]] constexpr Size operator+(const Size &lhs, const Size &rhs) noexcept
        { return Size(lhs.width + rhs.width, lhs.height + rhs.height); }
    [[nodiscard]] constexpr Size operator-(const Size &lhs, const Size &rhs) noexcept
        { return Size(lhs.width - rhs.width, lhs.height - rhs.height); }
    [[nodiscard]] constexpr Size operator*(const Size &lhs, const Size &rhs) noexcept
        { return Size(lhs.width * rhs.width, lhs.height * rhs.height); }
    [[nodiscard]] constexpr Size operator/(const Size &lhs, const Size &rhs) noexcept
        { return Size(lhs.width / rhs.width, lhs.height / rhs.height); }

    /** @brief Size binary assignments */
    constexpr Size &operator+=(Size &lhs, const Size &rhs) noexcept
        { lhs.width += rhs.width; lhs.height += rhs.height; return lhs; }
    constexpr Size &operator-=(Size &lhs, const Size &rhs) noexcept
        { lhs.width -= rhs.width; lhs.height -= rhs.height; return lhs; }
    constexpr Size &operator*=(Size &lhs, const Size &rhs) noexcept
        { lhs.width *= rhs.width; lhs.height *= rhs.height; return lhs; }
    constexpr Size &operator/=(Size &lhs, const Size &rhs) noexcept
        { lhs.width /= rhs.width; lhs.height /= rhs.height; return lhs; }


    /** @brief Size and Pixel binary operators */
    [[nodiscard]] constexpr Size operator+(const Size &lhs, const Pixel rhs) noexcept
        { return Size(lhs.width + rhs, lhs.height + rhs); }
    [[nodiscard]] constexpr Size operator-(const Size &lhs, const Pixel rhs) noexcept
        { return Size(lhs.width - rhs, lhs.height - rhs); }
    [[nodiscard]] constexpr Size operator*(const Size &lhs, const Pixel rhs) noexcept
        { return Size(lhs.width * rhs, lhs.height * rhs); }
    [[nodiscard]] constexpr Size operator/(const Size &lhs, const Pixel rhs) noexcept
        { return Size(lhs.width / rhs, lhs.height / rhs); }

    /** @brief Size and Pixel binary assignments */
    constexpr Size &operator+=(Size &lhs, const Pixel rhs) noexcept
        { lhs.width += rhs; lhs.height += rhs; return lhs; }
    constexpr Size &operator-=(Size &lhs, const Pixel rhs) noexcept
        { lhs.width -= rhs; lhs.height -= rhs; return lhs; }
    constexpr Size &operator*=(Size &lhs, const Pixel rhs) noexcept
        { lhs.width *= rhs; lhs.height *= rhs; return lhs; }
    constexpr Size &operator/=(Size &lhs, const Pixel rhs) noexcept
        { lhs.width /= rhs; lhs.height /= rhs; return lhs; }


    /** @brief Size and Point binary operators */
    [[nodiscard]] constexpr Size operator+(const Size &lhs, const Point &rhs) noexcept
        { return Size(lhs.width + rhs.x, lhs.height + rhs.y); }
    [[nodiscard]] constexpr Size operator-(const Size &lhs, const Point &rhs) noexcept
        { return Size(lhs.width - rhs.x, lhs.height - rhs.y); }
    [[nodiscard]] constexpr Size operator*(const Size &lhs, const Point &rhs) noexcept
        { return Size(lhs.width * rhs.x, lhs.height * rhs.y); }
    [[nodiscard]] constexpr Size operator/(const Size &lhs, const Point &rhs) noexcept
        { return Size(lhs.width / rhs.x, lhs.height / rhs.y); }

    /** @brief Size and Point binary assignments */
    constexpr Size &operator+=(Size &lhs, const Point &rhs) noexcept
        { lhs.width += rhs.x; lhs.height += rhs.y; return lhs; }
    constexpr Size &operator-=(Size &lhs, const Point &rhs) noexcept
        { lhs.width -= rhs.x; lhs.height -= rhs.y; return lhs; }
    constexpr Size &operator*=(Size &lhs, const Point &rhs) noexcept
        { lhs.width *= rhs.x; lhs.height *= rhs.y; return lhs; }
    constexpr Size &operator/=(Size &lhs, const Point &rhs) noexcept
        { lhs.width /= rhs.x; lhs.height /= rhs.y; return lhs; }


    /** @brief Area binary operators */
    [[nodiscard]] constexpr Area operator+(const Area &lhs, const Area &rhs) noexcept
        { return Area(lhs.pos + rhs.pos, lhs.size + rhs.size); }
    [[nodiscard]] constexpr Area operator-(const Area &lhs, const Area &rhs) noexcept
        { return Area(lhs.pos - rhs.pos, lhs.size - rhs.size); }
    [[nodiscard]] constexpr Area operator*(const Area &lhs, const Area &rhs) noexcept
        { return Area(lhs.pos * rhs.pos, lhs.size * rhs.size); }
    [[nodiscard]] constexpr Area operator/(const Area &lhs, const Area &rhs) noexcept
        { return Area(lhs.pos / rhs.pos, lhs.size / rhs.size); }

    /** @brief Area binary assignments */
    constexpr Area &operator+=(Area &lhs, const Area &rhs) noexcept
        { lhs.pos += rhs.pos; lhs.size += rhs.size; return lhs; }
    constexpr Area &operator-=(Area &lhs, const Area &rhs) noexcept
        { lhs.pos -= rhs.pos; lhs.size -= rhs.size; return lhs; }
    constexpr Area &operator*=(Area &lhs, const Area &rhs) noexcept
        { lhs.pos *= rhs.pos; lhs.size *= rhs.size; return lhs; }
    constexpr Area &operator/=(Area &lhs, const Area &rhs) noexcept
        { lhs.pos /= rhs.pos; lhs.size /= rhs.size; return lhs; }


    /** @brief Area and Pixel binary operators */
    [[nodiscard]] constexpr Area operator+(const Area &lhs, const Pixel rhs) noexcept
        { return Area(lhs.pos + rhs, lhs.size + rhs); }
    [[nodiscard]] constexpr Area operator-(const Area &lhs, const Pixel rhs) noexcept
        { return Area(lhs.pos - rhs, lhs.size - rhs); }
    [[nodiscard]] constexpr Area operator*(const Area &lhs, const Pixel rhs) noexcept
        { return Area(lhs.pos * rhs, lhs.size * rhs); }
    [[nodiscard]] constexpr Area operator/(const Area &lhs, const Pixel rhs) noexcept
        { return Area(lhs.pos / rhs, lhs.size / rhs); }

    /** @brief Area and Pixel binary assignments */
    constexpr Area &operator+=(Area &lhs, const Pixel rhs) noexcept
        { lhs.pos += rhs; lhs.size += rhs; return lhs; }
    constexpr Area &operator-=(Area &lhs, const Pixel rhs) noexcept
        { lhs.pos -= rhs; lhs.size -= rhs; return lhs; }
    constexpr Area &operator*=(Area &lhs, const Pixel rhs) noexcept
        { lhs.pos *= rhs; lhs.size *= rhs; return lhs; }
    constexpr Area &operator/=(Area &lhs, const Pixel rhs) noexcept
        { lhs.pos /= rhs; lhs.size /= rhs; return lhs; }


    /** @brief Area and Point binary operators */
    [[nodiscard]] constexpr Area operator+(const Area &lhs, const Point &rhs) noexcept
        { return Area(lhs.pos + rhs, lhs.size); }
    [[nodiscard]] constexpr Area operator-(const Area &lhs, const Point &rhs) noexcept
        { return Area(lhs.pos - rhs, lhs.size); }
    [[nodiscard]] constexpr Area operator*(const Area &lhs, const Point &rhs) noexcept
        { return Area(lhs.pos * rhs, lhs.size); }
    [[nodiscard]] constexpr Area operator/(const Area &lhs, const Point &rhs) noexcept
        { return Area(lhs.pos / rhs, lhs.size); }

    /** @brief Area and Point binary assignments */
    constexpr Area &operator+=(Area &lhs, const Point &rhs) noexcept
        { lhs.pos.x += rhs.x; lhs.pos.y += rhs.y; return lhs; }
    constexpr Area &operator-=(Area &lhs, const Point &rhs) noexcept
        { lhs.pos.x -= rhs.x; lhs.pos.y -= rhs.y; return lhs; }
    constexpr Area &operator*=(Area &lhs, const Point &rhs) noexcept
        { lhs.pos.x *= rhs.x; lhs.pos.y *= rhs.y; return lhs; }
    constexpr Area &operator/=(Area &lhs, const Point &rhs) noexcept
        { lhs.pos.x /= rhs.x; lhs.pos.y /= rhs.y; return lhs; }


    /** @brief Area and Size binary operators */
    [[nodiscard]] constexpr Area operator+(const Area &lhs, const Size &rhs) noexcept
        { return Area(lhs.pos, lhs.size + rhs); }
    [[nodiscard]] constexpr Area operator-(const Area &lhs, const Size &rhs) noexcept
        { return Area(lhs.pos, lhs.size - rhs); }
    [[nodiscard]] constexpr Area operator*(const Area &lhs, const Size &rhs) noexcept
        { return Area(lhs.pos, lhs.size * rhs); }
    [[nodiscard]] constexpr Area operator/(const Area &lhs, const Size &rhs) noexcept
        { return Area(lhs.pos, lhs.size / rhs); }

    /** @brief Area and Size binary assignments */
    constexpr Area &operator+=(Area &lhs, const Size &rhs) noexcept
        { lhs.size.width += rhs.width; lhs.size.height += rhs.height; return lhs; }
    constexpr Area &operator-=(Area &lhs, const Size &rhs) noexcept
        { lhs.size.width -= rhs.width; lhs.size.height -= rhs.height; return lhs; }
    constexpr Area &operator*=(Area &lhs, const Size &rhs) noexcept
        { lhs.size.width *= rhs.width; lhs.size.height *= rhs.height; return lhs; }
    constexpr Area &operator/=(Area &lhs, const Size &rhs) noexcept
        { lhs.size.width /= rhs.width; lhs.size.height /= rhs.height; return lhs; }

    /** @brief Area and Padding binary operators */
    [[nodiscard]] constexpr Area operator+(const Area &lhs, const Padding &rhs) noexcept
        { return Area(lhs.pos - Point(rhs.left, rhs.top), lhs.size + Size(rhs.left + rhs.right, rhs.top + rhs.bottom)); }
    [[nodiscard]] constexpr Area operator-(const Area &lhs, const Padding &rhs) noexcept
        { return Area(lhs.pos + Point(rhs.left, rhs.top), lhs.size - Size(rhs.left + rhs.right, rhs.top + rhs.bottom)); }

    /** @brief Area and Padding binary assignments */
    constexpr Area &operator+=(Area &lhs, const Padding &rhs) noexcept
        { lhs.pos -= Point(rhs.left, rhs.top); lhs.size += Size(rhs.left + rhs.right, rhs.top + rhs.bottom); return lhs; }
    constexpr Area &operator-=(Area &lhs, const Padding &rhs) noexcept
        { lhs.pos += Point(rhs.left, rhs.top); lhs.size -= Size(rhs.left + rhs.right, rhs.top + rhs.bottom); return lhs; }


    /** @brief Padding binary operators */
    [[nodiscard]] constexpr Padding operator+(const Padding &lhs, const Padding &rhs) noexcept
        { return Padding { .left = lhs.left + rhs.left, .right = lhs.right + rhs.right, .top = lhs.top + rhs.top, .bottom = lhs.bottom + rhs.bottom }; }
    [[nodiscard]] constexpr Padding operator-(const Padding &lhs, const Padding &rhs) noexcept
        { return Padding { .left = lhs.left - rhs.left, .right = lhs.right - rhs.right, .top = lhs.top - rhs.top, .bottom = lhs.bottom - rhs.bottom }; }
    [[nodiscard]] constexpr Padding operator*(const Padding &lhs, const Padding &rhs) noexcept
        { return Padding { .left = lhs.left * rhs.left, .right = lhs.right * rhs.right, .top = lhs.top * rhs.top, .bottom = lhs.bottom * rhs.bottom }; }
    [[nodiscard]] constexpr Padding operator/(const Padding &lhs, const Padding &rhs) noexcept
        { return Padding { .left = lhs.left / rhs.left, .right = lhs.right / rhs.right, .top = lhs.top / rhs.top, .bottom = lhs.bottom / rhs.bottom }; }

    /** @brief Padding binary assignments */
    constexpr Padding &operator+=(Padding &lhs, const Padding &rhs) noexcept
        { lhs.left += rhs.left; lhs.right += rhs.right; lhs.top += rhs.top; lhs.bottom += rhs.bottom; return lhs; }
    constexpr Padding &operator-=(Padding &lhs, const Padding &rhs) noexcept
        { lhs.left -= rhs.left; lhs.right -= rhs.right; lhs.top -= rhs.top; lhs.bottom -= rhs.bottom; return lhs; }
    constexpr Padding &operator*=(Padding &lhs, const Padding &rhs) noexcept
        { lhs.left *= rhs.left; lhs.right *= rhs.right; lhs.top *= rhs.top; lhs.bottom *= rhs.bottom; return lhs; }
    constexpr Padding &operator/=(Padding &lhs, const Padding &rhs) noexcept
        { lhs.left /= rhs.left; lhs.right /= rhs.right; lhs.top /= rhs.top; lhs.bottom /= rhs.bottom; return lhs; }


    /** @brief Padding and Pixel binary operators */
    [[nodiscard]] constexpr Padding operator+(const Padding &lhs, const Pixel rhs) noexcept
        { return Padding { .left = lhs.left + rhs, .right = lhs.right + rhs, .top = lhs.top + rhs, .bottom = lhs.bottom + rhs }; }
    [[nodiscard]] constexpr Padding operator-(const Padding &lhs, const Pixel rhs) noexcept
        { return Padding { .left = lhs.left - rhs, .right = lhs.right - rhs, .top = lhs.top - rhs, .bottom = lhs.bottom - rhs }; }
    [[nodiscard]] constexpr Padding operator*(const Padding &lhs, const Pixel rhs) noexcept
        { return Padding { .left = lhs.left * rhs, .right = lhs.right * rhs, .top = lhs.top * rhs, .bottom = lhs.bottom * rhs }; }
    [[nodiscard]] constexpr Padding operator/(const Padding &lhs, const Pixel rhs) noexcept
        { return Padding { .left = lhs.left / rhs, .right = lhs.right / rhs, .top = lhs.top / rhs, .bottom = lhs.bottom / rhs }; }

    /** @brief Padding and Pixel binary assignments */
    constexpr Padding &operator+=(Padding &lhs, const Pixel rhs) noexcept
        { lhs.left += rhs; lhs.right += rhs; lhs.top += rhs; lhs.bottom += rhs; return lhs; }
    constexpr Padding &operator-=(Padding &lhs, const Pixel rhs) noexcept
        { lhs.left -= rhs; lhs.right -= rhs; lhs.top -= rhs; lhs.bottom -= rhs; return lhs; }
    constexpr Padding &operator*=(Padding &lhs, const Pixel rhs) noexcept
        { lhs.left *= rhs; lhs.right *= rhs; lhs.top *= rhs; lhs.bottom *= rhs; return lhs; }
    constexpr Padding &operator/=(Padding &lhs, const Pixel rhs) noexcept
        { lhs.left /= rhs; lhs.right /= rhs; lhs.top /= rhs; lhs.bottom /= rhs; return lhs; }
}

constexpr kF::UI::Area kF::UI::Area::ApplyPadding(const Area &area, const Padding &padding) noexcept
{
    return Area {
        .pos = area.pos + Point {
            padding.left,
            padding.top
        },
        .size = area.size - Size {
            padding.left + padding.right,
            padding.top + padding.bottom
        }
    };
}

template<kF::UI::ConstraintSpecifierRequirements WidthSpecifier, kF::UI::ConstraintSpecifierRequirements HeightSpecifier>
constexpr kF::UI::Constraints kF::UI::Constraints::Make(const WidthSpecifier widthSpecifier, const HeightSpecifier heightSpecifier) noexcept
{
    Constraints constraints;

    if constexpr (std::is_same_v<WidthSpecifier, Fill>) {
        constraints.minSize.width = widthSpecifier.min;
        constraints.maxSize.width = PixelInfinity;
    } else if constexpr (std::is_same_v<WidthSpecifier, Hug>) {
        constraints.minSize.width = widthSpecifier.min;
        constraints.maxSize.width = PixelHug;
    } else if constexpr (std::is_same_v<WidthSpecifier, Fixed>) {
        constraints.minSize.width = widthSpecifier.value;
        constraints.maxSize.width = widthSpecifier.value;
    } else if constexpr (std::is_same_v<WidthSpecifier, Range>) {
        constraints.minSize.width = widthSpecifier.min;
        constraints.maxSize.width = widthSpecifier.max;
    }

    if constexpr (std::is_same_v<HeightSpecifier, Fill>) {
        constraints.minSize.height = heightSpecifier.min;
        constraints.maxSize.height = PixelInfinity;
    } else if constexpr (std::is_same_v<HeightSpecifier, Hug>) {
        constraints.minSize.height = heightSpecifier.min;
        constraints.maxSize.height = PixelHug;
    } else if constexpr (std::is_same_v<HeightSpecifier, Fixed>) {
        constraints.minSize.height = heightSpecifier.value;
        constraints.maxSize.height = heightSpecifier.value;
    } else if constexpr (std::is_same_v<HeightSpecifier, Range>) {
        constraints.minSize.height = heightSpecifier.min;
        constraints.maxSize.height = heightSpecifier.max;
    }

    return constraints;
}
