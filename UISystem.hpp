/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System
 */

#pragma once

#include <Kube/ECS/System.hpp>

#include "PresentPipeline.hpp"
#include "Renderer.hpp"
#include "Item.hpp"
#include "SpriteManager.hpp"
#include "FontManager.hpp"
#include "TraverseContext.hpp"
#include "EventQueue.hpp"
#include "Animator.hpp"

namespace kF::UI
{
    class UISystem;

    /** @brief Describe the drop trigger condition */
    struct DropTrigger
    {
        Button button { Button::Left };
        bool buttonState { false }; // False = Release, True = Press
    };

    /** @brief Concept of a lockable component */
    template<typename Component>
    concept LockComponentRequirements = std::is_same_v<Component, kF::UI::MouseEventArea>
        || std::is_same_v<Component, kF::UI::WheelEventArea>
        || std::is_same_v<Component, kF::UI::DropEventArea>
        || std::is_same_v<Component, kF::UI::KeyEventReceiver>
        || std::is_same_v<Component, kF::UI::TextEventReceiver>;

    /** @brief Keyboard input mode */
    enum class KeyboardInputMode
    {
        Key,
        Text
    };
}

/** @brief UI renderer system */
class alignas_double_cacheline kF::UI::UISystem
    : public ECS::System<"UISystem", PresentPipeline, UIAllocator,
        // Base (all items share these components)
        TreeNode,
        Area,
        Depth,
        // Layout
        Constraints,
        Layout,
        Transform,
        // Paint
        PainterArea,
        Clip,
        // Input
        ECS::StableComponent<MouseEventArea>,
        ECS::StableComponent<WheelEventArea>,
        ECS::StableComponent<DropEventArea>,
        ECS::StableComponent<KeyEventReceiver>,
        ECS::StableComponent<TextEventReceiver>,
        // Time
        ECS::StableComponent<Timer>,
        ECS::StableComponent<Animator>
    >
{
public:
    static_assert(std::is_same_v<UI::ComponentsTuple, UISystem::ComponentsTuple>, "UI::UISystem::Mismatching component tuple");

    /** @brief Entity list */
    using EntityCache = Core::SmallVector<ECS::Entity, (Core::CacheLineSize - Core::CacheLineQuarterSize) / sizeof(ECS::Entity), UIAllocator>;
    static_assert(std::is_same_v<UI::ComponentsTuple, ComponentsTuple>, "UI::UISystem: Mismatching component list");


    /** @brief Cache */
    struct alignas_cacheline Cache
    {
        // Root item
        ItemPtr root {};
        // Window size & DPI
        Size windowSize {};
        DPI windowDPI {};
        // Entities max depth
        DepthUnit maxDepth {};
        // Frame invalidation
        GPU::FrameIndex invalidateFlags { ~static_cast<GPU::FrameIndex>(0) };
        bool invalidateTree { true };
        // Time
        std::int64_t lastTick {};
        // Window
        GPU::BackendWindow *window {};
    };
    static_assert_fit_cacheline(Cache);

    struct alignas_double_cacheline DropCache
    {
        /** @brief Data functor */
        using DataFunctor = Core::Functor<const void *(void), UIAllocator>;

        TypeHash typeHash {};
        Size size {};
        DropTrigger dropTrigger {};
        DataFunctor data {};
        PainterArea painterArea {};
    };
    static_assert_fit_double_cacheline(DropCache);

    /** @brief Delayed event */
    using DelayedEvent = Core::Functor<void(void), UIAllocator>;

    /** @brief List of delayed events */
    using DelayedEvents = Core::Vector<DelayedEvent, UIAllocator>;

    /** @brief Event cache */
    struct alignas_double_cacheline EventCache
    {
        // Event queues
        EventQueuePtr<MouseEvent> mouseQueue {};
        EventQueuePtr<WheelEvent> wheelQueue {};
        EventQueuePtr<KeyEvent> keyQueue {};
        EventQueuePtr<TextEvent> textQueue {};
        // Locks
        ECS::Entity mouseLock { ECS::NullEntity };
        ECS::Entity wheelLock { ECS::NullEntity };
        ECS::Entity dropLock { ECS::NullEntity };
        ECS::Entity keyLock { ECS::NullEntity };
        ECS::Entity textLock { ECS::NullEntity };
        // Delayed events
        DelayedEvents delayedEvents {};
        // Drag & drop
        DropCache drop {};
        // Hover
        EntityCache mouseHoveredEntities {};
        EntityCache dropHoveredEntities {};
    };
    static_assert_alignof_double_cacheline(EventCache);
    static_assert_sizeof(EventCache, Core::CacheLineDoubleSize * 3);

    /** @brief Cache of cursor */
    struct alignas_half_cacheline CursorCache
    {
        Core::Vector<SDL_Cursor *, UIAllocator> cursors {};
        Cursor cursor {};
    };
    static_assert_fit_half_cacheline(CursorCache);


    /** @brief Virtual destructor */
    ~UISystem(void) noexcept override;

    /** @brief Constructor */
    UISystem(GPU::BackendWindow * const window) noexcept;


    /** @brief Get window size */
    [[nodiscard]] Size windowSize(void) const noexcept { return _cache.windowSize; }

    /** @brief Get window DPI */
    [[nodiscard]] DPI windowDPI(void) const noexcept { return _cache.windowDPI; }


    /** @brief Get current cursor */
    [[nodiscard]] Cursor cursor(void) const noexcept { return _cursorCache.cursor; }

    /** @brief Set current mouse cursor */
    void setCursor(const Cursor cursor) noexcept;


    /** @brief Get relative mouse mode state */
    [[nodiscard]] bool relativeMouseMode(void) const noexcept;

    /** @brief Set relative mouse mode state */
    void setRelativeMouseMode(const bool state) noexcept;


    /** @brief Get mouse grab state */
    [[nodiscard]] bool mouseGrab(void) const noexcept;

    /** @brief Set current mouse grab state */
    void setMouseGrab(const bool state) noexcept;


    /** @brief Set current mouse capture state */
    void setMouseCapture(const bool state) noexcept;


    /** @brief Set current window grab state */
    void setWindowGrab(const bool state) noexcept;


    /** @brief Get mouse position inside window */
    [[nodiscard]] UI::Point mousePosition(void) const noexcept;

    /** @brief Set mouse position inside window */
    void setMousePosition(const UI::Point pos) noexcept;


    /** @brief Get keyboard grab state */
    [[nodiscard]] bool keyboardGrab(void) const noexcept;

    /** @brief Set current keyboard grab state */
    void setKeyboardGrab(const bool state) noexcept;


    /** @brief Get scene max depth */
    [[nodiscard]] DepthUnit maxDepth(void) const noexcept { return _cache.maxDepth; }


    /** @brief Set clear color of UI renderer */
    inline void setClearColor(const Color &color) noexcept { _renderer.setClearColor(color); }


    /** @brief Get the sprite manager */
    [[nodiscard]] inline SpriteManager &spriteManager(void) noexcept { return _spriteManager; }

    /** @brief Get the font manager */
    [[nodiscard]] inline FontManager &fontManager(void) noexcept { return _fontManager; }


    /** @brief Get root item */
    [[nodiscard]] Item &root(void) noexcept { return *_cache.root; }
    [[nodiscard]] const Item &root(void) const noexcept { return *_cache.root; }

    /** @brief Construct root Item instance */
    template<typename Derived, typename ...Args>
        requires std::derived_from<Derived, Item>
    inline Derived &emplaceRoot(Args &&...args) noexcept
        { _cache.root = Core::UniquePtr<Derived, UIAllocator>::Make(std::forward<Args>(args)...); return reinterpret_cast<Derived &>(*_cache.root); }


    /** @brief Drag a type rendered with a given painter area */
    template<typename Type>
    inline void drag(Type &&type, const Size &size, PainterArea &&painterArea, const DropTrigger dropTrigger = {}) noexcept
        { onDrag(TypeHash::Get<std::remove_cvref_t<Type>>(), size, dropTrigger, [type = std::forward<Type>(type)] { return &type; }, std::move(painterArea)); }

    /** @brief Check if UISystem is currently dragging something */
    [[nodiscard]] inline bool isDragging(void) const noexcept { return _eventCache.drop.typeHash != TypeHash {}; }

    /** @brief Get dragged type */
    [[nodiscard]] inline TypeHash dragType(void) const noexcept { return _eventCache.drop.typeHash; }

    /** @brief Get dragged opaque instance */
    [[nodiscard]] inline const void *getDragOpaqueInstance(void) const noexcept { return _eventCache.drop.data(); }

    /** @brief Cancel a drag */
    void cancelDrag(void) noexcept;

    /** @brief Invalidate UI scene */
    void invalidate(void) noexcept;


    /** @brief Get locked entity */
    template<kF::UI::LockComponentRequirements Component>
    [[nodiscard]] inline ECS::Entity lockedEntity(void) const noexcept
        { return const_cast<UISystem *>(this)->lockedEntityImpl<Component>(); }

    /** @brief Lock an event component (overrides any locked entity) */
    template<kF::UI::LockComponentRequirements Component>
    inline void lockEvents(const ECS::Entity entity) noexcept { lockedEntityImpl<Component>() = entity; }

    /** @brief Unlock an event component, only if a lock exists for a given entity */
    template<kF::UI::LockComponentRequirements Component>
    void unlockEvents(const ECS::Entity entity) noexcept;

    /** @brief Unlock an event component */
    template<kF::UI::LockComponentRequirements Component>
    inline void unlockEvents(void) noexcept { lockEvents<Component>(ECS::NullEntity); }


    /** @brief Check if an entity is hovered */
    [[nodiscard]] inline bool isHovered(const ECS::Entity entity) const noexcept
        { return _eventCache.mouseHoveredEntities.find(entity) != _eventCache.mouseHoveredEntities.end(); }

    /** @brief Check if an entity is drop hovered */
    [[nodiscard]] inline bool isDropHovered(const ECS::Entity entity) const noexcept
        { return _eventCache.dropHoveredEntities.find(entity) != _eventCache.dropHoveredEntities.end(); }


    /** @brief Delay a callback to the end of current tick */
    inline void delayToTickEnd(DelayedEvent &&callback) noexcept
        { _eventCache.delayedEvents.push(std::move(callback)); }


    /** @brief Virtual tick callback */
    [[nodiscard]] bool tick(void) noexcept override;


    /** @brief Register renderer primitive */
    template<kF::UI::PrimitiveKind Primitive>
    inline void registerPrimitive(void) noexcept { _renderer.registerPrimitive<Primitive>(); }

private:
    // Item is a friend to prevent unsafe API access
    friend Item;

    // Hide system entity/components utilities
    using System::add;
    using System::addRange;
    using System::attach;
    using System::tryAttach;
    using System::attachRange;
    using System::dettach;
    using System::tryDettach;
    using System::dettachRange;
    using System::remove;
    using System::removeUnsafe;
    using System::removeUnsafeRange;


    /** @brief Get locked entity reference */
    template<kF::UI::LockComponentRequirements Component>
    [[nodiscard]] ECS::Entity &lockedEntityImpl(void) noexcept;


    /** @brief Dettach override */
    template<typename ...Components>
    inline void dettach(const ECS::Entity entity) noexcept { onDettach(entity); System::dettach<Components...>(entity); }

    /** @brief Try dettach override */
    template<typename ...Components>
    inline void tryDettach(const ECS::Entity entity) noexcept { onDettach(entity); System::tryDettach<Components...>(entity); }

    /** @brief Trigger callbacks on component dettached */
    template<typename ...Components>
    void onDettach(const ECS::Entity entity) noexcept;

    /** @brief Unsafe function notifying that mouse event area has been removed   */
    void onMouseEventAreaRemovedUnsafe(const ECS::Entity entity) noexcept;

    /** @brief Unsafe function notifying that wheel event area has been removed   */
    void onWheelEventAreaRemovedUnsafe(const ECS::Entity entity) noexcept;

    /** @brief Unsafe function notifying that drop event area has been removed   */
    void onDropEventAreaRemovedUnsafe(const ECS::Entity entity) noexcept;

    /** @brief Unsafe function notifying that key event receiver has been removed   */
    void onKeyEventReceiverRemovedUnsafe(const ECS::Entity entity) noexcept;

    /** @brief Unsafe function notifying that text event receiver has been removed   */
    void onTextEventReceiverRemovedUnsafe(const ECS::Entity entity) noexcept;


    /** @brief Check if a frame is invalid */
    [[nodiscard]] inline bool isFrameInvalid(const GPU::FrameIndex frame) const noexcept
        { return _cache.invalidateFlags & (static_cast<GPU::FrameIndex>(1) << frame); }

    /** @brief Validate a single frame */
    void validateFrame(const GPU::FrameIndex frame) noexcept;


    /** @brief Opaque type drag implementation */
    void onDrag(const TypeHash typeHash, const Size &size, const DropTrigger dropTrigger, DropCache::DataFunctor &&data, PainterArea &&painterArea) noexcept;


    /** @brief Get the clipped area of an entity */
    [[nodiscard]] Area getClippedArea(const ECS::Entity entity, const UI::Area &area) noexcept;


    /** @brief Sort every component tables that requires strong ordering */
    void sortTables(void) noexcept;


    /** @brief Process each event handler by consuming its queue */
    void processEventHandlers(void) noexcept;

    /** @brief Process a single MouseEvent by traversing MouseEventArea instances */
    void processMouseEventAreas(const MouseEvent &event) noexcept;

    /** @brief Process a single motion MouseEvent by traversing MouseEventArea instances */
    void processMouseEventAreasMotion(const MouseEvent &event) noexcept;

    /** @brief Process a single action MouseEvent by traversing MouseEventArea instances */
    void processMouseEventAreasAction(const MouseEvent &event) noexcept;

    /** @brief Process a single WheelEvent by traversing WheelEventArea instances */
    void processWheelEventAreas(const WheelEvent &event) noexcept;

    /** @brief Process a single DropEvent by traversing DropEventArea instances */
    void processDropEventAreas(const DropEvent &event) noexcept;

    /** @brief Process a single KeyEvent by traversing KeyEventReceiver instances */
    void processKeyEventReceivers(const KeyEvent &event) noexcept;

    /** @brief Process a single TextEvent by traversing TextEventReceiver instances */
    void processTextEventReceivers(const TextEvent &event) noexcept;


    /** @brief Process system time elapsed */
    void processElapsedTime(void) noexcept;

    /** @brief Process all Timer instances */
    [[nodiscard]] bool processTimers(const std::int64_t elapsed) noexcept;

    /** @brief Process all Animator instances */
    [[nodiscard]] bool processAnimators(const std::int64_t elapsed) noexcept;


    /** @brief Process all PainterArea instances */
    void processPainterAreas(void) noexcept;


    /** @brief Dispatch delayed events */
    void dispatchDelayedEvents(void) noexcept;


    /** @brief Traverse a table requiring clipped area */
    template<typename Component, typename Event, typename OnEvent, typename FixMSVCPLZ = Area> // @todo fix this ****
    ECS::Entity traverseClippedEventTable(const Event &event, const ECS::Entity entityLock, OnEvent &&onEvent) noexcept;

    /** @brief Traverse a table requiring clipped area & hover management */
    template<typename Component, typename Event, typename OnEnter, typename OnLeave, typename OnInside>
    ECS::Entity traverseClippedEventTableWithHover(
        const Event &event,
        const ECS::Entity entityLock,
        EntityCache &hoveredEntities,
        OnEnter &&onEnter,
        OnLeave &&onLeave,
        OnInside &&onInside
    ) noexcept;


    /** @brief Process EventFlags returned by event components
     *  @return True if the event flags requires to stop event processing */
    [[nodiscard]] bool processEventFlags(const EventFlags flags) noexcept;



    /** @brief Query current window Size */
    [[nodiscard]] static Size GetWindowSize(void) noexcept;

    /** @brief Query current window DPI */
    [[nodiscard]] static DPI GetWindowDPI(void) noexcept;

    // Cacheline N -> N + 1
    Internal::TraverseContext _traverseContext {};
    // Cacheline N + 2 -> N + 3
    SpriteManager _spriteManager {};
    // Cacheline N + 4
    FontManager _fontManager {};
    // Cacheline N + 5
    Cache _cache {};
    // Cacheline N + 6 -> N + 11
    EventCache _eventCache {};
    // Cacheline N + 11 -> N + 14
    Renderer _renderer;
    // Cursors
    CursorCache _cursorCache {};
};
static_assert_alignof_double_cacheline(kF::UI::UISystem);
static_assert_sizeof(kF::UI::UISystem, kF::Core::CacheLineDoubleSize * 17);

#include "Item.ipp"
#include "UISystem.ipp"