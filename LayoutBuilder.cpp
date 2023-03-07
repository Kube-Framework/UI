/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Layout processor
 */

#include <Kube/Core/Assert.hpp>

#include "LayoutBuilder.hpp"
#include "UISystem.hpp"

using namespace kF;

namespace kF::UI
{
    constexpr auto ComputeTotalSpacing(const auto childCount, const auto spacing) noexcept
        { return std::max(Pixel(childCount) - 1.0f, 0.0f) * spacing; };

    constexpr auto ComputeFillSize(
        const auto size,
        const auto childCount,
        const auto isDistributed,
        const auto spacing,
        const auto totalPadding,
        const auto totalFixed,
        const auto fillCount
    ) noexcept
    {
        if (isDistributed)
            return (size - (totalFixed + totalPadding + ComputeTotalSpacing(childCount, spacing))) / fillCount;
        else
            return size - totalPadding;
    };
}

UI::DepthUnit UI::Internal::LayoutBuilder::build(void) noexcept
{
    // Prepare context caches
    auto &nodeTable = _uiSystem.getTable<TreeNode>();
    _traverseContext.setupContext(
        nodeTable.count(),
        nodeTable.entities().begin(),
        nodeTable.begin(),
        _uiSystem.getTable<Area>().begin(),
        _uiSystem.getTable<Depth>().begin()
    );

    // Query root entity
    const auto rootEntity = Item::GetEntity(_uiSystem.root());
    const auto rootEntityIndex = _traverseContext.entityIndexOf(nodeTable.get(rootEntity));

    {// Resolve simple constraints during first pass
        _traverseContext.setupEntity(rootEntity, rootEntityIndex);
        discoverConstraints();
    }

    // Setup traverse context of the window
    const auto windowSize = _uiSystem.windowSize();
    auto windowConstraints = Constraints::Make(UI::Strict(windowSize.width), UI::Strict(windowSize.height));
    const TraverseContext::ResolveData windowResolveData {
        .constraints = &windowConstraints,
        .layout = &_defaultLayout,
        .fillSize = windowSize
    };

    { // Resolve complex constraints into fixed sizes during second pass
        _traverseContext.setupEntity(rootEntity, rootEntityIndex);
        resolveConstraints(windowResolveData);
    }

    { // Resolve areas during third pass
        _traverseContext.areaAt(rootEntityIndex) = Area { .size = windowSize };
        _traverseContext.setupEntity(rootEntity, rootEntityIndex);
        resolveAreas(windowResolveData);
    }

    return _maxDepth;
}
void UI::Internal::LayoutBuilder::discoverConstraints(void) noexcept
{
    // Prepare current context resolve data
    TraverseContext::ResolveData &data = [this] -> auto & {
        // Query context resolve data
        TraverseContext::ResolveData &data = _traverseContext.resolveData();

        // Query context node
        data.node = &_traverseContext.node();

        // Use explicit constraints if defined or use default fill constraints
        data.constraints = &_traverseContext.constraints();
        if (!Core::HasFlags(data.node->componentFlags, ComponentFlags::Constraints)) [[likely]]
            *data.constraints = Constraints::Make(Fill(), Fill());
        else [[unlikely]]
            *data.constraints = _uiSystem.get<Constraints>(_traverseContext.entity());

        // Resolve mirror constraints that have a fixed opposite
        constexpr auto ResolveStrictMirror = [](auto &constraint, const auto oppositeConstraint) {
            if ((constraint == PixelMirror) & IsFixedConstraint(oppositeConstraint))
                constraint = oppositeConstraint;
        };
        ResolveStrictMirror(
            data.constraints->maxSize.width,
            data.constraints->maxSize.height
        );
        ResolveStrictMirror(
            data.constraints->maxSize.height,
            data.constraints->maxSize.width
        );

        // Use explicit layout if defined or use default stack layout
        data.layout = &[this, &data](void) -> const Layout & {
            if (!Core::HasFlags(data.node->componentFlags, ComponentFlags::Layout)) [[likely]]
                return _defaultLayout;
            else
                return _uiSystem.get<Layout>(_traverseContext.entity());
        }();

        return data;
    }();

    // Discover every chuild entity index
    data.children.reserve(data.node->children.size());
    for (const auto childEntity : data.node->children) {
        const auto childEntityIndex = _traverseContext.entityIndexOf(childEntity);
        data.children.push(childEntityIndex);
    }

    // Resolve children constraints and keep meta-data
    for (const auto childEntityIndex : data.children) {
        // Top-bottom recursion
        const auto childEntity = _traverseContext.entityAt(childEntityIndex);
        _traverseContext.setupEntity(childEntity, childEntityIndex);
        discoverConstraints();

        // Update resolve data cache
        const Constraints &childConstraints = _traverseContext.constraintsAt(childEntityIndex);
        data.totalFixed += Size(
            std::max(childConstraints.maxSize.width, 0.0f),
            std::max(childConstraints.maxSize.height, 0.0f)
        );
        data.maxFixed = Size(
            std::max(childConstraints.maxSize.width, data.maxFixed.width),
            std::max(childConstraints.maxSize.height, data.maxFixed.height)
        );
        data.fillCount += Size(
            Pixel(childConstraints.maxSize.width == PixelFill),
            Pixel(childConstraints.maxSize.height == PixelFill)
        );
        data.unresolvedCount += Size(
            Pixel((childConstraints.maxSize.width == PixelHug) | (childConstraints.maxSize.width == PixelMirror)),
            Pixel((childConstraints.maxSize.height == PixelHug) | (childConstraints.maxSize.height == PixelMirror))
        );
    }

    // Resolve hug constraints
    constexpr auto ResolveHug = [](
        auto &constraint,
        const auto childCount,
        const auto isDistributed,
        const auto spacing,
        const auto totalPadding,
        const auto totalFixed,
        const auto maxFixed,
        const auto fillCount,
        const auto unresolvedCount
    ) {
        // Check if constraint need to get resolved
        if (constraint != PixelHug)
            return;
        // Fill if distributed axis has at least one filled child
        else if (isDistributed & bool(fillCount))
            constraint = PixelFill;
        // Axis is undefined
        else if (unresolvedCount)
            constraint = PixelHug;
        // Distributed axis is fixed
        else if (isDistributed)
            constraint = totalFixed + totalPadding + ComputeTotalSpacing(childCount, spacing);
        // Axis is stacked and all children are filling
        else if (childCount == std::uint32_t(fillCount))
            constraint = PixelFill;
        // Axis is stacked but not filled
        else
            constraint = maxFixed + totalPadding;
    };
    ResolveHug(
        data.constraints->maxSize.width,
        data.children.size(),
        data.layout->flowType == FlowType::Row,
        data.layout->spacing,
        data.layout->padding.left + data.layout->padding.right,
        data.totalFixed.width,
        data.maxFixed.width,
        data.fillCount.width,
        data.unresolvedCount.width
    );
    ResolveHug(
        data.constraints->maxSize.height,
        data.children.size(),
        data.layout->flowType == FlowType::Column,
        data.layout->spacing,
        data.layout->padding.top + data.layout->padding.bottom,
        data.totalFixed.height,
        data.maxFixed.height,
        data.fillCount.height,
        data.unresolvedCount.height
    );
}

void UI::Internal::LayoutBuilder::resolveConstraints(const TraverseContext::ResolveData &parentData) noexcept
{
    TraverseContext::ResolveData &data = _traverseContext.resolveData();

    // Query context item size and use it as max constraints
    data.constraints->maxSize = querySize(parentData.fillSize);

    // Compute fill size
    data.fillSize.width = ComputeFillSize(
        data.constraints->maxSize.width,
        data.children.size(),
        data.layout->flowType == FlowType::Row,
        data.layout->spacing,
        data.layout->padding.left + data.layout->padding.right,
        data.totalFixed.width,
        data.fillCount.width
    );
    data.fillSize.height = ComputeFillSize(
        data.constraints->maxSize.height,
        data.children.size(),
        data.layout->flowType == FlowType::Column,
        data.layout->spacing,
        data.layout->padding.top + data.layout->padding.bottom,
        data.totalFixed.height,
        data.fillCount.height
    );

    // Resolve children constraints
    for (const auto childEntityIndex : data.children) {
        // Top-bottom recursion
        const auto childEntity = _traverseContext.entityAt(childEntityIndex);
        _traverseContext.setupEntity(childEntity, childEntityIndex);
        resolveConstraints(data);
    }
}

UI::Size UI::Internal::LayoutBuilder::querySize(const Size &parentFillSize) noexcept
{
    TraverseContext::ResolveData &data = _traverseContext.resolveData();
    Size output { data.constraints->maxSize };

    // Resolve fill constraints
    constexpr auto ResolveFill = [](
        auto &constraint,
        const auto parentFillSize
    ) {
        if (constraint == PixelFill)
            constraint = parentFillSize;
    };
    ResolveFill(output.width, parentFillSize.width);
    ResolveFill(output.height, parentFillSize.height);

    // Resolve mirror constraints that have a fixed opposite
    constexpr auto ResolveStrictMirror = [](auto &constraint, const auto oppositeConstraint) {
        if ((constraint == PixelMirror) & IsFixedConstraint(oppositeConstraint))
            constraint = oppositeConstraint;
    };
    ResolveStrictMirror(
        output.width,
        output.height
    );
    ResolveStrictMirror(
        output.height,
        output.width
    );

    // If item still has unresolved constraints, we need to resolve recursively
    if ((output.width == PixelHug) | (output.height == PixelHug)
        | bool(data.unresolvedCount.width) | bool(data.unresolvedCount.height)) { // @todo ensure that checking unresoled is correct
        // Make an initial hug guess
        constexpr auto GuessFillSize = [](
            auto &out,
            const auto childCount,
            const auto isDistributed,
            const auto spacing,
            const auto totalPadding,
            const auto totalFixed,
            const auto fillCount,
            const auto parentFillSize
        ) {
            if (out == PixelHug)
                out = parentFillSize;
            out = ComputeFillSize(out, childCount, isDistributed, spacing, totalPadding, totalFixed, fillCount);
        };
        Size guessFillSize { output };
        GuessFillSize(
            guessFillSize.width,
            data.children.size(),
            data.layout->flowType == FlowType::Row,
            data.layout->spacing,
            data.layout->padding.left + data.layout->padding.right,
            data.totalFixed.width,
            data.fillCount.width,
            parentFillSize.width
        );
        GuessFillSize(
            guessFillSize.height,
            data.children.size(),
            data.layout->flowType == FlowType::Column,
            data.layout->spacing,
            data.layout->padding.top + data.layout->padding.bottom,
            data.totalFixed.height,
            data.fillCount.height,
            parentFillSize.height
        );

        // Resolve children constraints and store meta-data
        data.totalFixed = {};
        data.maxFixed = {};
        for (const auto childEntityIndex : data.children) {
            // Top-bottom recursion
            const auto childEntity = _traverseContext.entityAt(childEntityIndex);
            _traverseContext.setupEntity(childEntity, childEntityIndex);
            const auto childSize = querySize(guessFillSize);

            // Update resolve data cache
            const auto &childConstraint = _traverseContext.constraintsAt(childEntityIndex);
            constexpr auto UpdateFixedData = [](
                auto &totalFixed,
                auto &maxFixed,
                const auto childConstraint,
                const auto childSize
            ) {
                if (childConstraint != PixelFill) {
                    totalFixed += std::max(childSize, 0.0f);
                    maxFixed = std::max(childSize, maxFixed);
                }
            };
            UpdateFixedData(
                data.totalFixed.width,
                data.maxFixed.width,
                childConstraint.maxSize.width,
                childSize.width
            );
            UpdateFixedData(
                data.totalFixed.height,
                data.maxFixed.height,
                childConstraint.maxSize.height,
                childSize.height
            );
        }

        // Resolve hug constraints
        constexpr auto ResolveStrictHug = [](
            auto &constraint,
            const auto childCount,
            const auto isDistributed,
            const auto spacing,
            const auto totalPadding,
            const auto totalFixed,
            const auto maxFixed
        ) {
            // Check if constraint need to get resolved
            if (constraint != PixelHug)
                return;
            // Axis is distributed
            else if (isDistributed)
                constraint = totalFixed + totalPadding + ComputeTotalSpacing(childCount, spacing);
            // Axis is stacked
            else
                constraint = maxFixed + totalPadding;

        };
        ResolveStrictHug(
            output.width,
            data.children.size(),
            data.layout->flowType == FlowType::Row,
            data.layout->spacing,
            data.layout->padding.left + data.layout->padding.right,
            data.totalFixed.width,
            data.maxFixed.width
        );
        ResolveStrictHug(
            output.height,
            data.children.size(),
            data.layout->flowType == FlowType::Column,
            data.layout->spacing,
            data.layout->padding.top + data.layout->padding.bottom,
            data.totalFixed.height,
            data.maxFixed.height
        );
    }

    return output;
}

void UI::Internal::LayoutBuilder::resolveAreas(const TraverseContext::ResolveData &parentData) noexcept
{
    TraverseContext::ResolveData &data = _traverseContext.resolveData();

    // Set self depth
    _traverseContext.depth().depth = _maxDepth++;

    { // Resolve children areas
        // Query total fixed size
        data.totalFixed = {};
        data.maxFixed = {};
        for (const auto childEntityIndex : data.children) {
            const auto childSize = _traverseContext.constraintsAt(childEntityIndex).maxSize;
            data.totalFixed += childSize;
            data.maxFixed = Size(
                std::max(childSize.width, data.maxFixed.width),
                std::max(childSize.height, data.maxFixed.height)
            );
        }

        // Compute space between children
        const bool isWidthDistributed = data.layout->flowType == FlowType::Row;
        const bool isHeightDistributed = data.layout->flowType == FlowType::Column;
        const auto spaceBetween = [this, &data, isWidthDistributed, isHeightDistributed] {
            if (data.layout->spacingType == SpacingType::SpaceBetween) {
                const auto count = Pixel(data.children.size());
                if (isWidthDistributed)
                    return (data.constraints->maxSize.width - data.totalFixed.width) / count;
                else if (isHeightDistributed)
                    return (data.constraints->maxSize.height - data.totalFixed.height) / count;
            }
            return data.layout->spacing;
        }();

        // Compute final children area by anchoring content size inside padded area
        const auto anchor = data.layout->anchor;
        const UI::Area area = [this, &data, spaceBetween, anchor, isWidthDistributed, isHeightDistributed] {
            auto area = Area::ApplyPadding(_traverseContext.area(), data.layout->padding);
            const auto totalSpacing = spaceBetween * Pixel(data.children.size());
            const Size contentSize {
                isWidthDistributed ? data.totalFixed.width + totalSpacing : data.maxFixed.width,
                isHeightDistributed ? data.totalFixed.height + totalSpacing : data.maxFixed.height
            };
            area = Area::ApplyAnchor(area, contentSize, anchor);
            return area;
        }();

        // Determine start offset
        Point offset = area.pos;
        for (const auto childEntityIndex : data.children) {
            const auto childSize = _traverseContext.constraintsAt(childEntityIndex).maxSize;
            auto &childArea = _traverseContext.areaAt(childEntityIndex);
            childArea.pos = offset;
            childArea.size = childSize;
            if (isWidthDistributed)
                offset.x += childArea.size.width + spaceBetween;
            else
                childArea.size.width = std::max(childSize.width, area.size.width);
            if (isHeightDistributed)
                offset.y += childArea.size.height + spaceBetween;
            else
                childArea.size.height = std::max(childSize.height, area.size.height);

            // Apply anchor
            childArea = Area::ApplyAnchor(
                childArea,
                childSize,
                anchor
            );
        }
    }

    // Top-bottom recursion
    for (const auto childEntityIndex : data.children) {
        const auto childEntity = _traverseContext.entityAt(childEntityIndex);
        _traverseContext.setupEntity(childEntity, childEntityIndex);
        resolveAreas(data);
    }

    // Apply item transform
    applyTransform(_traverseContext.entityIndexOf(*data.node), _traverseContext.area());
}

void UI::Internal::LayoutBuilder::applyTransform(const ECS::EntityIndex entityIndex, Area &area) noexcept
{
    // Ensure entity has a transform component
    if (!Core::HasFlags(_traverseContext.nodeAt(entityIndex).componentFlags, ComponentFlags::Transform)) [[likely]]
        return;

    // Apply child scale transformation
    const auto entity = _uiSystem.getTable<TreeNode>().entities().at(entityIndex);
    auto &transform = _uiSystem.get<Transform>(entity);

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