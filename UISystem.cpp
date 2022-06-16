/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System
 */

#include <SDL2/SDL.h>

#include <Kube/GPU/GPU.hpp>
#include <Kube/UI/App.hpp>

#include "EventSystem.hpp"
#include "UISystem.hpp"

using namespace kF;

UI::Size UI::UISystem::GetWindowSize(void) noexcept
{
    const auto extent = GPU::GPUObject::Parent().swapchain().extent();
    kFEnsure(extent.width && extent.height,
        "UI::UISystem::GetWindowSize: Couldn't retreive display DPI");
    return Size(extent.width, extent.height);
}

UI::DPI UI::UISystem::GetWindowDPI(void) noexcept
{
    DPI dpi;
    kFEnsure(!SDL_GetDisplayDPI(0, &dpi.diagonal, &dpi.horizontal, &dpi.vertical),
        "UI::UISystem::GetWindowDPI: Couldn't retreive display DPI");
    return dpi;
}

UI::UISystem::~UISystem(void) noexcept
{
    _root.release();
}

UI::UISystem::UISystem(void) noexcept
    :   _windowSize(GetWindowSize()),
        _windowDPI(GetWindowDPI()),
        _renderer(*this),
        _mouseQueue(parent().getSystem<EventSystem>().addEventQueue<MouseEvent>()),
        _motionQueue(parent().getSystem<EventSystem>().addEventQueue<MotionEvent>()),
        _keyQueue(parent().getSystem<EventSystem>().addEventQueue<KeyEvent>())
{
    GPU::GPUObject::Parent().viewSizeDispatcher().add([this] {
        _windowSize = GetWindowSize();
        _windowDPI = GetWindowDPI();
        invalidate();
    });

    // Rebuild graph
    auto &graph = taskGraph();
    auto &computeTask = graph.add<&Renderer::batchPrimitives>(&_renderer);
    auto &transferTask = graph.add<&Renderer::transferPrimitives>(&_renderer);
    auto &dispatchTask = graph.add<&Renderer::dispatchInvalidFrame>(&_renderer);
    dispatchTask.after(computeTask);
    dispatchTask.after(transferTask);
}

bool UI::UISystem::tick(void) noexcept
{
    const auto currentFrame = _renderer.currentFrame();

    // Return if there are no items in the tree
    if (!_root) [[unlikely]]
        return false;

    // Process elapsed time
    processElapsedTime();

    // Process UI events
    processEventHandlers();

    // If the current frame is still valid, we only need to dispatch painter commands
    if (!isFrameInvalid(currentFrame)) {
        _renderer.dispatchValidFrame();
        return false;
    }

    // If the tree is invalid, compute areas then paint
    if (_invalidateTree) {
        // Process all areas
        processAreas();

        // Process all paint handlers
        processPainterAreas();
    }

    // Prepare painter to batch
    if (!_renderer.prepare()) [[unlikely]]
        return false;

    // Validate the current frame
    validateFrame(currentFrame);

    return true;
}

void kF::UI::UISystem::processEventHandlers(void) noexcept
{
    _mouseQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processMouseEventAreas(event);
    });
    _motionQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processMotionEventAreas(event);
    });
    _keyQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processKeyEventReceivers(event);
    });
}

void kF::UI::UISystem::processMouseEventAreas(const MouseEvent &event) noexcept
{
    auto &areaTable = getTable<Area>();
    auto &mouseTable = getTable<MouseEventArea>();

    // for (std::uint32_t index = mouseTable.count(); const auto &handler : Core::IteratorRange { mouseTable.rbegin(), mouseTable.rend() }) {
    //     auto &area = areaTable.get(mouseTable.entities().at(--index));
    for (std::uint32_t index {}; const auto &handler : mouseTable) {
        auto &area = areaTable.get(mouseTable.entities().at(index++));
        if (area.contains(event.pos)) [[unlikely]] {
            // kFInfo("[processMouseEventAreas] Area ", area, " hit by ", event.pos);
            const auto flags = handler.event(event, area);
            if (Core::HasFlags(flags, EventFlags::Invalidate)) [[likely]]
                invalidate();
            if (!Core::HasFlags(flags, EventFlags::Propagate)) [[likely]]
                break;
        }
    }
}

void kF::UI::UISystem::processMotionEventAreas(const MotionEvent &event) noexcept
{
    auto &areaTable = getTable<Area>();
    auto &motionTable = getTable<MotionEventArea>();

    for (std::uint32_t index {}; const auto &handler : motionTable) {
        auto &area = areaTable.get(motionTable.entities().at(index++));
        if (area.contains(event.pos)) [[unlikely]] {
            const auto flags = handler.event(event, area);
            if (Core::HasFlags(flags, EventFlags::Invalidate)) [[likely]]
                invalidate();
            if (!Core::HasFlags(flags, EventFlags::Propagate)) [[likely]]
                break;
        }
    }
}

void kF::UI::UISystem::processKeyEventReceivers(const KeyEvent &event) noexcept
{
    auto &keyTable = getTable<KeyEventReceiver>();

    for (const auto &handler : keyTable) {
        const auto flags = handler.event(event);
        if (Core::HasFlags(flags, EventFlags::Invalidate)) [[likely]]
            invalidate();
        if (!Core::HasFlags(flags, EventFlags::Propagate)) [[likely]]
            break;
    }
}

void UI::UISystem::processElapsedTime(void) noexcept
{
    // Query time
    const auto oldTick = _lastTick;
    _lastTick = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    // Compute elapsed time
    const auto elapsed = _lastTick - oldTick;
    bool invalidateState = false;

    // Process timers & animations
    if (oldTick) [[likely]]
        invalidateState |= processTimers(elapsed);
    invalidateState |= processAnimators(elapsed);

    // Invalidate UI
    if (invalidateState) [[likely]]
        invalidate();
}

bool UI::UISystem::processTimers(const std::int64_t elapsed) noexcept
{
    bool invalidateState = false;
    for (Timer &handler : getTable<Timer>()) {
        handler.elapsed += elapsed;
        if (handler.elapsed >= handler.interval) [[unlikely]] {
            invalidateState |= handler.event(elapsed);
            handler.elapsed = 0;
        }
    }
    return invalidateState;
}

bool UI::UISystem::processAnimators(const std::int64_t elapsed) noexcept
{
    bool invalidateState { false };

    for (Animator &handler : getTable<Animator>())
        invalidateState |= handler.tick(elapsed);
    return invalidateState;
}

void UI::UISystem::processPainterAreas(void) noexcept
{
    auto &painter = _renderer.painter();
    const auto &paintTable = getTable<PainterArea>();
    const auto &areaTable = getTable<Area>();
    const auto &depthTable = getTable<Depth>();
    const auto clipAreas = _traverseContext.clipAreas();
    const auto clipDepths = _traverseContext.clipDepths();
    std::uint32_t clipIndex {};
    const std::uint32_t clipCount { clipDepths.size<std::uint32_t>() };
    auto nextClipDepth = clipCount ? clipDepths[0] : ~static_cast<DepthUnit>(0);

    painter.clear();
    for (std::uint32_t index = 0u; const PainterArea &handler : paintTable) {
        const auto entity = paintTable.entities().at(index++);
        const auto entityIndex = areaTable.getUnstableIndex(entity);
        const Area &area = areaTable.atIndex(entityIndex);

        if (depthTable.atIndex(entityIndex).depth >= nextClipDepth) [[unlikely]] {
            painter.setClip(clipAreas[clipIndex]);
            if (++clipIndex != clipCount) [[likely]]
                nextClipDepth = clipDepths[clipIndex];
            else
                nextClipDepth = ~static_cast<DepthUnit>(0);
        }

        // Paint self
        if (handler.event) [[likely]]
            handler.event(painter, area);
    }
}

void UI::UISystem::processAreas(void) noexcept
{
    // kFInfo("UI::UISystem::processArea: Process begin");
    auto &nodeTable = getTable<TreeNode>();
    // Prepare context caches
    _traverseContext.setupContext(nodeTable.count(), nodeTable.begin(), getTable<Area>().begin(), getTable<Depth>().begin());

    // Traverse from each childrenless items to root
    for (const auto &node : nodeTable) {
        // If the node does not have children, we go upward from here
        if (node.children.empty()) [[unlikely]] {
            { // Setup the entity for traversal
                const auto entityIndex = _traverseContext.entityIndexOf(node);
                _traverseContext.setupEntity(
                    nodeTable.entities().at(entityIndex),
                    entityIndex
                );
            }
            // Traverse from bottom to top
            traverseConstraints();
        }
    }

    { // Setup root entity from top to bottom traversal
        const auto rootEntity = Item::GetEntity(*_root);
        const auto rootEntityIndex = _traverseContext.entityIndexOf(nodeTable.get(rootEntity));
        _traverseContext.setupEntity(rootEntity, rootEntityIndex);
        auto &area = _traverseContext.areaAt(rootEntityIndex);
        area.pos = Point {};
        area.size = _windowSize;
    }

    // Reset depth cache before traversal
    _maxDepth = DepthUnit {};

    // Traverse from top to bottom
    traverseAreas();

    // Sort component tables by depth
    sortTables();

    // kFInfo("UI::UISystem::processArea: Process end\n");
}

void UI::UISystem::sortTables(void) noexcept
{
    const auto ascentCompareFunc = [&depthTable = getTable<Depth>()](const ECS::Entity lhs, const ECS::Entity rhs) {
        return depthTable.get(lhs).depth < depthTable.get(rhs).depth;
    };
    const auto descentCompareFunc = [&depthTable = getTable<Depth>()](const ECS::Entity lhs, const ECS::Entity rhs) {
        return depthTable.get(lhs).depth > depthTable.get(rhs).depth;
    };

    getTable<PainterArea>().sort(ascentCompareFunc);
    getTable<MouseEventArea>().sort(descentCompareFunc);
    getTable<MotionEventArea>().sort(descentCompareFunc);
    getTable<KeyEventReceiver>().sort(descentCompareFunc);
}

void UI::UISystem::traverseConstraints(void) noexcept
{
    using namespace Internal;

    constexpr auto GetCounterInsertIndex = [](const ECS::Entity entity, const TreeNode &parentNode, const Internal::TraverseContext::Counter &counter) -> std::uint32_t {
        if (counter.empty()) [[likely]]
            return 0u;
        return std::min(
            static_cast<std::uint32_t>(std::distance(parentNode.children.begin(), parentNode.children.find(entity))),
            counter.size()
        );
    };

    // kFInfo("[traverseConstraints] Traversing entity ", _traverseContext.entity(), " of index ", _traverseContext.entityIndex());

    { // For each traversed node, we build constraints from children to parents
        const auto entity = _traverseContext.entity();
        TreeNode &node = _traverseContext.node();
        Constraints &constraints = _traverseContext.constraints();

        // If the node has explicit constraints use it, else we use default fill constraints
        if (!Core::HasFlags(node.componentFlags, ComponentFlags::Constraints)) [[likely]] {
            constraints = Constraints::Make(Fill(), Fill());
        } else [[unlikely]] {
            constraints = get<Constraints>(entity);
        }
        // kFInfo("[traverseConstraints] Entity constraints input: ", constraints);

        // If the node has variable constraints and at least one child, compute its size accordingly to its children
        if (!node.children.empty()) [[likely]] {
            // Update self constraints depending on children constraints
            if (!Core::HasFlags(node.componentFlags, ComponentFlags::Layout)) [[likely]]
                computeChildrenConstraints<Accumulate::No, Accumulate::No>(constraints, constraints.maxSize.width == PixelHug, constraints.maxSize.height == PixelHug);
            else [[unlikely]]
                buildLayoutConstraints(constraints);
        }

        // kFInfo("[traverseConstraints] Entity ", entity, " transformed constraints: ", constraints);

        // Check if node has parent => stop traversal
        if (!node.parent) [[unlikely]]
            return;

        // Setup parent node in traverse context
        const auto entityIndex = _traverseContext.entityIndex();
        const auto &parentNode = get<TreeNode>(node.parent);
        const auto parentEntityIndex = _traverseContext.entityIndexOf(parentNode);
        _traverseContext.setupEntity(node.parent, parentEntityIndex);

        // Insert entity index into counter list
        auto &counter = _traverseContext.counter();
        const auto insertIndex = GetCounterInsertIndex(entity, parentNode, counter);
        counter.insert(counter.begin() + insertIndex, entityIndex);

        // Check if parent still has unvisited child => stop traversal
        if (parentNode.children.size() != counter.size()) [[likely]]
            return;
    }

    // Process parent constraints
    traverseConstraints();
}

void UI::UISystem::buildLayoutConstraints(Constraints &constraints) noexcept
{
    using namespace Internal;

    const bool hugWidth = constraints.maxSize.width == PixelHug;
    const bool hugHeight = constraints.maxSize.height == PixelHug;

    if (!hugWidth && !hugHeight) [[likely]]
        return;

    const auto &layout = get<Layout>(_traverseContext.entity());
    switch (layout.flowType) {
    case FlowType::Stack:
        computeChildrenConstraints<Accumulate::No, Accumulate::No>(constraints, hugWidth, hugHeight);
        break;
    case FlowType::Column:
        computeChildrenConstraints<Accumulate::No, Accumulate::Yes>(constraints, hugWidth, hugHeight);
        if (const auto count = _traverseContext.node().children.size(); count)
            constraints.maxSize.height += layout.spacing * static_cast<Pixel>(count - 1);
        break;
    case FlowType::Row:
        computeChildrenConstraints<Accumulate::Yes, Accumulate::No>(constraints, hugWidth, hugHeight);
        if (const auto count = _traverseContext.node().children.size(); count)
            constraints.maxSize.width += layout.spacing * static_cast<Pixel>(count - 1);
        break;
    }

    const auto &padding = layout.padding;
    constraints.maxSize += Size(
        static_cast<Pixel>(hugWidth) * padding.left + padding.right,
        static_cast<Pixel>(hugHeight) * padding.top + padding.bottom
    );
}

template<kF::UI::Internal::Accumulate AccumulateX, kF::UI::Internal::Accumulate AccumulateY>
void kF::UI::UISystem::computeChildrenConstraints(Constraints &constraints, const bool hugWidth, const bool hugHeight) noexcept
{
    for (const auto childEntityIndex : _traverseContext.counter()) {
        const auto &rhs = _traverseContext.constraintsAt(childEntityIndex);
        if (hugWidth)
            ComputeAxisHugConstraint<AccumulateX>(constraints.maxSize.width, rhs.maxSize.width);
        if (hugHeight)
            ComputeAxisHugConstraint<AccumulateY>(constraints.maxSize.height, rhs.maxSize.height);
    }
}

template<kF::UI::Internal::Accumulate AccumulateValue>
void kF::UI::UISystem::ComputeAxisHugConstraint(Pixel &lhs, const Pixel rhs) noexcept
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

void UI::UISystem::traverseAreas(void) noexcept
{
    using namespace Internal;

    // Set self depth
    _traverseContext.depth().depth = _maxDepth++;

    // kFInfo("[traverseAreas] Traversing entity ", _traverseContext.entity(), " of index ", _traverseContext.entityIndex(), ", area ", _traverseContext.area(), " and depth ", _traverseContext.depth().depth);

    Area lastClip {};
    bool clip { false };
    const auto &node = _traverseContext.node();
    {
        // Build position of children using the context node area
        auto &area = _traverseContext.area();
        if (!Core::HasFlags(node.componentFlags, ComponentFlags::Layout)) [[likely]] {
            computeChildrenArea(area, Anchor::Center);
        } else [[unlikely]] {
            buildLayoutArea(area);
        }

        // Process clip
        const auto &clipTable = getTable<Clip>();
        const auto clipIndex = clipTable.getUnstableIndex(_traverseContext.entity());
        if (clipIndex != ECS::NullEntityIndex) [[unlikely]] {
            clip = true;
            lastClip = _traverseContext.currentClip();
            _traverseContext.setClip(
                Area::ApplyPadding(area, clipTable.atIndex(clipIndex).padding),
                _maxDepth
            );
        }
    }

    // Traverse each child
    std::uint32_t childIndex { 0u };
    for (const auto childEntityIndex : _traverseContext.counter()) {
        _traverseContext.setupEntity(node.children.at(childIndex++), childEntityIndex);
        traverseAreas();
    }

    // Set max child depth
    _traverseContext.depth().maxChildDepth = _maxDepth - 1;

    // Restore previous clip
    if (clip) [[unlikely]]
        _traverseContext.setClip(lastClip, _maxDepth);
}

void UI::UISystem::buildLayoutArea(const Area &contextArea) noexcept
{
    using namespace Internal;

    auto &layout = get<Layout>(_traverseContext.entity());
    const auto transformedArea = Area::ApplyPadding(contextArea, layout.padding);

    switch (layout.flowType) {
    case FlowType::Stack:
        computeChildrenArea(transformedArea, layout.anchor);
        break;
    case FlowType::Column:
        computeDistributedChildrenArea<Axis::Vertical>(transformedArea, layout);
        break;
    case FlowType::Row:
        computeDistributedChildrenArea<Axis::Horizontal>(transformedArea, layout);
        break;
    }
}

void UI::UISystem::computeChildrenArea(const Area &contextArea, const Anchor anchor) noexcept
{
    using namespace Internal;

    for (const auto childEntityIndex : _traverseContext.counter()) {
        const auto &constraints = _traverseContext.constraintsAt(childEntityIndex);
        auto &area = _traverseContext.areaAt(childEntityIndex);

        // Compute size
        area.size.width = ComputeSize<BoundType::Unknown>(
            contextArea.size.width,
            constraints.minSize.width,
            constraints.maxSize.width
        );
        area.size.height = ComputeSize<BoundType::Unknown>(
            contextArea.size.height,
            constraints.minSize.height,
            constraints.maxSize.height
        );

        // Compute pos
        area.pos = area.pos;
        ComputePosition(area, contextArea, anchor);

        // Apply children transform
        applyTransform(childEntityIndex, area);
    }
}

template<kF::UI::Internal::Axis DistributionAxis>
void kF::UI::UISystem::computeDistributedChildrenArea(const Area &contextArea, const Layout &layout) noexcept
{
    using namespace Internal;

    auto &counter = _traverseContext.counter();
    const auto childCount = static_cast<Pixel>(counter.size());
    const auto totalSpacing = layout.spacing * (childCount - 1.0f);
    Point flexCount {};
    Size freeSpace {};

    if constexpr (DistributionAxis == Axis::Horizontal)
        freeSpace.width = contextArea.size.width - totalSpacing;
    else
        freeSpace.height = contextArea.size.height - totalSpacing;

    for (const auto childEntityIndex : counter) {
        const auto &constraints = _traverseContext.constraintsAt(childEntityIndex);
        auto &area = _traverseContext.areaAt(childEntityIndex);

        // Compute width
        area.size.width = ComputeDistributedSize<DistributionAxis == Axis::Horizontal>(
            flexCount.x,
            freeSpace.width,
            contextArea.size.width,
            constraints.minSize.width,
            constraints.maxSize.width
        );

        // Compute height
        area.size.height = ComputeDistributedSize<DistributionAxis == Axis::Vertical>(
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
        const auto &constraints = _traverseContext.constraintsAt(childEntityIndex);
        auto &area = _traverseContext.areaAt(childEntityIndex);

        // Compute size of flex items
        if constexpr (DistributionAxis == Axis::Horizontal) {
            if (constraints.maxSize.width == PixelInfinity) [[likely]]
                area.size.width = flexSize.width;
        } else {
            if (constraints.maxSize.height == PixelInfinity) [[likely]]
                area.size.height = flexSize.height;
        }

        // Compute position
        ComputeDistributedPosition<DistributionAxis>(offset, area, contextArea, spacing, layout.anchor);

        // Apply children transform
        applyTransform(childEntityIndex, area);
    }
}

template<bool Distribute>
kF::UI::Pixel kF::UI::UISystem::ComputeDistributedSize([[maybe_unused]] Pixel &flexCount, [[maybe_unused]] Pixel &freeSpace,
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

template<kF::UI::Internal::BoundType Bound>
kF::UI::Pixel kF::UI::UISystem::ComputeSize(const Pixel parent, const Pixel min, const Pixel max) noexcept
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

void UI::UISystem::ComputePosition(Area &area, const Area &contextArea, const Anchor anchor) noexcept
{
    area.pos = contextArea.pos;
    switch (anchor) {
    case Anchor::Center:
        area.pos += (contextArea.size - area.size) / 2;
        break;
    case Anchor::Left:
        area.pos += Size(0, contextArea.size.height / 2 - area.size.height / 2);
        break;
    case Anchor::Right:
        area.pos += Size(contextArea.size.width - area.size.width, contextArea.size.height / 2 - area.size.height / 2);
        break;
    case Anchor::Top:
        area.pos += Size(contextArea.size.width / 2 - area.size.width / 2, 0);
        break;
    case Anchor::Bottom:
        area.pos += Size(contextArea.size.width / 2 - area.size.width / 2, contextArea.size.height - area.size.height);
        break;
    case Anchor::TopLeft:
        break;
    case Anchor::TopRight:
        area.pos += Size(contextArea.size.width - area.size.width, 0);
        break;
    case Anchor::BottomLeft:
        area.pos += Size(0, contextArea.size.height - area.size.height);
        break;
    case Anchor::BottomRight:
        area.pos += Size(contextArea.size.width - area.size.width, contextArea.size.height - area.size.height);
        break;
    }
}

template<kF::UI::Internal::Axis DistributionAxis>
void kF::UI::UISystem::ComputeDistributedPosition(Point &offset, Area &area, const Area &contextArea, const Pixel spacing, const Anchor anchor) noexcept
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

void kF::UI::UISystem::applyTransform(const ECS::EntityIndex entityIndex, Area &area) noexcept
{
    // Ensure entity has a transform component
    if (!Core::HasFlags(_traverseContext.nodeAt(entityIndex).componentFlags, ComponentFlags::Transform)) [[likely]]
        return;

    // Apply child scale transformation
    const auto entity = getTable<TreeNode>().entities().at(entityIndex);
    const auto &transform = get<Transform>(entity);

    const auto scaledSize = Size::Max(transform.minSize, Size {
        area.size.width * transform.scale.width,
        area.size.height * transform.scale.height
    });
    area.pos = Point {
        area.pos.x + area.size.width * transform.origin.x - scaledSize.width * transform.origin.x,
        area.pos.y + area.size.height * transform.origin.y - scaledSize.height * transform.origin.y
    };
    area.size = scaledSize;
}
