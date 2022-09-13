/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System
 */

#include <SDL2/SDL.h>

#include <Kube/GPU/GPU.hpp>

#include <Kube/ECS/Executor.hpp>

#include "EventSystem.hpp"
#include "UISystem.hpp"
#include "LayoutBuilder.hpp"

using namespace kF;

UI::Size UI::UISystem::GetWindowSize(void) noexcept
{
    const auto extent = GPU::GPUObject::Parent().swapchain().extent();
    kFEnsure(extent.width && extent.height,
        "UI::UISystem::GetWindowSize: Couldn't retreive display DPI");
    return Size(static_cast<Pixel>(extent.width), static_cast<Pixel>(extent.height));
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
    // Release tree before managers
    _cache.root.release();
}

UI::UISystem::UISystem(void) noexcept
    :   _cache(Cache {
            .windowSize = GetWindowSize(),
            .windowDPI = GetWindowDPI()
        }),
        _eventCache(EventCache {
            .mouseQueue = parent().getSystem<EventSystem>().addEventQueue<MouseEvent>(),
            .motionQueue = parent().getSystem<EventSystem>().addEventQueue<MotionEvent>(),
            .wheelQueue = parent().getSystem<EventSystem>().addEventQueue<WheelEvent>(),
            .keyQueue = parent().getSystem<EventSystem>().addEventQueue<KeyEvent>()
        }),
        _renderer(*this)
{
    // Observe view size
    GPU::GPUObject::Parent().viewSizeDispatcher().add([this] {
        _cache.windowSize = GetWindowSize();
        _cache.windowDPI = GetWindowDPI();
        invalidate();
    });

    // Build task graph
    auto &graph = taskGraph();
    auto &computeTask = graph.add([this] {
        _spriteManager.prepareFrameCache();
        _renderer.batchPrimitives();
    });
    auto &transferTask = graph.add<&Renderer::transferPrimitives>(&_renderer);
    auto &dispatchTask = graph.add<&Renderer::dispatchInvalidFrame>(&_renderer);
    dispatchTask.after(computeTask);
    dispatchTask.after(transferTask);
}

bool UI::UISystem::tick(void) noexcept
{
    const auto currentFrame = _renderer.currentFrame();

    // Return if there are no items in the tree
    if (!_cache.root) [[unlikely]]
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
    if (_cache.invalidateTree) {
        // Build layouts using LayoutBuilder
        _cache.maxDepth = Internal::LayoutBuilder(*this, _traverseContext).build();

        // Sort component tables by depth
        sortTables();

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

void UI::UISystem::sortTables(void) noexcept
{
    const auto &depthTable = getTable<Depth>();
    const auto ascentCompareFunc = [&depthTable](const ECS::Entity lhs, const ECS::Entity rhs) {
        return depthTable.get(lhs).depth < depthTable.get(rhs).depth;
    };
    const auto descentCompareFunc = [&depthTable](const ECS::Entity lhs, const ECS::Entity rhs) {
        return depthTable.get(lhs).depth > depthTable.get(rhs).depth;
    };

    getTable<PainterArea>().sort(ascentCompareFunc);
    getTable<MouseEventArea>().sort(descentCompareFunc);
    getTable<MotionEventArea>().sort(descentCompareFunc);
    getTable<WheelEventArea>().sort(descentCompareFunc);
    getTable<DropEventArea>().sort(descentCompareFunc);
    getTable<KeyEventReceiver>().sort(descentCompareFunc);
}

UI::Area UI::UISystem::getClippedArea(const ECS::Entity entity, const UI::Area &area) noexcept
{
    const auto clipDepths = _traverseContext.clipDepths();
    if (clipDepths.empty())
        return area;

    // Find depth index
    const auto count = clipDepths.size<std::uint32_t>();
    const auto depth = get<UI::Depth>(entity).depth;
    auto index = count - 1u;
    while (true) {
        if (clipDepths.at(index) <= depth)
            break;
        if (index)
            --index;
        else {
            index = count;
            break;
        }
    }

    // If no clip is in range or target clip is default one, return the area
    const auto &clipAreas = _traverseContext.clipAreas();
    if (index == count || clipAreas.at(index) == DefaultClip)
        return area;

    // Find highest parent of current clip
    auto idx = index;
    while (idx) {
        if (clipAreas.at(idx - 1) == DefaultClip)
            break;
        --idx;
    }

    // Compute clip recursively
    Area recursiveClip = clipAreas.at(idx);
    while (idx != index) {
        recursiveClip = Area::ApplyClip(clipAreas.at(++idx), recursiveClip);
    }

    // Apply recursive clip to area
    return Area::ApplyClip(area, recursiveClip);
}

void UI::UISystem::processEventHandlers(void) noexcept
{
    // @todo Events can crash if they remove Items from the tree
    _eventCache.mouseQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processMouseEventAreas(event);
    });
    _eventCache.motionQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processMotionEventAreas(event);
    });
    _eventCache.wheelQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processWheelEventAreas(event);
    });
    _eventCache.keyQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processKeyEventReceivers(event);
    });
}

void UI::UISystem::processMouseEventAreas(const MouseEvent &event) noexcept
{
    traverseClippedEventTable<MouseEventArea>(event, _eventCache.mouseLock);
}

void UI::UISystem::processMotionEventAreas(const MotionEvent &event) noexcept
{
    const auto hitEntity = traverseClippedEventTable<MotionEventArea>(event, _eventCache.motionLock);

    if (hitEntity == _eventCache.lastHovered) [[likely]]
        return;

    const auto onHoverChanged = [this](auto &component, const bool value) noexcept {
        component.hovered = value;
        if (component.hoverEvent)
            component.hoverEvent(value);
        // We may invalidate on hover transitions
        if (component.invalidateOnHoverChanged) [[likely]]
            invalidate();
    };
    auto &table = getTable<MotionEventArea>();

    // Reset last hovered state
    if (table.exists(_eventCache.lastHovered)) [[likely]]
        onHoverChanged(table.get(_eventCache.lastHovered), false);

    // Set new hovered state
    if (hitEntity != ECS::NullEntity)
        onHoverChanged(table.get(hitEntity), true);

    _eventCache.lastHovered = hitEntity;
}

void UI::UISystem::processWheelEventAreas(const WheelEvent &event) noexcept
{
    traverseClippedEventTable<WheelEventArea>(event, _eventCache.wheelLock);
}

void UI::UISystem::processKeyEventReceivers(const KeyEvent &event) noexcept
{
    auto &keyTable = getTable<KeyEventReceiver>();

    for (const auto &handler : keyTable) {
        const auto flags = handler.event(event);
        if (processEventFlags(keyTable, handler, _eventCache.keyLock, flags))
            break;
    }
}

template<typename Component, typename Event, typename OtherComp>
inline ECS::Entity UI::UISystem::traverseClippedEventTable(const Event &event, ECS::Entity &entityLock) noexcept
{
    auto &table = getTable<Component>();
    auto &areaTable = getTable<OtherComp>();

    // Send event to locked entity if any
    if (entityLock != ECS::NullEntity && table.exists(entityLock)) {
        // Compute clipped
        const auto clippedArea = getClippedArea(entityLock, areaTable.get(entityLock));

        // Process locked event without checking for mouse collision
        const auto &component = table.get(entityLock);
        const auto flags = component.event(event, clippedArea);

        // If locked event flags tells to stop, return now
        if (processEventFlags(table, component, entityLock, flags))
            return entityLock;
    }

    std::uint32_t index { ~0u };
    ECS::Entity hitEntity { ECS::NullEntity };

    for (const auto entity : table.entities()) {
        const auto area = areaTable.get(entity);
        ++index;

        // Test non-clipped area
        if (!area.contains(event.pos)) [[likely]]
            continue;

        // Test clipped area
        const auto clippedArea = getClippedArea(entity, area);
        if (!clippedArea.contains(event.pos)) [[likely]]
            continue;

        // Process event
        const auto &component = table.atIndex(index);
        const auto flags = component.event ? component.event(event, clippedArea) : EventFlags::Stop;

        // Process event flags
        if (processEventFlags(table, component, entityLock, flags)) {
            hitEntity = entity;
            break;
        }
    }
    return hitEntity;
}

template<typename Table>
inline bool UI::UISystem::processEventFlags(
        const Table &table, const typename Table::ValueType &value, ECS::Entity &lock, const EventFlags flags) noexcept
{
    // Invalidate frame flag
    if (Core::HasFlags(flags, EventFlags::Invalidate))
        invalidate();

    // Lock flag
    if (Core::HasFlags(flags, EventFlags::Lock))
        lock = table.entities().at(static_cast<ECS::EntityIndex>(std::distance(table.begin(), &value)));
    else
        lock = ECS::NullEntity;

    // Return true on stop
    return !Core::HasFlags(flags, EventFlags::Propagate);
}

void UI::UISystem::processElapsedTime(void) noexcept
{
    // Query time
    const auto oldTick = _eventCache.lastTick;
    _eventCache.lastTick = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    // Compute elapsed time
    const auto elapsed = _eventCache.lastTick - oldTick;
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
        handler.elapsedTimeState += elapsed;
        if (handler.elapsedTimeState >= handler.interval) [[unlikely]] {
            invalidateState |= handler.event(elapsed);
            handler.elapsedTimeState = 0;
        }
    }
    return invalidateState;
}

bool UI::UISystem::processAnimators(const std::int64_t elapsed) noexcept
{
    // @todo Fix bug when removing an animator at tick time
    bool invalidateState { false };

    for (Animator &handler : getTable<Animator>())
        invalidateState |= handler.tick(elapsed);
    return invalidateState;
}

void UI::UISystem::processPainterAreas(void) noexcept
{
    constexpr auto MaxDepth = ~static_cast<DepthUnit>(0);

    auto &painter = _renderer.painter();
    const auto &paintTable = getTable<PainterArea>();
    const auto &areaTable = getTable<Area>();
    const auto &depthTable = getTable<Depth>();
    const auto clipAreas = _traverseContext.clipAreas();
    const auto clipDepths = _traverseContext.clipDepths();
    std::uint32_t clipIndex {};
    const std::uint32_t clipCount { clipDepths.size<std::uint32_t>() };
    auto nextClipDepth = clipCount ? clipDepths[0] : MaxDepth;

    painter.clear();
    for (std::uint32_t index = 0u; const PainterArea &handler : paintTable) {
        // Skip invisible item
        if (!handler.event) [[unlikely]]
            continue;

        // Query Area
        const auto entity = paintTable.entities().at(index++);
        const auto entityIndex = areaTable.getUnstableIndex(entity);
        const Area &area = areaTable.atIndex(entityIndex);

        // Process clip
        if (nextClipDepth != MaxDepth) [[likely]] {
            const auto depth = depthTable.atIndex(entityIndex).depth;
            while (depth >= nextClipDepth) [[unlikely]] {
                painter.setClip(clipAreas[clipIndex]);
                if (++clipIndex != clipCount) [[likely]] {
                    nextClipDepth = clipDepths[clipIndex];
                } else {
                    nextClipDepth = MaxDepth;
                }
            }
        }

        // Paint self
        handler.event(painter, area);
    }
}