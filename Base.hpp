/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Base
 */

#pragma once

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
        Crop = 0,
        Fit,
        Stretch
    };

    /** @brief Layout anchor */
    enum class Anchor : std::uint32_t
    {
        Center = 0,
        Left,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };

    /** @brief Layout Flow Type */
    enum class FlowType : std::uint32_t
    {
        Stack = 0x0,
        Column,
        Row
    };

    /** @brief Layout Spacing Type */
    enum class SpacingType : std::uint32_t
    {
        Packed = 0x0,
        SpaceBetween
    };

    /** @brief Text alignment */
    enum class TextAlignment : std::uint32_t
    {
        Left,
        Center,
        Right
    };


    /** @brief Integral pixel type */
    using Pixel = float;

    /** @brief Pixel infinity */
    constexpr Pixel PixelInfinity = std::numeric_limits<Pixel>::infinity();

    /** @brief Pixel hug */
    constexpr Pixel PixelHug = 0.0f;


    /** @brief 32bit RGBA color structure */
    struct alignas(std::uint32_t) Color
    {
        std::uint8_t r;
        std::uint8_t g;
        std::uint8_t b;
        std::uint8_t a;


        /** @brief Comparison operators */
        [[nodiscard]] constexpr bool operator==(const Color &other) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const Color &other) const noexcept = default;
    };


    struct Size;

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

        /** @brief Get min x and y from two points */
        [[nodiscard]] static inline Point Min(const Point lhs, const Point rhs) noexcept
            { return Point { std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y) }; }

        /** @brief Get max x and y from two points */
        [[nodiscard]] static inline Point Max(const Point lhs, const Point rhs) noexcept
            { return Point { std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y) }; }
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

        /** @brief Get min width and height from two sizes */
        [[nodiscard]] static inline Size Min(const Size lhs, const Size rhs) noexcept
            { return Size { std::min(lhs.width, rhs.width), std::min(lhs.height, rhs.height) }; }

        /** @brief Get max width and height from two sizes */
        [[nodiscard]] static inline Size Max(const Size lhs, const Size rhs) noexcept
            { return Size { std::max(lhs.width, rhs.width), std::max(lhs.height, rhs.height) }; }
    };


    /** @brief Area */
    struct alignas_quarter_cacheline Area
    {
        Point pos {};
        Size size {};


        /** @brief Comparison operators */
        [[nodiscard]] constexpr bool operator==(const Area &other) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const Area &other) const noexcept = default;


        /** @brief Check if a point intersect with area */
        [[nodiscard]] inline bool contains(const Point &point) const noexcept
            { return pos.x <= point.x && pos.y <= point.y && pos.x + size.width >= point.x && pos.y + size.height >= point.y; }
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

    /** @brief Constraints range specifier */
    struct Range
    {
        Pixel min {};
        Pixel max {};
    };

    /** @brief Requirements of a constraint specifier */
    template<typename Type>
    concept ConstraintSpecifierRequirements = std::same_as<Type, Fill> || std::same_as<Type, Hug> || std::same_as<Type, Fixed> || std::same_as<Type, Range>;

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
    };
}

/** @brief Stream utilities */
std::ostream &operator<<(std::ostream &lhs, const kF::UI::Color &rhs) noexcept;
std::ostream &operator<<(std::ostream &lhs, const kF::UI::Point &rhs) noexcept;
std::ostream &operator<<(std::ostream &lhs, const kF::UI::Size &rhs) noexcept;
std::ostream &operator<<(std::ostream &lhs, const kF::UI::Area &rhs) noexcept;
std::ostream &operator<<(std::ostream &lhs, const kF::UI::Constraints &rhs) noexcept;
std::ostream &operator<<(std::ostream &lhs, const kF::UI::Padding &rhs) noexcept;
std::ostream &operator<<(std::ostream &lhs, const kF::UI::Radius &rhs) noexcept;

#include "Base.ipp"