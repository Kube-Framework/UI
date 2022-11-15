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

    /** @brief Point unary operators */
    [[nodiscard]] constexpr Point operator-(const Point &value) noexcept
        { return Point(-value.x, -value.y); }

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


    /** @brief Size unary operators */
    [[nodiscard]] constexpr Size operator-(const Size &value) noexcept
        { return Size(-value.width, -value.height); }

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


    /** @brief Area unary operators */
    [[nodiscard]] constexpr Area operator-(const Area &value) noexcept
        { return Area(-value.pos, -value.size); }

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


    /** @brief Padding unary operators */
    [[nodiscard]] constexpr Padding operator-(const Padding &value) noexcept
        { return Padding { .left = -value.left, .right = -value.right, .top = -value.top, .bottom = -value.bottom }; }

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

constexpr bool kF::UI::Area::contains(const Point point) const noexcept
{
    return (left() <= point.x) & (right() >= point.x) & (top() <= point.y) & (bottom() >= point.y);
}

constexpr bool kF::UI::Area::contains(const Area &area) const noexcept
{
    return (left() < area.right()) & (right() > area.left()) & (bottom() > area.top()) & (top() < area.bottom());
}

constexpr bool kF::UI::Area::contains(const Point a, const Point b) const noexcept
{
    constexpr auto GetX = [](const auto y, const auto slope, const auto dt) {
        return slope != 0.0f ? (y - dt) / slope : 0.0f;
    };
    constexpr auto GetY = [](const auto x, const auto slope, const auto dt) {
        return x * slope + dt;
    };

    // Compute Y slope & dt
    const auto slope = b.x != a.x ? (b.y - a.y) / (b.x - a.x) : 0.0f;
    const auto dt = a.y - slope * a.x;

    // Cache area positions
    const UI::Pixel top_ = top();
    const UI::Pixel bottom_ = bottom();
    const UI::Pixel left_ = left();
    const UI::Pixel right_ = right();

    // Out of bounds check
    if (((a.y < top_) & (b.y < top_))
        | ((a.y > bottom_) & (b.y > bottom_))
        | ((a.x < left_) & b.x < left_)
        | ((a.x > right_) & (b.x > right_)))
        return false;

    // Compute line intersections
    const auto topX = GetX(top_, slope, dt);
    const auto bottomX = GetX(bottom_, slope, dt);
    const auto leftY = GetY(left_, slope, dt);
    const auto rightY = GetY(right_, slope, dt);

    return ((topX >= left_) & (topX <= right_))
        | ((bottomX >= left_) & (bottomX <= right_))
        | ((leftY >= top_) & (leftY <= bottom_))
        | ((rightY >= top_) & (rightY <= bottom_));
}

constexpr kF::UI::Area kF::UI::Area::MakeCenter(const Point center, const Size size) noexcept
{
    return Area { center - size / 2.0f, size };
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

constexpr kF::UI::Area kF::UI::Area::ApplyClip(const Area &area, const Area &clipArea) noexcept
{
    Area clipped;

    clipped.pos.x = std::max(area.pos.x, clipArea.pos.x);
    clipped.pos.y = std::max(area.pos.y, clipArea.pos.y);

    const auto removedWidth = clipped.pos.x - area.pos.x;
    clipped.size.width = std::min(area.size.width - removedWidth, clipArea.right() - clipped.pos.x);
    const auto removedHeight = clipped.pos.y - area.pos.y;
    clipped.size.height = std::min(area.size.height - removedHeight, clipArea.bottom() - clipped.pos.y);
    return clipped;
}

constexpr kF::UI::Area kF::UI::Area::ApplyAnchor(const Area &area, const Size childSize, const Anchor anchor) noexcept
{
    Area child {
        area.pos,
        childSize
    };
    switch (anchor) {
    case Anchor::TopLeft:
        break;
    case Anchor::Top:
        child.pos += Size(area.size.width / 2 - child.size.width / 2, 0);
        break;
    case Anchor::TopRight:
        child.pos += Size(area.size.width - child.size.width, 0);
        break;
    case Anchor::Left:
        child.pos += Size(0, area.size.height / 2 - child.size.height / 2);
        break;
    case Anchor::Center:
        child.pos += (area.size - child.size) / 2;
        break;
    case Anchor::Right:
        child.pos += Size(area.size.width - child.size.width, area.size.height / 2 - child.size.height / 2);
        break;
    case Anchor::BottomLeft:
        child.pos += Size(0, area.size.height - child.size.height);
        break;
    case Anchor::Bottom:
        child.pos += Size(area.size.width / 2 - child.size.width / 2, area.size.height - child.size.height);
        break;
    case Anchor::BottomRight:
        child.pos += Size(area.size.width - child.size.width, area.size.height - child.size.height);
        break;
    }
    return child;
}

template<kF::UI::ConstraintSpecifierRequirements WidthSpecifier, kF::UI::ConstraintSpecifierRequirements HeightSpecifier>
constexpr kF::UI::Constraints kF::UI::Constraints::Make(const WidthSpecifier widthSpecifier, const HeightSpecifier heightSpecifier) noexcept
{
    constexpr auto ApplySpecifier = []<typename Specifier>(const Specifier specifier, Pixel &min, Pixel &max) {
        if constexpr (std::is_same_v<Specifier, Fill>) {
            min = specifier.min;
            max = PixelInfinity;
        } else if constexpr (std::is_same_v<Specifier, Hug>) {
            min = specifier.min;
            max = PixelHug;
        } else if constexpr (std::is_same_v<Specifier, Fixed>) {
            max = specifier.value;
        } else if constexpr (std::is_same_v<Specifier, Strict>) {
            min = specifier.value;
            max = specifier.value;
        } else if constexpr (std::is_same_v<Specifier, Range>) {
            min = specifier.min;
            max = specifier.max;
        }
    };

    Constraints constraints;

    ApplySpecifier(widthSpecifier, constraints.minSize.width, constraints.maxSize.width);
    ApplySpecifier(heightSpecifier, constraints.minSize.height, constraints.maxSize.height);

    return constraints;
}