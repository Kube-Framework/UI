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

#include "ArcProcessor.hpp"
#include "CubicBezierProcessor.hpp"
#include "CurveProcessor.hpp"
#include "GradientRectangleProcessor.hpp"
#include "RectangleProcessor.hpp"
#include "TextProcessor.hpp"

using namespace kF;

UI::Size UI::UISystem::GetWindowSize(void) noexcept
{
    const auto extent = GPU::GPUObject::Parent().swapchain().extent();
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

    // Release system cursors
    for (const auto backendCursor : _cursorCache.cursors)
        ::SDL_FreeCursor(backendCursor);
}

UI::UISystem::UISystem(GPU::BackendWindow * const window) noexcept
    : _cache(Cache {
        .windowSize = GetWindowSize(),
        .windowDPI = GetWindowDPI(),
        .window = window
    })
    , _eventCache(EventCache {
        .mouseQueue = parent().getSystem<EventSystem>().addEventQueue<MouseEvent>(),
        .wheelQueue = parent().getSystem<EventSystem>().addEventQueue<WheelEvent>(),
        .keyQueue = parent().getSystem<EventSystem>().addEventQueue<KeyEvent>(),
        .textQueue = parent().getSystem<EventSystem>().addEventQueue<TextEvent>()
    })
    , _renderer(*this)
{
    // Observe view size
    GPU::GPUObject::Parent().viewSizeDispatcher().add([this] {
        _cache.windowSize = GetWindowSize();
        _cache.windowDPI = GetWindowDPI();
        invalidate();
    });

    // Build task graph
    auto &graph = taskGraph();
    auto &prepareSpriteManagerTask = graph.add<&SpriteManager::prepareFrameCache>(&_spriteManager);
    auto &batchPrimitivesTask = graph.add<&Renderer::batchPrimitives>(&_renderer);
    auto &transferPrimitivesTask = graph.add<&Renderer::transferPrimitives>(&_renderer);
    auto &dispatchTask = graph.add<&Renderer::dispatchInvalidFrame>(&_renderer);
    graph.add<&UISystem::dispatchDelayedEvents>(this);
    batchPrimitivesTask.after(prepareSpriteManagerTask);
    dispatchTask.after(batchPrimitivesTask);
    dispatchTask.after(transferPrimitivesTask);

    // Relative mouse mode SDL2 bug
    SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);

    // Instantiate system cursors
    _cursorCache.cursors.resize(SystemCursorCount);
    for (auto i = 0u; i != SystemCursorCount; ++i)
        _cursorCache.cursors.at(i) = ::SDL_CreateSystemCursor(static_cast<SDL_SystemCursor>(i));
    { // Invisible cursor
        auto surface = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_RGBA8888);
        _cursorCache.cursors.push(SDL_CreateColorCursor(surface, 0, 0));
    }

    // Register primitives
    registerPrimitive<Rectangle>();
    registerPrimitive<Text>();
    registerPrimitive<GradientRectangle>();
    registerPrimitive<Curve>();
    registerPrimitive<CubicBezier>();
    registerPrimitive<Arc>();
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

    // Do not process item tree if the window size is zero
    if (!_cache.windowSize.width || !_cache.windowSize.height)
        return false;

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

void UI::UISystem::onDrag(const TypeHash typeHash, const Size &size, const DropTrigger dropTrigger, DropCache::DataFunctor &&data, PainterArea &&painterArea) noexcept
{
    _eventCache.drop.typeHash = typeHash;
    _eventCache.drop.size = size;
    _eventCache.drop.dropTrigger = dropTrigger;
    _eventCache.drop.data = std::move(data);
    _eventCache.drop.painterArea = std::move(painterArea);

    // Trigger begin event of every DropEventArea matching type hash
    processDropEventAreas(DropEvent {
        .type = DropEvent::Type::Begin,
        .pos = mousePosition(),
        .timestamp = SDL_GetTicks()
    });
}

void UI::UISystem::cancelDrag(void) noexcept
{
    // Trigger end event of every DropEventArea matching type hash
    processDropEventAreas(DropEvent {
        .type = DropEvent::Type::End,
        .pos = mousePosition(),
        .timestamp = SDL_GetTicks()
    });

    // Reset drop cache
    _eventCache.dropLock = ECS::NullEntity;
    _eventCache.drop = {};
    _eventCache.dropHoveredEntities.clear();
    invalidate();
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
    // @todo Replace backward search with binary search
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
    else
        return Area::ApplyClip(area, clipAreas.at(index));
}

void UI::UISystem::processEventHandlers(void) noexcept
{
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
    _eventCache.textQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processTextEventReceivers(event);
    });

    // If we drag while a mouse area is hovered, we must send leave event to avoid conflicts
    if (isDragging() && !_eventCache.mouseHoveredEntities.empty()) {
        kFAssert(_eventCache.mouseLock == ECS::NullEntity,
            "UI::UISystem::processEventHandlers: Cannot lock mouse while dragging");
        int x, y;
        const auto activeButtons = static_cast<Button>(SDL_GetMouseState(&x, &y));
        const MouseEvent leaveEvent {
            .pos = UI::Point(static_cast<UI::Pixel>(x), static_cast<UI::Pixel>(y)),
            .type = MouseEvent::Type::Leave,
            .activeButtons = activeButtons,
            .modifiers = static_cast<Modifier>(SDL_GetModState()),
            .timestamp = SDL_GetTicks()
        };
        for (const auto hoveredEntity : _eventCache.mouseHoveredEntities) {
            auto &component = get<MouseEventArea>(hoveredEntity);
            const auto &clippedArea = getClippedArea(hoveredEntity, get<Area>(hoveredEntity));
            const auto flags = component.event(leaveEvent, clippedArea, hoveredEntity, *this);
            if (Core::HasFlags(flags, EventFlags::Invalidate))
                invalidate();
        }
        _eventCache.mouseHoveredEntities.clear();
    }
}

void UI::UISystem::processMouseEventAreas(const MouseEvent &event) noexcept
{
    if (event.type == MouseEvent::Type::Motion)
        return processMouseEventAreasMotion(event);
    else
        return processMouseEventAreasAction(event);
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
        _eventCache.mouseHoveredEntities,
        // On enter
        [this](const MouseEvent &event, MouseEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
            MouseEvent mouseEvent { event };
            mouseEvent.type = MouseEvent::Type::Enter;
            return component.event(mouseEvent, clippedArea, entity, *this);
        },
        // On leave
        [this](const MouseEvent &event, MouseEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
            MouseEvent mouseEvent { event };
            mouseEvent.type = MouseEvent::Type::Leave;
            return component.event(mouseEvent, clippedArea, entity, *this);
        },
        // On inside
        [this](const MouseEvent &event, MouseEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
            return component.event(event, clippedArea, entity, *this);
        }
    );

    kFEnsure(!(isDragging() && _eventCache.mouseLock != ECS::NullEntity),
        "UI::UISystem::processMouseEventAreasMotion: Cannot lock mouse when dragging");
}

void UI::UISystem::processMouseEventAreasAction(const MouseEvent &event) noexcept
{
    // Handle drop trigger
    if (isDragging()) {
        const bool isButton = _eventCache.drop.dropTrigger.button == event.button;
        const bool isSameState = (_eventCache.drop.dropTrigger.buttonState && event.type == MouseEvent::Type::Press)
                | (!_eventCache.drop.dropTrigger.buttonState && event.type == MouseEvent::Type::Release);
        if (!isButton | !isSameState)
            return;
        // Send 'Drop' event
        processDropEventAreas(DropEvent {
            .type = DropEvent::Type::Drop,
            .pos = event.pos,
            .timestamp = event.timestamp
        });
        cancelDrag();
        return;
    }

    traverseClippedEventTable<MouseEventArea>(
        event,
        _eventCache.mouseLock,
        [this](const MouseEvent &event, MouseEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
            const auto updateHoverIndex = [this, entity](auto &hoverIndex) {
                for (auto loop = 0u; loop != 2; ++loop) {
                    if (hoverIndex < _eventCache.mouseHoveredEntities.size() && _eventCache.mouseHoveredEntities.at(hoverIndex) == entity)
                        return true;
                    hoverIndex = Core::Distance<std::uint32_t>(
                        _eventCache.mouseHoveredEntities.begin(),
                        _eventCache.mouseHoveredEntities.find(entity)
                    );
                }
                return false;
            };
            const auto manageNonExpectedEventFlags = [this](const EventFlags flags) {
                if (Core::HasFlags(flags, EventFlags::Invalidate))
                    invalidate();
            };
            auto hoverIndex = 0u;

            // Trigger enter event and add entity to hover list if not inside hover list
            if (!updateHoverIndex(hoverIndex)) {
                MouseEvent mouseEvent { event };
                mouseEvent.type = MouseEvent::Type::Enter;
                _eventCache.mouseHoveredEntities.push(entity);
                manageNonExpectedEventFlags(component.event(mouseEvent, clippedArea, entity, *this));
            }
            // The entity has been destroyed if not in the hover list anymore
            if (!updateHoverIndex(hoverIndex))
                return EventFlags::Stop;
            // Trigger action event
            const auto flags = component.event(event, clippedArea, entity, *this);
            // Return now if mouse action is inside area or is locked or entity has been destroyed
            if (clippedArea.contains(event.pos) || _eventCache.mouseLock == entity || !updateHoverIndex(hoverIndex))
                return flags;
            // Else send leave event and remove entity from hover list
            MouseEvent mouseEvent(event);
            mouseEvent.type = MouseEvent::Type::Leave;
            manageNonExpectedEventFlags(component.event(mouseEvent, clippedArea, entity, *this));
            if (updateHoverIndex(hoverIndex))
                _eventCache.mouseHoveredEntities.erase(_eventCache.mouseHoveredEntities.begin() + hoverIndex);
            return flags;
        }
    );

    kFEnsure(!(isDragging() && _eventCache.mouseLock != ECS::NullEntity),
        "UI::UISystem::processMouseEventAreasAction: Cannot lock mouse while dragging");
}

void UI::UISystem::processWheelEventAreas(const WheelEvent &event) noexcept
{
    traverseClippedEventTable<WheelEventArea>(
        event,
        _eventCache.wheelLock,
        [this](const WheelEvent &event, const WheelEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
            return component.event(event, clippedArea, entity, *this);
        }
    );
}

void UI::UISystem::processDropEventAreas(const DropEvent &event) noexcept
{
    switch (event.type) {
    case DropEvent::Type::Begin:
    case DropEvent::Type::End:
        getTable<DropEventArea>().traverse([this, &event](const ECS::Entity entity, DropEventArea &component) {
            component.event(
                _eventCache.drop.typeHash,
                _eventCache.drop.data(),
                event,
                getClippedArea(entity, getTable<Area>().get(entity)),
                entity,
                *this
            );
        });
        break;
    case DropEvent::Type::Motion:
    case DropEvent::Type::Enter:
    case DropEvent::Type::Leave:
        traverseClippedEventTableWithHover<DropEventArea>(
            event,
            _eventCache.dropLock,
            _eventCache.dropHoveredEntities,
            // On enter
            [this](const DropEvent &event, DropEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
                DropEvent dropEvent { event };
                dropEvent.type = DropEvent::Type::Enter;
                return component.event(_eventCache.drop.typeHash, _eventCache.drop.data(), dropEvent, clippedArea, entity, *this);
            },
            // On leave
            [this](const DropEvent &event, DropEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
                DropEvent dropEvent { event };
                dropEvent.type = DropEvent::Type::Leave;
                return component.event(_eventCache.drop.typeHash, _eventCache.drop.data(), dropEvent, clippedArea, entity, *this);
            },
            // On inside
            [this](const DropEvent &event, DropEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
                return component.event(_eventCache.drop.typeHash, _eventCache.drop.data(), event, clippedArea, entity, *this);
            }
        );
        break;
    case DropEvent::Type::Drop:
        traverseClippedEventTable<DropEventArea>(
            event,
            _eventCache.dropLock,
            [this](const DropEvent &event, DropEventArea &component, const Area &clippedArea, const ECS::Entity entity) {
                return component.event(_eventCache.drop.typeHash, _eventCache.drop.data(), event, clippedArea, entity, *this);
            }
        );
        break;
    }
}

void UI::UISystem::processKeyEventReceivers(const KeyEvent &event) noexcept
{
    auto &table = getTable<KeyEventReceiver>();

    // Send event to locked entity if any
    if (_eventCache.keyLock != ECS::NullEntity) {
        // Process locked event now
        const auto flags = table.get(_eventCache.keyLock).event(event, _eventCache.keyLock, *this);
        // If locked event flags tells to stop, return now
        if (processEventFlags(flags))
            return;
    }

    // Traverse all receivers
    table.traverse([this, &event](const ECS::Entity entity, KeyEventReceiver &component) {
        return !processEventFlags(component.event(event, entity, *this));
    });
}

void UI::UISystem::processTextEventReceivers(const TextEvent &event) noexcept
{
    auto &table = getTable<TextEventReceiver>();

    // Send event to locked entity if any
    if (_eventCache.textLock != ECS::NullEntity) {
        // Process locked event now
        const auto flags = table.get(_eventCache.textLock).event(event, _eventCache.textLock, *this);

        // If locked event flags tells to stop, return now
        if (processEventFlags(flags))
            return;
    }

    // Traverse all receivers
    table.traverse([this, &event](const ECS::Entity entity, TextEventReceiver &component) {
        return !processEventFlags(component.event(event, entity, *this));
    });
}

void UI::UISystem::processElapsedTime(void) noexcept
{
    // Query time
    const auto oldTick = _cache.lastTick;
    _cache.lastTick = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    // Compute elapsed time
    const auto elapsed = bool(oldTick != 0) * (_cache.lastTick - oldTick);
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
            invalidateState |= handler.event(static_cast<std::uint64_t>(elapsed));
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
        // Reset clip
        if (painter.currentClip() != DefaultClip)
            painter.setClip(DefaultClip);
        const auto mousePos = mousePosition();
        const Area area(mousePos - _eventCache.drop.size / 2, _eventCache.drop.size);
        if (auto &painterAreaEvent = _eventCache.drop.painterArea.event; painterAreaEvent)
            painterAreaEvent(painter, area);
    }
}


template<typename Component, typename Event, typename OnEvent, typename OtherComp>
inline ECS::Entity UI::UISystem::traverseClippedEventTable(const Event &event, const ECS::Entity entityLock, OnEvent &&onEvent) noexcept
{
    auto &table = getTable<Component>();
    auto &areaTable = getTable<OtherComp>();

    // Send event to locked entity if any
    if (entityLock != ECS::NullEntity) {
        // Compute clipped
        const auto clippedArea = getClippedArea(entityLock, areaTable.get(entityLock));

        // Process locked event without checking for mouse collision
        auto &component = table.get(entityLock);
        const auto flags = onEvent(event, component, clippedArea, entityLock);

        // If locked event flags tells to stop, return now
        if (processEventFlags(flags))
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
        if (processEventFlags(flags)) {
            hitEntity = entity;
            return false;
        }
        return true;
    });
    return hitEntity;
}

template<typename Component, typename Event, typename OnEnter, typename OnLeave, typename OnInside>
inline ECS::Entity UI::UISystem::traverseClippedEventTableWithHover(
    const Event &event,
    const ECS::Entity entityLock,
    EntityCache &hoveredEntities,
    OnEnter &&onEnter,
    OnLeave &&onLeave,
    OnInside &&onInside
) noexcept
{
    EntityCache hoverStack {};
    const auto discardHoveredEntities = [this, &event, &hoveredEntities, &onLeave, &hoverStack] {
        // Discard any entity not on the hover stack
        const auto it = std::remove_if(hoveredEntities.begin(), hoveredEntities.end(),
            [this, &event, &onLeave, &hoverStack, &table = getTable<Component>()](const ECS::Entity hoveredEntity) {
                const auto it = hoverStack.find(hoveredEntity);
                if (it != hoverStack.end())
                    return false;
                const auto unstableIndex = table.getUnstableIndex(hoveredEntity);
                if (unstableIndex == ECS::NullEntityIndex)
                    return true;
                auto &component = table.atIndex(unstableIndex);
                const auto &clippedArea = getClippedArea(hoveredEntity, get<Area>(hoveredEntity));
                const auto flags = onLeave(event, component, clippedArea, hoveredEntity);
                if (Core::HasFlags(flags, EventFlags::Invalidate))
                    invalidate();
                return true;
            }
        );
        if (it != hoveredEntities.end())
            hoveredEntities.erase(it, hoveredEntities.end());
    };
    const auto entity = traverseClippedEventTable<Component>(
        event,
        entityLock,
        [&hoveredEntities, &onEnter, &onInside, &hoverStack, &discardHoveredEntities](
            const Event &event, Component &component, const Area &clippedArea, const ECS::Entity entity
        ) {
            EventFlags flags;
            hoverStack.push(entity);
            // Hit entity is entered
            if (const auto it = hoveredEntities.find(entity); it == hoveredEntities.end()) {
                discardHoveredEntities();
                flags = onEnter(event, component, clippedArea, entity);
                hoveredEntities.push(entity);
            // Hit entity is already entered
            } else
                flags = onInside(event, component, clippedArea, entity);
            return flags;
        }
    );

    // Only allow locked entity to be hovered
    if (_eventCache.mouseLock != ECS::NullEntity) {
        hoverStack.clear();
        hoverStack.push(_eventCache.mouseLock);
    }
    discardHoveredEntities();
    return entity;
}

inline bool UI::UISystem::processEventFlags(const EventFlags flags) noexcept
{
    // Invalidate frame flag
    if (Core::HasFlags(flags, EventFlags::Invalidate))
        invalidate();

    // Return true on stop
    return !Core::HasFlags(flags, EventFlags::Propagate);
}

void UI::UISystem::dispatchDelayedEvents(void) noexcept
{
    for (auto &event : _eventCache.delayedEvents)
        event();
    _eventCache.delayedEvents.clear();
}

void UI::UISystem::setCursor(const Cursor cursor) noexcept
{
    if (_cursorCache.cursor == cursor)
        return;
    _cursorCache.cursor = cursor;
    SDL_SetCursor(reinterpret_cast<SDL_Cursor *>(_cursorCache.cursors.at(Core::ToUnderlying(cursor))));
}

bool UI::UISystem::relativeMouseMode(void) const noexcept
{
    return SDL_GetRelativeMouseMode();
}

void UI::UISystem::setRelativeMouseMode(const bool state) noexcept
{
    SDL_SetRelativeMouseMode(static_cast<SDL_bool>(state));
}

bool UI::UISystem::mouseGrab(void) const noexcept
{
    return SDL_GetWindowMouseGrab(_cache.window);
}

void UI::UISystem::setMouseGrab(const bool state) noexcept
{
    SDL_SetWindowMouseGrab(_cache.window, static_cast<SDL_bool>(state));
}

void UI::UISystem::setMouseCapture(const bool state) noexcept
{
    SDL_CaptureMouse(static_cast<SDL_bool>(state));
}

void UI::UISystem::setWindowGrab(const bool state) noexcept
{
    SDL_SetWindowGrab(_cache.window, static_cast<SDL_bool>(state));
}

bool UI::UISystem::keyboardGrab(void) const noexcept
{
    return SDL_GetWindowKeyboardGrab(_cache.window);
}

void UI::UISystem::setKeyboardGrab(const bool state) noexcept
{
    SDL_SetWindowKeyboardGrab(_cache.window, static_cast<SDL_bool>(state));
}

void UI::UISystem::setKeyboardInputMode(const bool state) noexcept
{
    if (state)
        SDL_StartTextInput();
    else
        SDL_StopTextInput();
}

UI::Point UI::UISystem::mousePosition(void) const noexcept
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    return Point(static_cast<Pixel>(x), static_cast<Pixel>(y));
}

void UI::UISystem::setMousePosition(const UI::Point pos) noexcept
{
    SDL_WarpMouseInWindow(_cache.window, static_cast<int>(pos.x), static_cast<int>(pos.y));
}