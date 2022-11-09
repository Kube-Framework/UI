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

void UI::UISystem::onDrag(const TypeHash typeHash, const void * const data, const Size &size, const DropTrigger dropTrigger, PainterArea &&painterArea) noexcept
{
    _eventCache.drop.typeHash = typeHash;
    _eventCache.drop.data = data;
    _eventCache.drop.size = size;
    _eventCache.drop.dropTrigger = dropTrigger;
    _eventCache.drop.painterArea = std::move(painterArea);

    // Trigger begin event of every DropEventArea matching typeHash
    int x, y;
    SDL_GetMouseState(&x, &y);
    processDropEventAreas(DropEvent {
        .type = DropEvent::Type::Begin,
        .pos = Point(static_cast<Pixel>(x), static_cast<Pixel>(y)),
        .timestamp = SDL_GetTicks()
    });
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
    // @todo Mouse / wheel events processed out of order
    _eventCache.mouseQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processMouseEventAreas(event);
    });
    _eventCache.wheelQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processWheelEventAreas(event);
    });
    _eventCache.keyQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processKeyEventReceivers(event);
    });

    if (isDragging() && _eventCache.lastMouseHovered != ECS::NullEntity) {
        auto &lastComponent = get<MouseEventArea>(_eventCache.lastMouseHovered);
        const auto &lastClippedArea = getClippedArea(_eventCache.lastMouseHovered, get<Area>(_eventCache.lastMouseHovered));
        int x, y;
        const auto activeButtons = static_cast<Button>(SDL_GetMouseState(&x, &y));
        const MouseEvent leaveEvent {
            .pos = UI::Point(static_cast<UI::Pixel>(x), static_cast<UI::Pixel>(y)),
            .type = MouseEvent::Type::Leave,
            .activeButtons = activeButtons,
            .modifiers = static_cast<Modifier>(SDL_GetModState()),
            .timestamp = SDL_GetTicks()
        };
        lastComponent.hovered = false;
        const auto flags = lastComponent.event(leaveEvent, lastClippedArea);
        _eventCache.lastMouseHovered = ECS::NullEntity;
        kFEnsure(!Core::HasFlags(flags, EventFlags::Lock),
            "UI::UISystem::processEventHandlers: Cannot lock mouse while dragging");
    }
}

void UI::UISystem::processMouseEventAreas(const MouseEvent &event) noexcept
{
    if (event.type == MouseEvent::Type::Motion)
        return processMouseEventAreasMotion(event);
    else
        return processMouseEventAreasAction(event);
}

void UI::UISystem::processMouseEventAreasAction(const MouseEvent &event) noexcept
{
    // Handle drop trigger
    if (isDragging() && _eventCache.drop.dropTrigger.button == event.button
            && ((_eventCache.drop.dropTrigger.buttonState && event.type == MouseEvent::Type::Press)
            || (!_eventCache.drop.dropTrigger.buttonState && event.type == MouseEvent::Type::Release))) {
        // Send drop event
        processDropEventAreas(DropEvent {
            .type = DropEvent::Type::Drop,
            .pos = event.pos,
            .timestamp = event.timestamp
        });
        // Send end drop event
        processDropEventAreas(DropEvent {
            .type = DropEvent::Type::End,
            .pos = event.pos,
            .timestamp = event.timestamp
        });
        // Reset drop cache
        _eventCache.drop = {};
        _eventCache.lastDropHovered = ECS::NullEntity;
        _eventCache.mouseLock = ECS::NullEntity;
        invalidate();
        return;
    }

    traverseClippedEventTable<MouseEventArea>(
        event,
        _eventCache.mouseLock,
        [this](const MouseEvent &event, MouseEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
            const auto flags = component.event(event, clippedArea);
            component.hovered = !Core::HasFlags(flags, EventFlags::Propagate) && clippedArea.contains(event.pos);
            // If mouse is not inside and not locked, then trigger leave event
            if (!component.hovered && !Core::HasFlags(flags, EventFlags::Lock)) {
                MouseEvent mouseEvent(event);
                mouseEvent.type = UI::MouseEvent::Type::Leave;
                const auto leaveFlags = component.event(mouseEvent, clippedArea);
                if (processEventFlags(_eventCache.mouseLock, leaveFlags, entity)) {
                    _eventCache.lastMouseHovered = _eventCache.mouseLock;
                    // If leave event did lock, stop
                    if (Core::HasFlags(leaveFlags, EventFlags::Lock)) {
                        if (Core::HasFlags(flags, EventFlags::Invalidate))
                            invalidate();
                        return EventFlags::Stop;
                    }
                }
            }
            return flags;
        }
    );

    kFEnsure(!(isDragging() && _eventCache.mouseLock != ECS::NullEntity),
        "UI::UISystem::processMouseEventAreasAction: Cannot lock mouse while dragging");
}

void UI::UISystem::processMouseEventAreasMotion(const MouseEvent &event) noexcept
{
    if (isDragging()) {
        processDropEventAreas(DropEvent {
            .type = DropEvent::Type::Motion,
            .pos = event.pos,
            .timestamp = event.timestamp
        });
        invalidate();
        return;
    }

    traverseClippedEventTableWithHover<MouseEventArea>(
        event,
        _eventCache.mouseLock,
        _eventCache.lastMouseHovered,
        // On enter
        [this](const MouseEvent &event, MouseEventArea &component, const Area &clippedArea) noexcept {
            auto mouseEvent { event };
            mouseEvent.type = MouseEvent::Type::Enter;
            const auto flags = component.event(mouseEvent, clippedArea);
            component.hovered = !Core::HasFlags(flags, EventFlags::Propagate);
            return flags;
        },
        // On leave
        [this](const MouseEvent &event, MouseEventArea &component, const Area &clippedArea) noexcept {
            auto mouseEvent { event };
            mouseEvent.type = MouseEvent::Type::Leave;
            component.hovered = false;
            return component.event(mouseEvent, clippedArea);
        },
        // On inside
        [this](const MouseEvent &event, MouseEventArea &component, const Area &clippedArea) noexcept {
            return component.event(event, clippedArea);
        }
    );

    kFEnsure(!(isDragging() && _eventCache.mouseLock != ECS::NullEntity),
        "UI::UISystem::processMouseEventAreasMotion: Cannot lock mouse when dragging");
}

void UI::UISystem::processWheelEventAreas(const WheelEvent &event) noexcept
{
    traverseClippedEventTable<WheelEventArea>(
        event,
        _eventCache.wheelLock,
        [](const WheelEvent &event, const WheelEventArea &component, const Area &clippedArea, const ECS::Entity) {
            return component.event(event, clippedArea);
        }
    );
}

void UI::UISystem::processDropEventAreas(const DropEvent &event) noexcept
{
    switch (event.type) {
    case DropEvent::Type::Begin:
    case DropEvent::Type::End:
    {
        const auto &areaTable = getTable<Area>();
        getTable<DropEventArea>().traverse([this, &event, &areaTable](const ECS::Entity entity, DropEventArea &component) {
            component.event(
                _eventCache.drop.typeHash,
                _eventCache.drop.data,
                event,
                getClippedArea(entity, areaTable.get(entity))
            );
        });
        break;
    }
    case DropEvent::Type::Motion:
    case DropEvent::Type::Enter:
    case DropEvent::Type::Leave:
        traverseClippedEventTableWithHover<DropEventArea>(
            event,
            _eventCache.mouseLock,
            _eventCache.lastDropHovered,
            // On enter
            [this](const DropEvent &event, DropEventArea &component, const Area &clippedArea) noexcept {
                auto dropEvent { event };
                dropEvent.type = DropEvent::Type::Enter;
                const auto flags = component.event(_eventCache.drop.typeHash, _eventCache.drop.data, dropEvent, clippedArea);
                component.hovered = !Core::HasFlags(flags, EventFlags::Propagate);
                return flags;
            },
            // On leave
            [this](const DropEvent &event, DropEventArea &component, const Area &clippedArea) noexcept {
                auto dropEvent { event };
                dropEvent.type = DropEvent::Type::Leave;
                component.hovered = false;
                return component.event(_eventCache.drop.typeHash, _eventCache.drop.data, dropEvent, clippedArea);
            },
            // On inside
            [this](const DropEvent &event, DropEventArea &component, const Area &clippedArea) noexcept {
                return component.event(_eventCache.drop.typeHash, _eventCache.drop.data, event, clippedArea);
            }
        );
        break;
    case DropEvent::Type::Drop:
        traverseClippedEventTable<DropEventArea>(
            event,
            _eventCache.mouseLock,
            [this](const DropEvent &event, DropEventArea &component, const Area &clippedArea, const ECS::Entity) noexcept {
                return component.event(_eventCache.drop.typeHash, _eventCache.drop.data, event, clippedArea);
            }
        );
        // Reset locks that could be affected by drag events
        _eventCache.mouseLock = ECS::NullEntity;
        break;
    }
}

void UI::UISystem::processKeyEventReceivers(const KeyEvent &event) noexcept
{
    // @todo prevent crash when deleting KeyEventReceiver inside event
    getTable<KeyEventReceiver>().traverse([this, &event](const ECS::Entity entity, KeyEventReceiver &component) noexcept {
        const auto flags = component.event(event);
        if (processEventFlags(_eventCache.keyLock, flags, entity))
            return false;
        return true;
    });
}

template<typename Component, typename Event, typename OnEvent, typename OtherComp>
inline ECS::Entity UI::UISystem::traverseClippedEventTable(const Event &event, ECS::Entity &entityLock, OnEvent &&onEvent) noexcept
{
    auto &table = getTable<Component>();
    auto &areaTable = getTable<OtherComp>();

    // Send event to locked entity if any
    if (entityLock != ECS::NullEntity && table.exists(entityLock)) {
        // Compute clipped
        const auto clippedArea = getClippedArea(entityLock, areaTable.get(entityLock));

        // Process locked event without checking for mouse collision
        auto &component = table.get(entityLock);
        const auto flags = onEvent(event, component, clippedArea, entityLock);

        // If locked event flags tells to stop, return now
        if (processEventFlags(entityLock, flags, entityLock))
            return entityLock;
    }

    ECS::Entity hitEntity { ECS::NullEntity };
    table.traverse([&](const ECS::Entity entity) {
        const auto area = areaTable.get(entity);
        // Test non-clipped area
        if (!area.contains(event.pos)) [[likely]]
            return true;

        // Test clipped area
        const auto clippedArea = getClippedArea(entity, area);
        if (!clippedArea.contains(event.pos)) [[likely]]
            return true;

        // Process event
        auto &component = table.get(entity);
        const auto flags = onEvent(event, component, clippedArea, entity);

        // Process event flags
        if (processEventFlags(entityLock, flags, entity)) {
            hitEntity = entity;
            return false;
        }
        return true;
    });
    return hitEntity;
}

template<typename Component, typename Event, typename OnEnter, typename OnLeave, typename OnInside>
inline ECS::Entity UI::UISystem::traverseClippedEventTableWithHover(
        const Event &event, ECS::Entity &entityLock, ECS::Entity &lastHovered, OnEnter &&onEnter, OnLeave &&onLeave, OnInside &&onInside) noexcept
{
    const auto hitEntity = traverseClippedEventTable<Component>(
        event,
        entityLock,
        [this, &entityLock, &lastHovered, &onEnter, &onLeave, &onInside](
            const Event &event, Component &component, const Area &clippedArea, const ECS::Entity hitEntity
        ) noexcept {
            EventFlags flags;
            // If last entity is different from current, we have to notify it
            if (hitEntity != lastHovered) [[unlikely]] {
                // If last hovered entity is not null, call leave event
                if (lastHovered != ECS::NullEntity) {
                    auto &lastComponent = get<Component>(lastHovered);
                    const auto &lastClippedArea = getClippedArea(lastHovered, get<Area>(lastHovered));
                    flags = onLeave(event, lastComponent, lastClippedArea);
                    // If leaved entity asked for stop, we don't propagate
                    if (processEventFlags(entityLock, flags, lastHovered)) {
                        lastHovered = entityLock;
                        // Only stop when leaving event produced a lock
                        if (Core::HasFlags(flags, EventFlags::Lock))
                            return EventFlags::Stop;
                    }
                }
                flags = onEnter(event, component, clippedArea);
            } else {
                flags = onInside(event, component, clippedArea);
            }
            return flags;
        }
    );

    // Store and update last hovered
    const auto last = lastHovered;
    lastHovered = hitEntity;

    // If no target was found and another target was hovered, we have to leave it
    if (hitEntity == ECS::NullEntity && last != ECS::NullEntity) [[unlikely]] {
        auto &lastComponent = get<Component>(last);
        const auto &lastClippedArea = getClippedArea(last, get<Area>(last));
        const auto flags = onLeave(event, lastComponent, lastClippedArea);
        if (processEventFlags(entityLock, flags, lastHovered))
            lastHovered = entityLock;
    }
    return hitEntity;
}

inline bool UI::UISystem::processEventFlags(ECS::Entity &lock, const EventFlags flags, const ECS::Entity hitEntity) noexcept
{
    // Invalidate frame flag
    if (Core::HasFlags(flags, EventFlags::Invalidate))
        invalidate();

    // Lock flag
    if (Core::HasFlags(flags, EventFlags::Lock))
        lock = hitEntity;
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

    getTable<Timer>().traverse([elapsed, &invalidateState](Timer &handler) {
        handler.elapsedTimeState += elapsed;
        if (handler.elapsedTimeState >= handler.interval) [[unlikely]] {
            invalidateState |= handler.event(elapsed);
            handler.elapsedTimeState = 0;
        }
    });
    return invalidateState;
}

bool UI::UISystem::processAnimators(const std::int64_t elapsed) noexcept
{
    // @todo Fix bug when removing an animator at tick time
    bool invalidateState { false };

    getTable<Animator>().traverse([elapsed, &invalidateState](Animator &handler) {
        invalidateState |= handler.tick(elapsed);
    });
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
    for (ECS::EntityIndex index {}; const PainterArea &handler : paintTable) {
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

    // Draw drag if any
    if (isDragging()) [[unlikely]] {
        int x, y;
        SDL_GetMouseState(&x, &y);
        const auto mousePos = Point(static_cast<Pixel>(x), static_cast<Pixel>(y));
        const Area area(mousePos - _eventCache.drop.size / 2, _eventCache.drop.size);
        _eventCache.drop.painterArea.event(painter, area);
    }
}
