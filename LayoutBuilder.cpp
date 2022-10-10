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
    template <Accumulate AccumulateValue>
    void ComputeAxisHugConstraint(Pixel &lhs, const Pixel rhs, [[maybe_unused]] Pixel &maxFixed) noexcept;

    /** @brief Compute child size inside parent space according to min/max range */
    template <BoundType Bound>
    [[nodiscard]] Pixel ComputeSize([[maybe_unused]] const Pixel parent, [[maybe_unused]] const Pixel min, const Pixel max) noexcept;
}

UI::DepthUnit UI::Internal::LayoutBuilder::build(void) noexcept
{
    // Prepare context caches
    auto &nodeTable = _uiSystem->getTable<TreeNode>();
    _traverseContext->setupContext(
        nodeTable.count(),
        nodeTable.begin(),
        _uiSystem->getTable<Area>().begin(),
        _uiSystem->getTable<Depth>().begin());

    // Traverse from each childrenless items to root
    for (const auto &node : nodeTable)
    {
        // If the node does not have children, we go upward from here
        if (node.children.empty()) [[unlikely]]
        {
            { // Setup the entity for traversal
                const auto entityIndex = _traverseContext->entityIndexOf(node);
                _traverseContext->setupEntity(
                    nodeTable.entities().at(entityIndex),
                    entityIndex);
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
        area.pos = Point{};
        area.size = _uiSystem->windowSize();

        // Reset depth cache before traversal
        _maxDepth = DepthUnit{};
    }

    // Traverse from top to bottom
    traverseAreas();

    return _maxDepth;
}

void UI::Internal::LayoutBuilder::traverseConstraints(void) noexcept
{
    constexpr auto GetCounterInsertIndex = [](UISystem &uiSystem, const ECS::Entity entity, const TreeNode &parentNode, const Internal::TraverseContext::Counter &counter) -> std::uint32_t
    {
        if (counter.empty())
            return 0u;

        const auto entityParentIndex = Core::Distance<std::uint32_t>(parentNode.children.begin(), parentNode.children.find(entity));
        auto insertIndex = 0u;

        // @todo Optimize insertion to reduce indirections
        for (const auto childEntityIndex : counter)
        {
            const auto childEntity = uiSystem.getTable<TreeNode>().entities().at(childEntityIndex);
            const auto childParentIndex = Core::Distance<std::uint32_t>(parentNode.children.begin(), parentNode.children.find(childEntity));
            if (childParentIndex > entityParentIndex)
                break;
            ++insertIndex;
        }
        return insertIndex;
    };

    { // For each traversed node, we build constraints from children to parents
        const auto entity = _traverseContext->entity();
        TreeNode &node = _traverseContext->node();
        Constraints &constraints = _traverseContext->constraints();

        // If the node has explicit constraints use it, else we use default fill constraints
        if (!Core::HasFlags(node.componentFlags, ComponentFlags::Constraints)) [[likely]]
        {
            constraints = Constraints::Make(Fill(), Fill());
        }
        else [[unlikely]]
        {
            constraints = _uiSystem->get<Constraints>(entity);
        }

        // If the node has at least one child, compute its size accordingly to its children
        if (!node.children.empty()) [[likely]]
        {
            // Update self constraints depending on children constraints
            if (!Core::HasFlags(node.componentFlags, ComponentFlags::Layout)) [[likely]]
            {
                computeChildrenHugConstraints<Accumulate::No, Accumulate::No>(
                    constraints, Pixel{}, constraints.maxSize.width == PixelHug, constraints.maxSize.height == PixelHug);
            }
            else [[unlikely]]
            {
                buildLayoutConstraints(constraints);
            }
        }

        // Ensure constraints is in range
        if (constraints.minSize.width > constraints.maxSize.width)
            constraints.maxSize.width = constraints.minSize.width;
        if (constraints.minSize.height > constraints.maxSize.height)
            constraints.maxSize.height = constraints.minSize.height;

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
        const auto insertIndex = GetCounterInsertIndex(*_uiSystem, entity, parentNode, counter);
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
    switch (layout.flowType)
    {
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
        computeFlexChildrenHugConstraints<Axis::Vertical, GetXAxis, GetYAxis>(constraints, layout, hugWidth, hugHeight);
        break;
    case FlowType::FlexRow:
        computeFlexChildrenHugConstraints<Axis::Horizontal, GetYAxis, GetXAxis>(constraints, layout, hugWidth, hugHeight);
        break;
    }

    if (hugWidth)
    {
        constraints.maxSize.width += layout.padding.left + layout.padding.right;
        // constraints.minSize.width = std::max(constraints.minSize.width, constraints.maxSize.width);
    }

    if (hugHeight)
    {
        constraints.maxSize.height += layout.padding.top + layout.padding.bottom;
        // constraints.minSize.height = std::max(constraints.minSize.height, constraints.maxSize.height);
    }
}

template <kF::UI::Internal::Accumulate AccumulateX, kF::UI::Internal::Accumulate AccumulateY>
void UI::Internal::LayoutBuilder::computeChildrenHugConstraints(Constraints &constraints, [[maybe_unused]] const Pixel spacing,
                                                                const bool hugWidth, const bool hugHeight) noexcept
{
    Size maxFixed{};
    for (const auto childEntityIndex : _traverseContext->counter())
    {
        const auto &rhs = _traverseContext->constraintsAt(childEntityIndex);
        if (hugWidth)
            ComputeAxisHugConstraint<AccumulateX>(constraints.maxSize.width, rhs.maxSize.width, maxFixed.width);
        if (hugHeight)
            ComputeAxisHugConstraint<AccumulateY>(constraints.maxSize.height, rhs.maxSize.height, maxFixed.height);
    }

    // If we have at least one fixed children we use it as fill value
    if ((constraints.maxSize.width == PixelInfinity) & (maxFixed.width != 0))
        constraints.maxSize.width = maxFixed.width;
    if ((constraints.maxSize.height == PixelInfinity) & (maxFixed.height != 0))
        constraints.maxSize.height = maxFixed.height;

    if constexpr (AccumulateX == Accumulate::Yes)
    {
        if (hugWidth)
            constraints.maxSize.width += spacing * static_cast<Pixel>(_traverseContext->counter().size() - 1);
    }
    else if constexpr (AccumulateY == Accumulate::Yes)
    {
        if (hugHeight)
            constraints.maxSize.height += spacing * static_cast<Pixel>(_traverseContext->counter().size() - 1);
    }
}

template <kF::UI::Internal::Accumulate AccumulateValue>
void kF::UI::Internal::ComputeAxisHugConstraint(Pixel &lhs, const Pixel rhs, [[maybe_unused]] Pixel &maxFixed) noexcept
{
    if constexpr (AccumulateValue == Accumulate::Yes)
    {
        if (lhs != PixelInfinity)
            lhs = rhs == PixelInfinity ? PixelInfinity : lhs + rhs;
    }
    else
    {
        // if ((lhs == PixelInfinity) | (rhs == PixelInfinity)) [[likely]]
        //     lhs = std::min(lhs, rhs);
        // else
        lhs = std::max(lhs, rhs);
        if ((rhs != PixelInfinity) & (rhs > maxFixed))
            maxFixed = rhs;
    }
}

template <kF::UI::Internal::Axis DistributionAxis, auto GetX, auto GetY>
void UI::Internal::LayoutBuilder::computeFlexChildrenHugConstraints(Constraints &constraints, const Layout &layout,
                                                                    [[maybe_unused]] const bool hugWidth, [[maybe_unused]] const bool hugHeight) noexcept
{
    if constexpr (DistributionAxis == Axis::Vertical)
    {
        kFAssert(!hugWidth,
                 "UI::LayoutBuilder::computeFlexChildrenConstraints: FlowType::FlexColumn cannot take Hug as width constraint");
        kFAssert(constraints.maxSize.width != PixelInfinity,
                 "UI::LayoutBuilder::computeFlexChildrenConstraints: FlowType::FlexColumn cannot take Fill width constraint when height is Hug constraint");
    }
    else
    {
        kFAssert(!hugHeight,
                 "UI::LayoutBuilder::computeFlexChildrenConstraints: FlowType::FlexRow cannot take Hug as height constraint");
        kFAssert(constraints.maxSize.height != PixelInfinity,
                 "UI::LayoutBuilder::computeFlexChildrenConstraints: FlowType::FlexRow cannot take Fill height constraint when width is Hug constraint");
    }

    auto childIndexRange = std::as_const(_traverseContext->counter()).toRange();
    const auto lineWidth = GetX(constraints.maxSize);

    // Loop over each child to compute self constraints
    while (childIndexRange.from != childIndexRange.to)
    {
        Pixel lineHeight{};
        Pixel lineRemain{lineWidth};

        // Compute current line metrics
        computeFlexLayoutChildrenLineMetrics<GetX, GetY>(childIndexRange, layout.flexSpacing, lineWidth, lineHeight, lineRemain);

        // Increment constraints hug axis by lineHeight
        if (auto &hugConstraint = GetY(constraints.maxSize); hugConstraint != PixelInfinity && lineHeight != PixelInfinity)
            hugConstraint += lineHeight + layout.spacing * static_cast<Pixel>(childIndexRange.from != childIndexRange.to);
        else
            hugConstraint = PixelInfinity;
    }
}

void UI::Internal::LayoutBuilder::traverseAreas(void) noexcept
{
    // Set self depth
    _traverseContext->depth().depth = _maxDepth++;

    Area lastClip{DefaultClip};
    bool clip{false};
    const auto &node = _traverseContext->node();
    auto &area = _traverseContext->area();

    // Build position of children using the context node area
    if (!Core::HasFlags(node.componentFlags, ComponentFlags::Layout)) [[likely]]
        computeChildrenArea(area, Anchor{});
    else [[unlikely]]
        buildLayoutArea(area);

    if (const auto &nodeCounter = _traverseContext->counter(); !nodeCounter.empty()) [[likely]]
    {
        { // Process clip
            const auto &clipTable = _uiSystem->getTable<Clip>();
            const auto clipIndex = clipTable.getUnstableIndex(_traverseContext->entity());
            if (clipIndex != ECS::NullEntityIndex) [[unlikely]]
            {
                const auto currentClip = _traverseContext->currentClip();
                if (lastClip.contains(currentClip)) [[likely]]
                {
                    lastClip = currentClip;
                    clip = true;
                    const auto clipArea = Area::ApplyPadding(area, clipTable.atIndex(clipIndex).padding);
                    _traverseContext->setClip(clipArea, _maxDepth);
                }
            }
        }

        // Traverse each child
        std::uint32_t childIndex{0u};
        for (const auto childEntityIndex : nodeCounter)
        {
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
    const auto childIndexRange = std::as_const(_traverseContext->counter()).toRange();

    switch (layout.flowType)
    {
    case FlowType::Stack:
        computeChildrenArea(transformedArea, layout.anchor);
        break;
    case FlowType::Column:
        computeLayoutChildrenArea<Axis::Vertical, GetYAxis, GetXAxis>(transformedArea, layout, childIndexRange);
        break;
    case FlowType::Row:
        computeLayoutChildrenArea<Axis::Horizontal, GetXAxis, GetYAxis>(transformedArea, layout, childIndexRange);
        break;
    case FlowType::FlexColumn:
        computeFlexLayoutChildrenArea<Axis::Vertical, GetXAxis, GetYAxis>(transformedArea, layout);
        break;
    case FlowType::FlexRow:
        computeFlexLayoutChildrenArea<Axis::Horizontal, GetYAxis, GetXAxis>(transformedArea, layout);
        break;
    }
}

void UI::Internal::LayoutBuilder::computeChildrenArea(const Area &contextArea, const Anchor anchor) noexcept
{
    for (const auto childEntityIndex : _traverseContext->counter())
    {
        const auto &constraints = _traverseContext->constraintsAt(childEntityIndex);
        auto &area = _traverseContext->areaAt(childEntityIndex);

        // Compute size
        area.size.width = Internal::ComputeSize<BoundType::Unknown>(
            contextArea.size.width,
            constraints.minSize.width,
            constraints.maxSize.width);
        area.size.height = Internal::ComputeSize<BoundType::Unknown>(
            contextArea.size.height,
            constraints.minSize.height,
            constraints.maxSize.height);

        // Compute pos
        area = Area::ApplyAnchor(contextArea, area.size, anchor);

        // Apply children transform
        applyTransform(childEntityIndex, area);
    }
}

template <kF::UI::Internal::Axis DistributionAxis, auto GetX, auto GetY>
void UI::Internal::LayoutBuilder::computeLayoutChildrenArea(const Area &contextArea, const Layout &layout, const EntityIndexRange &childIndexRange) noexcept
{
    const auto childCount = static_cast<Pixel>(childIndexRange.size());
    const auto totalSpacing = layout.spacing * (childCount - 1.0f);
    Pixel flexCount{};
    Pixel freeSpace = GetX(contextArea.size) - totalSpacing;

    for (const auto childEntityIndex : childIndexRange)
    {
        const auto &constraints = _traverseContext->constraintsAt(childEntityIndex);
        auto &area = _traverseContext->areaAt(childEntityIndex);

        // Non-distributed axis
        GetY(area.size) = Internal::ComputeSize<BoundType::Unknown>(GetY(contextArea.size), GetY(constraints.minSize), GetY(constraints.maxSize));

        // Distributed axis
        if (GetX(constraints.maxSize) == PixelInfinity) [[likely]]
        {
            ++flexCount;
        }
        else [[unlikely]]
        {
            GetX(area.size) = Internal::ComputeSize<BoundType::Fixed>(GetX(contextArea.size), GetX(constraints.minSize), GetX(constraints.maxSize));
            freeSpace -= GetX(area.size);
        }
    }

    Point offset = contextArea.pos;
    const auto flexSize = flexCount ? freeSpace / flexCount : 0.0f;

    auto spacing = layout.spacing;
    if (childCount >= 2.0 && flexSize == 0.0f && layout.spacingType == SpacingType::SpaceBetween) [[unlikely]]
        spacing += freeSpace / (childCount - 1u);

    // Compute global anchor offset
    const auto globalAnchorOffset = [](const auto &contextArea, const auto &layout, const auto flexCount, const auto freeSpace)
    {
        const Pixel totalWidth = GetX(contextArea.size) - Core::BranchlessIf(flexCount, 0.0f, freeSpace);
        Size size;
        GetX(size) = totalWidth;
        GetY(size) = GetY(contextArea.size);
        const auto anchoredArea = Area::ApplyAnchor(contextArea, size, layout.anchor);
        return anchoredArea.pos - contextArea.pos;
    }(contextArea, layout, flexCount, freeSpace);

    for (const auto childEntityIndex : childIndexRange)
    {
        const auto &constraints = _traverseContext->constraintsAt(childEntityIndex);
        auto &area = _traverseContext->areaAt(childEntityIndex);

        // Compute size of flex items
        if (GetX(constraints.maxSize) == PixelInfinity) [[likely]]
            GetX(area.size) = flexSize;

        { // Compute child position
            Area transformedArea{contextArea};
            GetX(transformedArea.size) = GetX(area.size);
            GetX(transformedArea.pos) = GetX(offset);
            GetY(transformedArea.pos) = GetY(offset);
            GetX(offset) += GetX(area.size) + spacing;
            area = Area::ApplyAnchor(transformedArea, area.size, layout.anchor);
            area.pos += globalAnchorOffset;
        }

        // Apply children transform
        applyTransform(childEntityIndex, area);
    }
}

template <kF::UI::Internal::BoundType Bound>
UI::Pixel kF::UI::Internal::ComputeSize([[maybe_unused]] const Pixel parent, [[maybe_unused]] const Pixel min, const Pixel max) noexcept
{
    constexpr auto ComputeInfinite = [](const auto parent, const auto min)
    {
        return std::max(parent, min);
    };

    constexpr auto ComputeFinite = [](const auto parent, const auto min, const auto max)
    {
        return std::max(max, min);
        // return std::max(std::min(parent, max), min);
    };

    if constexpr (Bound == BoundType::Unknown)
    {
        if (max == PixelInfinity) [[likely]]
            return ComputeInfinite(parent, min);
        else [[unlikely]]
            return ComputeFinite(parent, min, max);
    }
    else
    {
        if constexpr (Bound == BoundType::Infinite)
            return ComputeInfinite(parent, min);
        else
            return ComputeFinite(parent, min, max);
    }
}

template <kF::UI::Internal::Axis DistributionAxis, auto GetX, auto GetY>
void UI::Internal::LayoutBuilder::computeFlexLayoutChildrenArea(const Area &contextArea, const Layout &layout) noexcept
{
    auto childIndexRange = std::as_const(_traverseContext->counter()).toRange();
    const auto lineWidth = GetX(contextArea.size);
    Pixel totalHeight{};

    // Loop over each child to compute self constraints
    while (childIndexRange.from != childIndexRange.to)
    { // We know that we have at least 1 child
        const auto lineBeginChildIndex = childIndexRange.from;
        Pixel lineHeight{};
        Pixel lineRemain{lineWidth};

        // Compute current line metrics
        computeFlexLayoutChildrenLineMetrics<GetX, GetY>(childIndexRange, layout.flexSpacing, lineWidth, lineHeight, lineRemain);

        UI::Area lineArea;
        GetX(lineArea.pos) = GetX(contextArea.pos);
        GetY(lineArea.pos) = GetY(contextArea.pos) + totalHeight;
        GetX(lineArea.size) = lineWidth;
        GetY(lineArea.size) = lineHeight;
        const UI::Layout lineLayout{
            .flowType = DistributionAxis == Axis::Vertical ? FlowType::Row : FlowType::Column,
            .anchor = layout.flexAnchor,
            .spacingType = layout.flexSpacingType,
            .spacing = layout.flexSpacing};
        computeLayoutChildrenArea<DistributionAxis == Axis::Vertical ? Axis::Horizontal : Axis::Vertical, GetX, GetY>(
            lineArea, lineLayout, EntityIndexRange{lineBeginChildIndex, childIndexRange.from});

        // Increment distributed axis offset
        totalHeight += lineHeight + layout.spacing * static_cast<Pixel>(childIndexRange.from != childIndexRange.to);
    }

    // If totalHeight is equal to contextArea height we don't need to anchor children
    if (totalHeight == GetY(contextArea.pos) + GetY(contextArea.size))
        return;

    // Compute anchor offset
    const auto anchorArea = Area::ApplyAnchor(contextArea, Size(lineWidth, totalHeight), layout.anchor);
    const auto anchorOffset = anchorArea.pos - contextArea.pos;

    // If anchor offset is null we don't need to anchor children
    if (anchorOffset == Point())
        return;

    // Anchor all children
    for (auto childIndex : _traverseContext->counter())
        _traverseContext->areaAt(childIndex).pos += anchorOffset;
}

template <auto GetX, auto GetY>
void UI::Internal::LayoutBuilder::computeFlexLayoutChildrenLineMetrics(
    EntityIndexRange &childIndexRange, const Pixel spacing, const Pixel lineWidth, Pixel &lineHeight, Pixel &lineRemain) noexcept
{
    // Loop over the current line
    while (true)
    {
        const auto &childConstraints = _traverseContext->constraintsAt(*childIndexRange.from);
        const auto childMin = GetX(childConstraints.minSize);

        // If we cannot fit this child anyway go to the next line (if no child are in this line we must position the item anyway)
        if (childMin && childMin > lineRemain && lineRemain != lineWidth)
            break;

        // Find child maximum size
        const auto childMax = GetX(childConstraints.maxSize) == PixelInfinity ? lineWidth : GetX(childConstraints.maxSize);

        // Try to insert maximum children size and spacing
        if (const auto totalInsert = childMax + spacing; lineRemain > totalInsert)
            lineRemain -= totalInsert;
        // If the size doesn't fit we break the line
        else
            lineRemain = 0;

        // Get maximum height of all line's children
        lineHeight = std::max(lineHeight, GetY(childConstraints.maxSize));

        // Increment and break line when reaching end of range or if lineRemain is zero
        if (++childIndexRange.from == childIndexRange.to || !lineRemain)
            break;
    }
}

void UI::Internal::LayoutBuilder::applyTransform(const ECS::EntityIndex entityIndex, Area &area) noexcept
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

    const auto scaledSize = Size{
        transform.minSize.width + (area.size.width - transform.minSize.width) * transform.scale.width,
        transform.minSize.height + (area.size.height - transform.minSize.height) * transform.scale.height};
    area.pos = Point{
        area.pos.x + transform.offset.x + area.size.width * transform.origin.x - scaledSize.width * transform.origin.x,
        area.pos.y + transform.offset.y + area.size.height * transform.origin.y - scaledSize.height * transform.origin.y};
    area.size = scaledSize;
}
