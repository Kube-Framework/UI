/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Base
 */

#pragma once

#include <limits>

#include <Kube/Core/StaticSafeAllocator.hpp>

namespace kF::UI
{
    /** @brief UI RenderPass Index */
    constexpr std::uint32_t RenderPassIndex = 0u;

    /** @brief UI Primitive Subpass Index */
    constexpr std::uint32_t GraphicSubpassIndex = 0u;


    /** @brief Allocator of the UI library */
    using UIAllocator = Core::StaticSafeAllocator<"UIAllocator">;

    /** @brief Event Allocator of the UI library */
    using EventAllocator = Core::StaticSafeAllocator<"EventAllocator">;

    /** @brief Allocator Resource of the UI library */
    using ResourceAllocator = Core::StaticSafeAllocator<"ResourceAllocator">;


    /** @brief Image fill mode */
    enum class FillMode : std::uint32_t
    {
        Crop,
        Fit,
        Stretch
    };

    /** @brief Layout anchor */
    enum class Anchor : std::uint8_t
    {
        TopLeft,
        Top,
        TopRight,
        Left,
        Center,
        Right,
        BottomLeft,
        Bottom,
        BottomRight
    };

    /** @brief Layout Spacing Type */
    enum class SpacingType : std::uint8_t
    {
        Packed,
        SpaceBetween
    };

    /** @brief Layout Flow Type */
    enum class FlowType : std::uint32_t
    {
        Stack,
        Column,
        Row,
        FlexColumn,
        FlexRow
    };

    /** @brief Text alignment */
    enum class TextAlignment : std::uint32_t
    {
        Left,
        Center,
        Right,
        Justify
    };


    /** @brief Integral pixel type */
    using Pixel = float;

    /** @brief Pixel infinity */
    constexpr Pixel PixelInfinity = std::numeric_limits<Pixel>::infinity();

    /** @brief Pixel fill */
    constexpr Pixel PixelFill = std::numeric_limits<Pixel>::lowest();

    /** @brief Pixel hug */
    constexpr Pixel PixelHug = std::numeric_limits<Pixel>::lowest() / 10.0f;

    /** @brief Pixel mirror */
    constexpr Pixel PixelMirror = std::numeric_limits<Pixel>::lowest() / 100.0f;

    /** @brief Check if a pixel constraint is fixed, which mean it isn't Fill, Hug nor Mirror */
    [[nodiscard]] constexpr bool IsFixedConstraint(const Pixel pixel) noexcept { return pixel > PixelMirror; }

    static_assert(
        !IsFixedConstraint(PixelFill)
        && !IsFixedConstraint(PixelHug)
        && !IsFixedConstraint(PixelMirror)
        && IsFixedConstraint(0.0f)
        && IsFixedConstraint(PixelInfinity),
        "kF::UI:Base: Implementation error"
    );


    /** @brief Dot per inches */
    struct DPI
    {
        Pixel diagonal {};
        Pixel horizontal {};
        Pixel vertical {};
    };

    /** @brief Scale DIP (Device Independent Pixels) to pixel unit considering display DPI (Dots Per Inch) */
    [[nodiscard]] static inline Pixel ScalePixel(const Pixel dip, const Pixel displayDPI) noexcept
        { return dip / (displayDPI / 96.0f); }


    /** @brief 32bit RGBA color structure */
    struct alignas(std::uint32_t) Color
    {
        using Unit = std::uint8_t;

        Unit r;
        Unit g;
        Unit b;
        Unit a;


        /** @brief Comparison operators */
        [[nodiscard]] constexpr bool operator==(const Color &other) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const Color &other) const noexcept = default;


        /** @brief Apply alpha to a color (override color's alpha) */
        [[nodiscard]] static constexpr Color ApplyAlpha(const Color color, const std::uint8_t alpha) noexcept
            { return Color { color.r, color.g, color.b, alpha }; }


        /** @brief Stream overload insert operator */
        friend std::ostream &operator<<(std::ostream &lhs, const Color &rhs) noexcept;
    };


    struct Size;
    struct Padding;

    /** @brief Point */
    struct alignas_eighth_cacheline Point
    {
        Pixel x {};
        Pixel y {};


        /** @brief Comparison operators */
        [[nodiscard]] constexpr bool operator==(const Point &other) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const Point &other) const noexcept = default;


        /** @brief Convert point to size */
        [[nodiscard]] constexpr Size toSize(void) const noexcept;


        /** @brief Get absolute point */
        [[nodiscard]] static inline Point Abs(const Point value) noexcept
            { return Point { std::abs(value.x), std::abs(value.y) }; }

        /** @brief Get min x and y from two points */
        [[nodiscard]] static inline Point Min(const Point lhs, const Point rhs) noexcept
            { return Point { std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y) }; }

        /** @brief Get max x and y from two points */
        [[nodiscard]] static inline Point Max(const Point lhs, const Point rhs) noexcept
            { return Point { std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y) }; }


        /** @brief Stream overload insert operator */
        friend std::ostream &operator<<(std::ostream &lhs, const Point &rhs) noexcept;
    };


    /** @brief Size */
    struct alignas_eighth_cacheline Size
    {
        Pixel width {};
        Pixel height {};


        /** @brief Comparison operators */
        [[nodiscard]] constexpr bool operator==(const Size &other) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const Size &other) const noexcept = default;


        /** @brief Convert size to point */
        [[nodiscard]] constexpr Point toPoint(void) const noexcept;


        /** @brief Get absolute size */
        [[nodiscard]] static inline Size Abs(const Size value) noexcept
            { return Size { std::abs(value.width), std::abs(value.height) }; }

        /** @brief Get min width and height from two sizes */
        [[nodiscard]] static inline Size Min(const Size lhs, const Size rhs) noexcept
            { return Size { std::min(lhs.width, rhs.width), std::min(lhs.height, rhs.height) }; }

        /** @brief Get max width and height from two sizes */
        [[nodiscard]] static inline Size Max(const Size lhs, const Size rhs) noexcept
            { return Size { std::max(lhs.width, rhs.width), std::max(lhs.height, rhs.height) }; }


        /** @brief Stream overload insert operator */
        friend std::ostream &operator<<(std::ostream &lhs, const Size &rhs) noexcept;
    };


    /** @brief Area */
    struct alignas_quarter_cacheline Area
    {
        Point pos {};
        Size size {};


        /** @brief Comparison operators */
        [[nodiscard]] constexpr bool operator==(const Area &other) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const Area &other) const noexcept = default;


        /** @brief Get left coordinate */
        [[nodiscard]] constexpr Pixel left(void) const noexcept
            { return pos.x; }

        /** @brief Get right coordinate */
        [[nodiscard]] constexpr Pixel right(void) const noexcept
            { return pos.x + size.width; }

        /** @brief Get top coordinate */
        [[nodiscard]] constexpr Pixel top(void) const noexcept
            { return pos.y; }

        /** @brief Get bottom coordinate */
        [[nodiscard]] constexpr Pixel bottom(void) const noexcept
            { return pos.y + size.height; }


        /** @brief Get top left coordinate */
        [[nodiscard]] constexpr Point topLeft(void) const noexcept
            { return pos; }

        /** @brief Get top right coordinate */
        [[nodiscard]] constexpr Point topRight(void) const noexcept
            { return Point(pos.x + size.width, pos.y); }

        /** @brief Get bottom left coordinate */
        [[nodiscard]] constexpr Point bottomLeft(void) const noexcept
            { return Point(pos.x, pos.y + size.height); }

        /** @brief Get bottom right coordinate */
        [[nodiscard]] constexpr Point bottomRight(void) const noexcept
            { return Point(pos.x + size.width, pos.y + size.height); }


        /** @brief Get center X coordinate */
        [[nodiscard]] constexpr Pixel centerX(void) const noexcept
            { return pos.x + size.width / 2.0f; }

        /** @brief Get center Y coordinate */
        [[nodiscard]] constexpr Pixel centerY(void) const noexcept
            { return pos.y + size.height / 2.0f; }

        /** @brief Get center position */
        [[nodiscard]] constexpr Point center(void) const noexcept
            { return Point(centerX(), centerY()); }


        /** @brief Check if a point overlap with area */
        [[nodiscard]] constexpr bool contains(const Point point) const noexcept;

        /** @brief Check if an area overlap with area */
        [[nodiscard]] constexpr bool contains(const Area &area) const noexcept;

        /** @brief Check if an area overlap with a segment */
        [[nodiscard]] constexpr bool contains(const Point a, const Point b) const noexcept;


        /** @brief Create an Area of given 'size' centered to a given 'center' point */
        [[nodiscard]] static constexpr Area MakeCenter(const Point center, const Size size) noexcept;


        /** @brief Apply padding to an area */
        [[nodiscard]] static constexpr Area ApplyPadding(const Area &area, const Padding &padding) noexcept;

        /** @brief Apply clip to an area */
        [[nodiscard]] static constexpr Area ApplyClip(const Area &area, const Area &clipArea) noexcept;

        /** @brief Apply anchor to a position a parent's child area from its size */
        [[nodiscard]] static constexpr Area ApplyAnchor(const Area &area, const Size childSize, const Anchor anchor) noexcept;


        /** @brief Stream overload insert operator */
        friend std::ostream &operator<<(std::ostream &lhs, const Area &rhs) noexcept;
    };


    /** @brief Constraints fill specifier */
    struct Fill
    {
        Pixel min {};
    };

    /** @brief Constraints hug content specifier */
    struct Hug
    {
        Pixel min {};
    };

    /** @brief Constraints fixed specifier */
    struct Fixed
    {
        Pixel value {};
    };

    /** @brief Constraints strict specifier */
    struct Strict
    {
        Pixel value {};
    };

    /** @brief Constraints range specifier */
    struct Range
    {
        Pixel min {};
        Pixel max {};
    };

    /** @brief Constraints mirror specifier (copies opposite axis) */
    struct Mirror
    {
        Pixel min {};
    };

    /** @brief Requirements of a constraint specifier */
    template<typename Type>
    concept ConstraintSpecifierRequirements = std::same_as<Type, Fill>
            || std::same_as<Type, Hug>
            || std::same_as<Type, Fixed>
            || std::same_as<Type, Strict>
            || std::same_as<Type, Range>
            || std::same_as<Type, Mirror>;

    /** @brief Constraints */
    struct alignas_quarter_cacheline Constraints
    {
        Size minSize {};
        Size maxSize {};


        /** @brief Comparison operators */
        [[nodiscard]] constexpr bool operator==(const Constraints &other) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const Constraints &other) const noexcept = default;


        /** @brief Create max width / height constraints */
        template<kF::UI::ConstraintSpecifierRequirements WidthSpecifier, kF::UI::ConstraintSpecifierRequirements HeightSpecifier>
        [[nodiscard]] static constexpr Constraints Make(const WidthSpecifier widthSpecifier, const HeightSpecifier heightSpecifier) noexcept;

        /** @brief Create single size constraints */
        template<kF::UI::ConstraintSpecifierRequirements SizeSpecifier>
        [[nodiscard]] static constexpr Constraints Make(const SizeSpecifier sizeSpecifier) noexcept { return Make(sizeSpecifier, sizeSpecifier); }


        /** @brief Stream overload insert operator */
        friend std::ostream &operator<<(std::ostream &lhs, const Constraints &rhs) noexcept;
    };


    /** @brief Padding */
    struct alignas_quarter_cacheline Padding
    {
        Pixel left {};
        Pixel right {};
        Pixel top {};
        Pixel bottom {};


        /** @brief Comparison operators */
        [[nodiscard]] constexpr bool operator==(const Padding &other) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const Padding &other) const noexcept = default;


        /** @brief Fill a padding structure with a single value for all pads */
        [[nodiscard]] static constexpr Padding MakeCenter(const Pixel value) noexcept { return Padding(value, value, value, value); }

        /** @brief Fill a padding structure with a single value for horizontal pads */
        [[nodiscard]] static constexpr Padding MakeHorizontal(const Pixel value) noexcept { return Padding(value, value, 0, 0); }

        /** @brief Fill a padding structure with a single value for vertical pads */
        [[nodiscard]] static constexpr Padding MakeVertical(const Pixel value) noexcept { return Padding(0, 0, value, value); }


        /** @brief Stream overload insert operator */
        friend std::ostream &operator<<(std::ostream &lhs, const Padding &rhs) noexcept;
    };


    /** @brief Radius */
    struct alignas_quarter_cacheline Radius
    {
        Pixel topLeft {};
        Pixel topRight {};
        Pixel bottomLeft {};
        Pixel bottomRight {};


        /** @brief Comparison operators */
        [[nodiscard]] constexpr bool operator==(const Radius &other) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const Radius &other) const noexcept = default;


        /** @brief Fill a radius structure with a single value */
        [[nodiscard]] static constexpr Radius MakeFill(const Pixel value) noexcept { return Radius(value, value, value, value); }

        /** @brief Fill top radius with a single value */
        [[nodiscard]] static constexpr Radius MakeTop(const Pixel value) noexcept { return Radius(value, value, 0.0f, 0.0f); }

        /** @brief Fill bottom radius with a single value */
        [[nodiscard]] static constexpr Radius MakeBottom(const Pixel value) noexcept { return Radius(0.0f, 0.0f, value, value); }

        /** @brief Fill left radius with a single value */
        [[nodiscard]] static constexpr Radius MakeLeft(const Pixel value) noexcept { return Radius(value, 0.0f, value, 0.0f); }

        /** @brief Fill right radius with a single value */
        [[nodiscard]] static constexpr Radius MakeRight(const Pixel value) noexcept { return Radius(0.0f, value, 0.0f, value); }


        /** @brief Stream overload insert operator */
        friend std::ostream &operator<<(std::ostream &lhs, const Radius &rhs) noexcept;
    };


    /** @brief Default clip area is infinity (means window size) */
    static constexpr Area DefaultClip { Point {}, Size { PixelInfinity, PixelInfinity }};


    /** @brief Store a type hash code for comparison purposes */
    struct alignas_eighth_cacheline TypeHash
    {
        /** @brief Get an opaque type handle from a templated type */
        template<typename Type>
        [[nodiscard]] static constexpr TypeHash Get(void) noexcept
            { return TypeHash { typeid(std::remove_cvref_t<Type>).hash_code() }; }

        std::size_t hash {};

        /** @brief Comparison operators */
        [[nodiscard]] bool operator==(const TypeHash &other) const noexcept = default;
        [[nodiscard]] bool operator!=(const TypeHash &other) const noexcept = default;
    };
    static_assert_fit_eighth_cacheline(TypeHash);


    /** @brief Index of a font */
    struct FontIndex
    {
        /** @brief Type of index */
        using IndexType = std::uint32_t;

        IndexType value {};

        /** @brief Implicit conversion to value */
        [[nodiscard]] inline operator IndexType(void) const noexcept { return value; }
    };


    /** @brief Index of a sprite */
    struct SpriteIndex
    {
        /** @brief Type of index */
        using IndexType = std::uint32_t;

        IndexType value {};

        /** @brief Implicit conversion to value */
        [[nodiscard]] inline operator IndexType(void) const noexcept { return value; }
    };


    /** @brief Helper that interacts with a Point or a Size to retreive its X axis component */
    constexpr auto GetXAxis = []<typename Type>(Type &&data) noexcept -> auto &
    {
        if constexpr (std::is_same_v<Point, std::remove_cvref_t<Type>>)
            return data.x;
        else
            return data.width;
    };

    /** @brief Helper that interacts with a Point or a Size to retreive its Y axis component */
    constexpr auto GetYAxis = []<typename Type>(Type &&data) noexcept -> auto &
    {
        if constexpr (std::is_same_v<Point, std::remove_cvref_t<Type>>)
            return data.y;
        else
            return data.height;
    };


    namespace Internal
    {
        /** @brief Forward an argument either by forwarding or by invoking a functor */
        template<typename Arg>
        [[nodiscard]] constexpr decltype(auto) ForwardArg(Arg &&arg) noexcept
        {
            if constexpr (std::is_invocable_v<Arg>)
                return arg();
            else
                return std::forward<Arg>(arg);
        };
    }
}

#include "Base.ipp"
