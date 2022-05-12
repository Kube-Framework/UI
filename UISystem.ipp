/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System
 */

#include "UISystem.hpp"

inline void kF::UI::UISystem::invalidate(void) noexcept
{
    _invalidateFlags = ~static_cast<GPU::FrameIndex>(0);
    _invalidateTree = true;
}

inline void kF::UI::UISystem::validateFrame(const GPU::FrameIndex frame) noexcept
{
    _invalidateFlags &= ~(static_cast<GPU::FrameIndex>(1) << frame);
    _invalidateTree = false;
}

template<kF::UI::Internal::Accumulate AccumulateX, kF::UI::Internal::Accumulate AccumulateY>
inline void kF::UI::UISystem::computeChildrenConstraints(Constraints &constraints) noexcept
{
    const bool hugWidth = constraints.maxSize.width == PixelHug;
    const bool hugHeight = constraints.maxSize.height == PixelHug;

    if (hugWidth || hugHeight) [[unlikely]] {
        for (const auto childEntityIndex : _traverseContext.counter()) {
            const auto &rhs = _traverseContext.constraintsAt(childEntityIndex);
            if (hugWidth)
                ComputeAxisHugConstraint<AccumulateX>(constraints.maxSize.width, rhs.maxSize.width);
            if (hugHeight)
                ComputeAxisHugConstraint<AccumulateY>(constraints.maxSize.height, rhs.maxSize.height);
        }
    }
}

template<kF::UI::Internal::Accumulate AccumulateValue>
inline void kF::UI::UISystem::ComputeAxisHugConstraint(Pixel &lhs, const Pixel rhs) noexcept
{
    using namespace Internal;

    if constexpr (AccumulateValue == Accumulate::Yes) {
        if (lhs != PixelInfinity)
            lhs = rhs == PixelInfinity ? PixelInfinity : lhs + rhs;
    } else {
        if (lhs == PixelInfinity | rhs == PixelInfinity) [[likely]]
            lhs = std::min(lhs, rhs);
        else
            lhs = std::max(lhs, rhs);
    }
}

template<kF::UI::Internal::Axis DistributionAxis>
inline void kF::UI::UISystem::computeDistributedChildrenArea(const Area &contextArea, const Pixel spacing, const Anchor anchor) noexcept
{
    using namespace Internal;

    auto &counter = _traverseContext.counter();
    const auto totalSpacing = spacing * (static_cast<Pixel>(counter.size()) - 1);
    Pixel flexCount = 0;
    Pixel freeSpace;

    if constexpr (DistributionAxis == Axis::Horizontal)
        freeSpace = contextArea.size.width - totalSpacing;
    else
        freeSpace = contextArea.size.height - totalSpacing;

    for (const auto childEntityIndex : counter) {
        const auto &constraints = _traverseContext.constraintsAt(childEntityIndex);
        auto &area = _traverseContext.areaAt(childEntityIndex);

        // Compute width
        area.size.width = ComputeDistributedSize<DistributionAxis == Axis::Horizontal>(
            flexCount,
            freeSpace,
            contextArea.size.width,
            constraints.minSize.width,
            constraints.maxSize.width
        );

        // Compute height
        area.size.height = ComputeDistributedSize<DistributionAxis == Axis::Vertical>(
            flexCount,
            freeSpace,
            contextArea.size.height,
            constraints.minSize.height,
            constraints.maxSize.height
        );
    }

    const auto flexSpace = flexCount ? freeSpace / flexCount : 0;
    Point offset = contextArea.pos;

    for (const auto childEntityIndex : counter) {
        const auto &constraints = _traverseContext.constraintsAt(childEntityIndex);
        auto &area = _traverseContext.areaAt(childEntityIndex);

        // Compute size of flex items
        if constexpr (DistributionAxis == Axis::Horizontal) {
            if (constraints.maxSize.width == PixelInfinity) [[likely]]
                area.size.width = flexSpace;
        } else {
            if (constraints.maxSize.height == PixelInfinity) [[likely]]
                area.size.height = flexSpace;
        }

        // Compute position
        ComputeDistributedPosition<DistributionAxis>(offset, area, contextArea, spacing, anchor);
    }
}

template<kF::UI::Internal::BoundType Bound>
inline kF::UI::Pixel kF::UI::UISystem::ComputeSize(const Pixel parent, const Pixel min, const Pixel max) noexcept
{
    using namespace Internal;

    Pixel out;

    if constexpr (Bound == BoundType::Unknown) {
        if (max == PixelInfinity) [[likely]]
            out = std::min(std::max(parent, min), max);
        else [[unlikely]]
            out = std::min(std::max(max, min), parent);
    } else {
        if constexpr (Bound == BoundType::Infinite)
            out = std::min(std::max(parent, min), max);
        else
            out = std::min(std::max(max, min), parent);
    }
    return out;
}

template<bool Distribute>
inline kF::UI::Pixel kF::UI::UISystem::ComputeDistributedSize(Pixel &flexCount, Pixel &freeSpace,
        const Pixel parent, const Pixel min, const Pixel max) noexcept
{
    using namespace Internal;

    Pixel out {};

    if constexpr (Distribute) {
        if (max == PixelInfinity) [[likely]] {
            ++flexCount;
        } else [[unlikely]] {
            out = ComputeSize<BoundType::Fixed>(parent, min, max);
            freeSpace -= out;
        }
    } else {
        out = ComputeSize<BoundType::Unknown>(parent, min, max);
    }
    return out;
}

template<kF::UI::Internal::Axis DistributionAxis>
inline void kF::UI::UISystem::ComputeDistributedPosition(Point &offset, Area &area, const Area &contextArea, const Pixel spacing, const Anchor anchor) noexcept
{
    using namespace Internal;

    Area transformedArea { contextArea };

    if constexpr (DistributionAxis == Axis::Horizontal) {
        transformedArea.size.width = area.size.width;
        transformedArea.pos.x = offset.x;
        transformedArea.pos.y = offset.y;
        offset.x += area.size.width + spacing;
    } else {
        transformedArea.size.height = area.size.height;
        transformedArea.pos.x = offset.x;
        transformedArea.pos.y = offset.y;
        offset.y += area.size.height + spacing;
    }

    ComputePosition(area, transformedArea, anchor);
}