/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System
 */

#include <Kube/GPU/GPU.hpp>
#include <Kube/UI/App.hpp>

#include "EventSystem.hpp"
#include "UISystem.hpp"

using namespace kF;

UI::UISystem::~UISystem(void) noexcept
{
    _root.release();
}

UI::UISystem::UISystem(void) noexcept
    :   _windowSize([] {
            const auto extent = GPU::GPUObject::Parent().swapchain().extent();
            return Size(extent.width, extent.height);
        }()),
        _renderer(*this),
        _mouseQueue(parent().getSystem<EventSystem>().addEventQueue<MouseEvent>()),
        _motionQueue(parent().getSystem<EventSystem>().addEventQueue<MotionEvent>()),
        _keyQueue(parent().getSystem<EventSystem>().addEventQueue<KeyEvent>())
{
    GPU::GPUObject::Parent().viewSizeDispatcher().add([this] {
        const auto extent = GPU::GPUObject::Parent().swapchain().extent();
        _windowSize = Size(static_cast<float>(extent.width), static_cast<float>(extent.height));
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

    // Process UI events
    processEventHandlers();

    // Process all update handlers
    processTimers();

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

    // for (std::uint32_t index = mouseTable.count(); const auto &handler : Core::Utils::IteratorRange { mouseTable.rbegin(), mouseTable.rend() }) {
    //     auto &area = areaTable.get(mouseTable.entities().at(--index));
    for (std::uint32_t index {}; const auto &handler : mouseTable) {
        auto &area = areaTable.get(mouseTable.entities().at(index++));
        if (area.contains(event.pos)) [[unlikely]] {
            // kFInfo("[processMouseEventAreas] Area ", area, " hit by ", event.pos);
            const auto flags = handler.event(event, area);
            if (Core::Utils::HasFlags(flags, EventFlags::Invalidate)) [[likely]]
                invalidate();
            if (!Core::Utils::HasFlags(flags, EventFlags::Propagate)) [[likely]]
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
            if (Core::Utils::HasFlags(flags, EventFlags::Invalidate)) [[likely]]
                invalidate();
            if (!Core::Utils::HasFlags(flags, EventFlags::Propagate)) [[likely]]
                break;
        }
    }
}

void kF::UI::UISystem::processKeyEventReceivers(const KeyEvent &event) noexcept
{
    auto &keyTable = getTable<KeyEventReceiver>();

    for (const auto &handler : keyTable) {
        const auto flags = handler.event(event);
        if (Core::Utils::HasFlags(flags, EventFlags::Invalidate)) [[likely]]
            invalidate();
        if (!Core::Utils::HasFlags(flags, EventFlags::Propagate)) [[likely]]
            break;
    }
}

void UI::UISystem::processTimers(void) noexcept
{
    const auto oldTick = _lastTick;
    _lastTick = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    if (oldTick) [[likely]] {
        const auto elapsed = _lastTick - oldTick;
        bool invalidateState = false;
        for (Timer &handler : getTable<Timer>()) {
            handler.elapsed += elapsed;
            if (handler.elapsed >= handler.interval) [[unlikely]] {
                invalidateState |= handler.event(elapsed);
                handler.elapsed = 0;
            }
        }
        if (invalidateState) [[likely]]
            invalidate();
    }
}

void UI::UISystem::processPainterAreas(void) noexcept
{
    const auto &table = getTable<PainterArea>();
    auto &painter = _renderer.painter();

    painter.clear();
    for (std::uint32_t index = 0u; const PainterArea &handler : table) {
        const Area &area = get<Area>(table.entities().at(index++));
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
    _maxDepth = Depth {};

    // Traverse from top to bottom
    traverseAreas();

    // Sort component tables by depth
    sortTables();

    // kFInfo("UI::UISystem::processArea: Process end\n");
}

void UI::UISystem::sortTables(void) noexcept
{
    const auto ascentCompareFunc = [&depthTable = getTable<Depth>()](const ECS::Entity lhs, const ECS::Entity rhs) {
        return depthTable.get(lhs) < depthTable.get(rhs);
    };
    const auto descentCompareFunc = [&depthTable = getTable<Depth>()](const ECS::Entity lhs, const ECS::Entity rhs) {
        return depthTable.get(lhs) > depthTable.get(rhs);
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
        if (!Core::Utils::HasFlags(node.componentFlags, ComponentFlags::Constraints)) [[likely]] {
            constraints = Constraints::Make(Fill(), Fill());
        } else [[unlikely]] {
            constraints = get<Constraints>(entity);
        }
        // kFInfo("[traverseConstraints] Entity constraints input: ", constraints);

        // If the node has variable constraints and at least one child, compute its size accordingly to its children
        if (!node.children.empty()) [[likely]] {
            // Update self constraints depending on children constraints
            if (!Core::Utils::HasFlags(node.componentFlags, ComponentFlags::Layout)) [[likely]]
                computeChildrenConstraints<Accumulate::No, Accumulate::No>(constraints);
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

    auto &layout = get<Layout>(_traverseContext.entity());

    switch (layout.flowType) {
    case FlowType::Stack:
        computeChildrenConstraints<Accumulate::No, Accumulate::No>(constraints);
        break;
    case FlowType::Column:
        computeChildrenConstraints<Accumulate::No, Accumulate::Yes>(constraints);
        break;
    case FlowType::Row:
        computeChildrenConstraints<Accumulate::Yes, Accumulate::No>(constraints);
        break;
    }
}

void UI::UISystem::traverseAreas(void) noexcept
{
    using namespace Internal;

    // Set depth
    _traverseContext.depth() = ++_maxDepth;

    // kFInfo("[traverseAreas] Traversing entity ", _traverseContext.entity(), " of index ", _traverseContext.entityIndex(), ", area ", _traverseContext.area(), " and depth ", _traverseContext.depth());

    const auto &node = _traverseContext.node();
    {
        // Build position of children using the context node area
        const auto &contextArea = _traverseContext.area();
        if (!Core::Utils::HasFlags(node.componentFlags, ComponentFlags::Layout)) [[likely]] {
            computeChildrenArea(contextArea, Anchor::Center);
        } else [[unlikely]] {
            buildLayoutArea(contextArea);
        }
    }

    // Traverse each child
    std::uint32_t childIndex { 0u };
    for (const auto childEntityIndex : _traverseContext.counter()) {
        _traverseContext.setupEntity(node.children.at(childIndex++), childEntityIndex);
        traverseAreas();
    }
}

void UI::UISystem::buildLayoutArea(const Area &contextArea) noexcept
{
    using namespace Internal;

    auto &layout = get<Layout>(_traverseContext.entity());
    const Area transformedArea {
        .pos = contextArea.pos + Point {
            layout.padding.left,
            layout.padding.top
        },
        .size = contextArea.size - Size {
            layout.padding.left + layout.padding.right,
            layout.padding.top + layout.padding.bottom
        }
    };

    switch (layout.flowType) {
    case FlowType::Stack:
        computeChildrenArea(transformedArea, layout.anchor);
        break;
    case FlowType::Column:
        computeDistributedChildrenArea<Axis::Vertical>(transformedArea, layout.spacing, layout.anchor);
        break;
    case FlowType::Row:
        computeDistributedChildrenArea<Axis::Horizontal>(transformedArea, layout.spacing, layout.anchor);
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
    }
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