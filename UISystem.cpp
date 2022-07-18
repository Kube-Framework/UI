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

kF::UI::Area kF::UI::UISystem::getClippedArea(const ECS::Entity entity, const UI::Area &area) noexcept
{
    const auto &clipDepths = _traverseContext.clipDepths();
    if (clipDepths.empty())
        return area;

    // Find depth index
    const auto depth = get<UI::Depth>(entity).depth;
    std::uint32_t index {};
    const auto count = clipDepths.size<std::uint32_t>();
    while (index != count && clipDepths[index] < depth) [[likely]]
        ++index;

    // Query clip area
    const auto &clipAreas = _traverseContext.clipAreas();
    const Area *clip {};
    if (index == count) // Last
        clip = &clipAreas.back();
    else if (index) // index - 1
        clip = &clipAreas.at(index - 1);

    // If a clip has been found, apply it to entity's Area
    if (clip && *clip != Area {}) [[likely]]
        return UI::Area::ApplyClip(area, *clip);
    // Else return entity's Area
    else
        return area;
}

void kF::UI::UISystem::processEventHandlers(void) noexcept
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

void kF::UI::UISystem::processMouseEventAreas(const MouseEvent &event) noexcept
{
    auto &areaTable = getTable<Area>();
    auto &mouseTable = getTable<MouseEventArea>();

    if (_mouseLock != ECS::NullEntity) [[unlikely]] {
        auto &handler = mouseTable.get(_mouseLock);
        const auto flags = handler.event(event, areaTable.get(_mouseLock));
        if (processEventFlags(mouseTable, handler, _mouseLock, flags))
            return;
    }

    for (std::uint32_t index {}; const auto &handler : mouseTable) {
        const auto entity = mouseTable.entities().at(index++);
        auto &area = areaTable.get(entity);
        if (area.contains(event.pos) && getClippedArea(entity, area).contains(event.pos)) [[unlikely]] {
            const auto flags = handler.event(event, area);
            if (processEventFlags(mouseTable, handler, _mouseLock, flags))
                break;
        }
    }
}

void kF::UI::UISystem::processMotionEventAreas(const MotionEvent &event) noexcept
{
    auto &areaTable = getTable<Area>();
    auto &motionTable = getTable<MotionEventArea>();

    for (std::uint32_t index {}; const auto &handler : motionTable) {
        const auto entity = motionTable.entities().at(index++);
        auto &area = areaTable.get(entity);
        if (area.contains(event.pos) && getClippedArea(entity, area).contains(event.pos)) [[unlikely]] {
            const auto flags = handler.event(event, area);
            if (processEventFlags(motionTable, handler, _motionLock, flags))
                break;
        }
    }
}

void kF::UI::UISystem::processWheelEventAreas(const WheelEvent &event) noexcept
{
    auto &areaTable = getTable<Area>();
    auto &wheelTable = getTable<WheelEventArea>();

    for (std::uint32_t index {}; const auto &handler : wheelTable) {
        const auto entity = wheelTable.entities().at(index++);
        auto &area = areaTable.get(entity);
        if (area.contains(event.pos) && getClippedArea(entity, area).contains(event.pos)) [[unlikely]] {
            const auto flags = handler.event(event, area);
            if (processEventFlags(wheelTable, handler, _wheelLock, flags))
                break;
        }
    }
}

void kF::UI::UISystem::processKeyEventReceivers(const KeyEvent &event) noexcept
{
    auto &keyTable = getTable<KeyEventReceiver>();

    for (const auto &handler : keyTable) {
        const auto flags = handler.event(event);
        if (processEventFlags(keyTable, handler, _keyLock, flags))
            break;
    }
}

template<typename Table>
inline bool kF::UI::UISystem::processEventFlags(
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