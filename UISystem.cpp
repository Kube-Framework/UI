/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System
 */

#include <SDL2/SDL.h>

#include <Kube/GPU/GPU.hpp>
#include <Kube/UI/App.hpp>

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
    _root.release();
}

UI::UISystem::UISystem(void) noexcept
    :   _windowSize(GetWindowSize()),
        _windowDPI(GetWindowDPI()),
        _mouseQueue(parent().getSystem<EventSystem>().addEventQueue<MouseEvent>()),
        _motionQueue(parent().getSystem<EventSystem>().addEventQueue<MotionEvent>()),
        _wheelQueue(parent().getSystem<EventSystem>().addEventQueue<WheelEvent>()),
        _keyQueue(parent().getSystem<EventSystem>().addEventQueue<KeyEvent>()),
        _renderer(*this)
{
    GPU::GPUObject::Parent().viewSizeDispatcher().add([this] {
        _windowSize = GetWindowSize();
        _windowDPI = GetWindowDPI();
        invalidate();
    });

    // Rebuild graph
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
        // Build layouts using LayoutBuilder
        _maxDepth = Internal::LayoutBuilder(*this, _traverseContext).build();

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
    _mouseQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processMouseEventAreas(event);
    });
    _motionQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processMotionEventAreas(event);
    });
    _wheelQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processWheelEventAreas(event);
    });
    _keyQueue->consume([this](const auto &range) {
        for (const auto &event : range)
            processKeyEventReceivers(event);
    });
}

void UI::UISystem::processMouseEventAreas(const MouseEvent &event) noexcept
{
    traverseClippedEventTable<MouseEventArea>(event, _mouseLock);
}

void UI::UISystem::processMotionEventAreas(const MotionEvent &event) noexcept
{
    traverseClippedEventTable<MotionEventArea>(event, _motionLock);
}

void UI::UISystem::processWheelEventAreas(const WheelEvent &event) noexcept
{
    traverseClippedEventTable<WheelEventArea>(event, _wheelLock);
}

void UI::UISystem::processKeyEventReceivers(const KeyEvent &event) noexcept
{
    auto &keyTable = getTable<KeyEventReceiver>();

    for (const auto &handler : keyTable) {
        const auto flags = handler.event(event);
        if (processEventFlags(keyTable, handler, _keyLock, flags))
            break;
    }
}

template<typename Component, typename Event>
inline void UI::UISystem::traverseClippedEventTable(const Event &event, ECS::Entity &entityLock) noexcept
{
    auto &table = getTable<Component>();
    auto &areaTable = getTable<Area>();
    std::uint32_t index {};

    for (const auto &component : table) {
        const auto entity = table.entities().at(index++);
        const auto area = areaTable.get(entity);

        // Test non-clipped area
        if (!area.contains(event.pos)) [[likely]]
            continue;

        // Test clipped area
        if (!getClippedArea(entity, area).contains(event.pos)) [[likely]]
            continue;

        // Event hit
        const auto flags = component.event(event, area);
        if (processEventFlags(table, component, entityLock, flags))
            break;
    }
}

template<typename Table>
inline bool UI::UISystem::processEventFlags(
        const Table &table, const Table::ValueType &value, ECS::Entity &lock, const EventFlags flags) noexcept
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