/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Layout processor
 */

#include <Kube/Core/Assert.hpp>

#include "LayoutBuilder.hpp"
#include "UISystem.hpp"

using namespace kF;

namespace kF::UI::Internal
{
    /** @brief Compute axis constraint (rhs) to another (lhs) */
    template<Accumulate AccumulateValue>
    void ComputeAxisHugConstraint(Pixel &lhs, const Pixel rhs) noexcept;

    /** @brief Compute distributed child size inside parent space according to min/max range and spacing */
    template<bool Distribute>
    [[nodiscard]] Pixel ComputeLayoutChildSize([[maybe_unused]] Pixel &flexCount, [[maybe_unused]] Pixel &freeSpace,
            const Pixel parent, const Pixel min, const Pixel max) noexcept;

    /** @brief Compute child size inside parent space according to min/max range */
    template<BoundType Bound>
    [[nodiscard]] Pixel ComputeSize([[maybe_unused]] const Pixel parent, [[maybe_unused]] const Pixel min, const Pixel max) noexcept;

    /** @brief Compute position of a distributed item using its context area and an anchor */
    template<Axis DistributionAxis>
    void ComputeLayoutChildPosition(Point &offset, Area &area, const Area &contextArea, const Pixel spacing, const Anchor anchor) noexcept;
}

UI::DepthUnit UI::Internal::LayoutBuilder::build(void) noexcept
{
    // Prepare context caches
    auto &nodeTable = _uiSystem->getTable<TreeNode>();
    _traverseContext->setupContext(
        nodeTable.count(),
        nodeTable.begin(),
        _uiSystem->getTable<Area>().begin(),
        _uiSystem->getTable<Depth>().begin()
    );

    // Traverse from each childrenless items to root
    for (const auto &node : nodeTable) {
        // If the node does not have children, we go upward from here
        if (node.children.empty()) [[unlikely]] {
            { // Setup the entity for traversal
                const auto entityIndex = _traverseContext->entityIndexOf(node);
                _traverseContext->setupEntity(
                    nodeTable.entities().at(entityIndex),
                    entityIndex
                );
            }
            // Traverse from bottom to top
            traverseConstraints();
        }
    }

    { // Setup root entity from top to bottom traversal
        const auto rootEntity = Item::GetEntity(_uiSystem->root());
        const auto rootEntityIndex = _traverseContext->entityIndexOf(nodeTable.get(rootEntity));
        _traverseContext->setupEntity(rootEntity, rootEntityIndex);

        // Force root area to be the size of the window
        // @todo Make the root being able to set the window size with regular layouts (ex: hug)
        auto &area = _traverseContext->areaAt(rootEntityIndex);
        area.pos = Point {};
        area.size = _uiSystem->windowSize();

        // Reset depth cache before traversal
        _maxDepth = DepthUnit {};
    }


    // Traverse from top to bottom
    traverseAreas();

    return _maxDepth;
}

void UI::Internal::LayoutBuilder::traverseConstraints(void) noexcept
{
    constexpr auto GetCounterInsertIndex = [](const ECS::Entity entity, const TreeNode &parentNode, const Internal::TraverseContext::Counter &counter) -> std::uint32_t {
        if (counter.empty()) [[likely]]
            return 0u;
        return std::min(
            static_cast<std::uint32_t>(std::distance(parentNode.children.begin(), parentNode.children.find(entity))),
            counter.size()
        );
    };

    { // For each traversed node, we build constraints from children to parents
        const auto entity = _traverseContext->entity();
        TreeNode &node = _traverseContext->node();
        Constraints &constraints = _traverseContext->constraints();

        // If the node has explicit constraints use it, else we use default fill constraints
        if (!Core::HasFlags(node.componentFlags, ComponentFlags::Constraints)) [[likely]] {
            constraints = Constraints::Make(Fill(), Fill());
        } else [[unlikely]] {
            constraints = _uiSystem->get<Constraints>(entity);
        }

        // If the node has at least one child, compute its size accordingly to its children
        if (!node.children.empty()) [[likely]] {
            // Update self constraints depending on children constraints
            if (!Core::HasFlags(node.componentFlags, ComponentFlags::Layout)) [[likely]] {
                computeChildrenHugConstraints<Accumulate::No, Accumulate::No>(
                    constraints, Pixel {}, constraints.maxSize.width == PixelHug, constraints.maxSize.height == PixelHug
                );
            } else [[unlikely]] {
                buildLayoutConstraints(constraints);
            }
        }

        // Ensure node has parent, else stop traversal
        if (!node.parent) [[unlikely]]
            return;

        // Setup parent node in traverse context
        const auto entityIndex = _traverseContext->entityIndex();
        const auto &parentNode = _uiSystem->get<TreeNode>(node.parent);
        const auto parentEntityIndex = _traverseContext->entityIndexOf(parentNode);
        _traverseContext->setupEntity(node.parent, parentEntityIndex);

        // Insert entity index into counter list
        auto &counter = _traverseContext->counter();
        const auto insertIndex = GetCounterInsertIndex(entity, parentNode, counter);
        counter.insert(counter.begin() + insertIndex, entityIndex);

        // Check if parent still has unvisited child => stop traversal
        if (parentNode.children.size() != counter.size()) [[likely]]
            return;
    }

    // Process parent constraints
    traverseConstraints();
}

void UI::Internal::LayoutBuilder::buildLayoutConstraints(Constraints &constraints) noexcept
{
    const bool hugWidth = constraints.maxSize.width == PixelHug;
    const bool hugHeight = constraints.maxSize.height == PixelHug;

    if (!hugWidth && !hugHeight) [[likely]]
        return;

    const auto &layout = _uiSystem->get<Layout>(_traverseContext->entity());
    switch (layout.flowType) {
    case FlowType::Stack:
        computeChildrenHugConstraints<Accumulate::No, Accumulate::No>(constraints, layout.spacing, hugWidth, hugHeight);
        break;
    case FlowType::Column:
        computeChildrenHugConstraints<Accumulate::No, Accumulate::Yes>(constraints, layout.spacing, hugWidth, hugHeight);
        break;
    case FlowType::Row:
        computeChildrenHugConstraints<Accumulate::Yes, Accumulate::No>(constraints, layout.spacing, hugWidth, hugHeight);
        break;
    case FlowType::FlexColumn:
        computeFlexChildrenHugConstraints<Axis::Vertical, GetXAxis, GetYAxis>(constraints, layout.spacing, hugWidth, hugHeight);
        break;
    case FlowType::FlexRow:
        computeFlexChildrenHugConstraints<Axis::Horizontal, GetYAxis, GetXAxis>(constraints, layout.spacing, hugWidth, hugHeight);
        break;
    }

    const auto &padding = layout.padding;
    if (hugWidth) {
        constraints.maxSize.width += padding.left + padding.right;
        constraints.minSize.width = std::max(constraints.minSize.width, constraints.maxSize.width);
    }
    if (hugHeight) {
        constraints.maxSize.height += padding.top + padding.bottom;
        constraints.minSize.height = std::max(constraints.minSize.height, constraints.maxSize.height);
    }

    // Ensure we are equal or larger than min constraints
    if (constraints.minSize.width > constraints.maxSize.width) [[unlikely]]
        constraints.maxSize.width = constraints.minSize.width;
    if (constraints.minSize.height > constraints.maxSize.height) [[unlikely]]
        constraints.maxSize.height = constraints.minSize.height;
}

template<kF::UI::Internal::Accumulate AccumulateX, kF::UI::Internal::Accumulate AccumulateY>
void kF::UI::Internal::LayoutBuilder::computeChildrenHugConstraints(Constraints &constraints, [[maybe_unused]] const Pixel spacing,
        const bool hugWidth, const bool hugHeight) noexcept
{
    for (const auto childEntityIndex : _traverseContext->counter()) {
        const auto &rhs = _traverseContext->constraintsAt(childEntityIndex);
        if (hugWidth)
            ComputeAxisHugConstraint<AccumulateX>(constraints.maxSize.width, rhs.maxSize.width);
        if (hugHeight)
            ComputeAxisHugConstraint<AccumulateY>(constraints.maxSize.height, rhs.maxSize.height);
    }

    if constexpr (AccumulateX == Accumulate::Yes)
        constraints.maxSize.width += spacing * static_cast<Pixel>(_traverseContext->counter().size() - 1);
    else if constexpr (AccumulateY == Accumulate::Yes)
        constraints.maxSize.height += spacing * static_cast<Pixel>(_traverseContext->counter().size() - 1);
}

template<kF::UI::Internal::Accumulate AccumulateValue>
void kF::UI::Internal::ComputeAxisHugConstraint(Pixel &lhs, const Pixel rhs) noexcept
{
    if constexpr (AccumulateValue == Accumulate::Yes) {
        if (lhs != PixelInfinity)
            lhs = rhs == PixelInfinity ? PixelInfinity : lhs + rhs;
    } else {
        if ((lhs == PixelInfinity) | (rhs == PixelInfinity)) [[likely]]
            lhs = std::min(lhs, rhs);
        else
            lhs = std::max(lhs, rhs);
    }
}

template<kF::UI::Internal::Axis FlexAxis, auto GetX, auto GetY>
void kF::UI::Internal::LayoutBuilder::computeFlexChildrenHugConstraints(Constraints &constraints, const Pixel spacing,
        const bool hugWidth, const bool hugHeight) noexcept
{
    if constexpr (FlexAxis == Axis::Vertical) {
        kFAssert(!hugWidth,
            "UI::LayoutBuilder::computeFlexChildrenConstraints: FlowType::FlexColumn cannot take Hug as width constraint");
        kFAssert(constraints.maxSize.width != PixelInfinity,
            "UI::LayoutBuilder::computeFlexChildrenConstraints: FlowType::FlexColumn cannot take Fill width constraint when height is Hug constraint");
    } else {
        kFAssert(!hugHeight,
            "UI::LayoutBuilder::computeFlexChildrenConstraints: FlowType::FlexRow cannot take Hug as height constraint");
        kFAssert(constraints.maxSize.height != PixelInfinity,
            "UI::LayoutBuilder::computeFlexChildrenConstraints: FlowType::FlexRow cannot take Fill height constraint when width is Hug constraint");
    }

    const auto lineSize = GetX(constraints.maxSize);
    auto lineRemain = lineSize;

    for (const auto childEntityIndex : _traverseContext->counter()) {
        const auto &rhs = _traverseContext->constraintsAt(childEntityIndex);

        // Compute hug axis
        ComputeAxisHugConstraint<Accumulate::Yes>(GetY(constraints.maxSize), GetY(rhs.maxSize));

        // Compute flex axis
        const auto maxInsertSize = GetX(rhs.maxSize) != PixelInfinity ? GetX(rhs.maxSize) : lineSize;
        auto insertSize = maxInsertSize;
        // Check line edge
        if (maxInsertSize > lineRemain) [[unlikely]] {
            insertSize = std::max(GetX(rhs.minSize), lineRemain);
            // We cannot fit in line
            if (insertSize > lineRemain) {
                // If the line is not empty we go to the next line (else we draw minimum)
                if (lineRemain != lineSize) [[likely]] {
                    // If the max insert size fits lineSize use it else use minimum (we know insertSize is rhs.minSize)
                    if (maxInsertSize <= lineSize)
                        insertSize = maxInsertSize;
                }
            }
        }
        lineRemain -= insertSize + spacing;
        if (lineRemain <= 0.0) [[unlikely]]
            lineRemain = lineSize;
    }
}

void UI::Internal::LayoutBuilder::traverseAreas(void) noexcept
{
    // Set self depth
    _traverseContext->depth().depth = _maxDepth++;

    Area lastClip {};
    bool clip { false };
    const auto &node = _traverseContext->node();
    auto &area = _traverseContext->area();

    // Build position of children using the context node area
    if (!Core::HasFlags(node.componentFlags, ComponentFlags::Layout)) [[likely]]
        computeChildrenArea(area, Anchor::Center);
    else [[unlikely]]
        buildLayoutArea(area);

    if (const auto &nodeCounter = _traverseContext->counter(); !nodeCounter.empty()) [[likely]] {
        { // Process clip
            const auto &clipTable = _uiSystem->getTable<Clip>();
            const auto clipIndex = clipTable.getUnstableIndex(_traverseContext->entity());
            if (clipIndex != ECS::NullEntityIndex) [[unlikely]] {
                clip = true;
                lastClip = _traverseContext->currentClip();
                _traverseContext->setClip(
                    Area::ApplyPadding(area, clipTable.atIndex(clipIndex).padding),
                    _maxDepth
                );
            }
        }

        // Traverse each child
        std::uint32_t childIndex { 0u };
        for (const auto childEntityIndex : nodeCounter) {
            _traverseContext->setupEntity(node.children.at(childIndex++), childEntityIndex);
            traverseAreas();
        }
    }

    // Set max child depth
    _traverseContext->depth().maxChildDepth = _maxDepth - 1;

    // Restore previous clip
    if (clip) [[unlikely]]
        _traverseContext->setClip(lastClip, _maxDepth);
}

void UI::Internal::LayoutBuilder::buildLayoutArea(const Area &contextArea) noexcept
{
    auto &layout = _uiSystem->get<Layout>(_traverseContext->entity());
    const auto transformedArea = Area::ApplyPadding(contextArea, layout.padding);

    switch (layout.flowType) {
    case FlowType::Stack:
        computeChildrenArea(transformedArea, layout.anchor);
        break;
    case FlowType::Column:
        computeLayoutChildrenArea<Axis::Vertical>(transformedArea, layout);
        break;
    case FlowType::Row:
        computeLayoutChildrenArea<Axis::Horizontal>(transformedArea, layout);
        break;
    case FlowType::FlexColumn:
        computeFlexLayoutChildrenArea<Axis::Vertical>(transformedArea, layout);
        break;
    case FlowType::FlexRow:
        computeFlexLayoutChildrenArea<Axis::Horizontal>(transformedArea, layout);
        break;
    }
}

void UI::Internal::LayoutBuilder::computeChildrenArea(const Area &contextArea, const Anchor anchor) noexcept
{
    for (const auto childEntityIndex : _traverseContext->counter()) {
        const auto &constraints = _traverseContext->constraintsAt(childEntityIndex);
        auto &area = _traverseContext->areaAt(childEntityIndex);

        // Compute size
        area.size.width = Internal::ComputeSize<BoundType::Unknown>(
            contextArea.size.width,
            constraints.minSize.width,
            constraints.maxSize.width
        );
        area.size.height = Internal::ComputeSize<BoundType::Unknown>(
            contextArea.size.height,
            constraints.minSize.height,
            constraints.maxSize.height
        );

        // Compute pos
        area = Area::ApplyAnchor(contextArea, area.size, anchor);

        // Apply children transform
        applyTransform(childEntityIndex, area);
    }
}

template<kF::UI::Internal::Axis DistributionAxis>
void kF::UI::Internal::LayoutBuilder::computeLayoutChildrenArea(const Area &contextArea, const Layout &layout) noexcept
{
    const auto &counter = _traverseContext->counter();
    const auto childCount = static_cast<Pixel>(counter.size());
    const auto totalSpacing = layout.spacing * (childCount - 1.0f);
    Point flexCount {};
    Size freeSpace {};

    if constexpr (DistributionAxis == Axis::Horizontal)
        freeSpace.width = contextArea.size.width - totalSpacing;
    else
        freeSpace.height = contextArea.size.height - totalSpacing;

    for (const auto childEntityIndex : counter) {
        const auto &constraints = _traverseContext->constraintsAt(childEntityIndex);
        auto &area = _traverseContext->areaAt(childEntityIndex);

        // Compute width
        area.size.width = Internal::ComputeLayoutChildSize<DistributionAxis == Axis::Horizontal>(
            flexCount.x,
            freeSpace.width,
            contextArea.size.width,
            constraints.minSize.width,
            constraints.maxSize.width
        );

        // Compute height
        area.size.height = Internal::ComputeLayoutChildSize<DistributionAxis == Axis::Vertical>(
            flexCount.y,
            freeSpace.height,
            contextArea.size.height,
            constraints.minSize.height,
            constraints.maxSize.height
        );
    }

    Point offset = contextArea.pos;
    const auto flexSize = Size {
        flexCount.x > 0.0f ? freeSpace.width / flexCount.x : 0.0f,
        flexCount.y > 0.0f ? freeSpace.height / flexCount.y : 0.0f
    };

    auto spacing = layout.spacing;
    if (childCount >= 2.0 && flexSize.width == 0.0f && flexSize.height == 0.0f && layout.spacingType == SpacingType::SpaceBetween) [[unlikely]] {
        if constexpr (DistributionAxis == Axis::Horizontal)
            spacing += freeSpace.width / (childCount - 1u);
        else
            spacing += freeSpace.height / (childCount - 1u);
    }

    for (const auto childEntityIndex : counter) {
        const auto &constraints = _traverseContext->constraintsAt(childEntityIndex);
        auto &area = _traverseContext->areaAt(childEntityIndex);

        // Compute size of flex items
        if constexpr (DistributionAxis == Axis::Horizontal) {
            if (constraints.maxSize.width == PixelInfinity) [[likely]]
                area.size.width = flexSize.width;
        } else {
            if (constraints.maxSize.height == PixelInfinity) [[likely]]
                area.size.height = flexSize.height;
        }

        // Compute position
        Internal::ComputeLayoutChildPosition<DistributionAxis>(offset, area, contextArea, spacing, layout.anchor);

        // Apply children transform
        applyTransform(childEntityIndex, area);
    }
}

template<bool Distribute>
UI::Pixel kF::UI::Internal::ComputeLayoutChildSize([[maybe_unused]] Pixel &flexCount, [[maybe_unused]] Pixel &freeSpace,
        const Pixel parent, const Pixel min, const Pixel max) noexcept
{
    Pixel out {};

    if constexpr (Distribute) {
        if (max == PixelInfinity) [[likely]] {
            ++flexCount;
        } else [[unlikely]] {
            out = Internal::ComputeSize<BoundType::Fixed>(parent, min, max);
            freeSpace -= out;
        }
    } else {
        out = Internal::ComputeSize<BoundType::Unknown>(parent, min, max);
    }
    return out;
}

template<kF::UI::Internal::BoundType Bound>
UI::Pixel kF::UI::Internal::ComputeSize([[maybe_unused]] const Pixel parent, [[maybe_unused]] const Pixel min, const Pixel max) noexcept
{
    constexpr auto ComputeInfinite = [](const auto parent, const auto min) {
        return std::max(parent, min);
    };

    constexpr auto ComputeFinite = [](const auto parent, const auto min, const auto max) {
        return std::max(std::min(parent, max), min);
    };

    if constexpr (Bound == BoundType::Unknown) {
        if (max == PixelInfinity) [[likely]]
            return ComputeInfinite(parent, min);
        else [[unlikely]]
            return ComputeFinite(parent, min, max);
    } else {
        if constexpr (Bound == BoundType::Infinite)
            return ComputeInfinite(parent, min);
        else
            return ComputeFinite(parent, min, max);
    }
}

template<kF::UI::Internal::Axis DistributionAxis>
void kF::UI::Internal::ComputeLayoutChildPosition(Point &offset, Area &area, const Area &contextArea, const Pixel spacing, const Anchor anchor) noexcept
{
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

    area = Area::ApplyAnchor(transformedArea, area.size, anchor);
}

template<kF::UI::Internal::Axis DistributionAxis>
void kF::UI::Internal::LayoutBuilder::computeFlexLayoutChildrenArea(const Area &contextArea, const Layout &layout) noexcept
{

}

void kF::UI::Internal::LayoutBuilder::applyTransform(const ECS::EntityIndex entityIndex, Area &area) noexcept
{
    // Ensure entity has a transform component
    if (!Core::HasFlags(_traverseContext->nodeAt(entityIndex).componentFlags, ComponentFlags::Transform)) [[likely]]
        return;

    // Apply child scale transformation
    const auto entity = _uiSystem->getTable<TreeNode>().entities().at(entityIndex);
    auto &transform = _uiSystem->get<Transform>(entity);

    // Update transform
    if (transform.event) [[unlikely]]
        transform.event(transform, area);

    const auto scaledSize = Size {
        transform.minSize.width + (area.size.width - transform.minSize.width) * transform.scale.width,
        transform.minSize.height + (area.size.height - transform.minSize.height) * transform.scale.height
    };
    area.pos = Point {
        area.pos.x + transform.offset.x + area.size.width * transform.origin.x - scaledSize.width * transform.origin.x,
        area.pos.y + transform.offset.y + area.size.height * transform.origin.y - scaledSize.height * transform.origin.y
    };
    area.size = scaledSize;
}
