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
        MouseEventArea,
        MotionEventArea,
        WheelEventArea,
        DropEventArea,
        KeyEventReceiver,
        // Time
        Timer,
        Animator
    >
{
public:
    static_assert(std::is_same_v<UI::ComponentsTuple, ComponentsTuple>, "UI::UISystem: Mismatching component list");

    /** @brief Cache */
    struct alignas_cacheline Cache
    {
        ItemPtr root {};
        Size windowSize {};
        DPI windowDPI {};
        DepthUnit maxDepth {};
        GPU::FrameIndex invalidateFlags { ~static_cast<GPU::FrameIndex>(0) };
        bool invalidateTree { true };
    };
    static_assert_fit_cacheline(Cache);

    struct DragCache
    {
        TypeHash type {};
        const void *data {};
        PainterArea painterArea {};
    };

    /** @brief Event cache */
    struct alignas_double_cacheline EventCache
    {
        // Event queues
        EventQueuePtr<MouseEvent> mouseQueue {};
        EventQueuePtr<MotionEvent> motionQueue {};
        EventQueuePtr<WheelEvent> wheelQueue {};
        EventQueuePtr<KeyEvent> keyQueue {};
        // Locks
        ECS::Entity mouseLock { ECS::NullEntity };
        ECS::Entity motionLock { ECS::NullEntity };
        ECS::Entity wheelLock { ECS::NullEntity };
        ECS::Entity keyLock { ECS::NullEntity };
        // Hover
        ECS::Entity lastHovered { ECS::NullEntity };
        // Time
        std::int64_t lastTick {};
        // Drag
        DragCache drag {};
    };
    static_assert_fit_double_cacheline(EventCache);


    /** @brief Virtual destructor */
    ~UISystem(void) noexcept override;

    /** @brief Constructor */
    UISystem(void) noexcept;


    /** @brief Get window size */
    [[nodiscard]] Size windowSize(void) const noexcept { return _cache.windowSize; }

    /** @brief Get window DPI */
    [[nodiscard]] DPI windowDPI(void) const noexcept { return _cache.windowDPI; }


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
    void drag(const Type &type, const PainterArea &painterArea) noexcept;


    /** @brief Invalidate UI scene */
    void invalidate(void) noexcept;


    /** @brief Virtual tick callback */
    [[nodiscard]] bool tick(void) noexcept override;


    /** @brief Register renderer primitive */
    template<kF::UI::PrimitiveKind Primitive>
    inline void registerPrimitive(void) noexcept { _renderer.registerPrimitive<Primitive>(); }

private:
    /** @brief Check if a frame is invalid */
    [[nodiscard]] inline bool isFrameInvalid(const GPU::FrameIndex frame) const noexcept
        { return _cache.invalidateFlags & (static_cast<GPU::FrameIndex>(1) << frame); }

    /** @brief Validate a single frame */
    void validateFrame(const GPU::FrameIndex frame) noexcept;


    /** @brief Get the clipped area of an entity */
    [[nodiscard]] Area getClippedArea(const ECS::Entity entity, const UI::Area &area) noexcept;


    /** @brief Sort every component tables that requires strong ordering */
    void sortTables(void) noexcept;


    /** @brief Process each event handler by consuming its queue */
    void processEventHandlers(void) noexcept;

    /** @brief Process a single MouseEvent by traversing MouseEventArea instances */
    void processMouseEventAreas(const MouseEvent &event) noexcept;

    /** @brief Process a single MotionEvent by traversing MotionEventArea instances */
    void processMotionEventAreas(const MotionEvent &event) noexcept;

    /** @brief Process a single WheelEvent by traversing WheelEventArea instances */
    void processWheelEventAreas(const WheelEvent &event) noexcept;

    /** @brief Process a single KeyEvent by traversing KeyEventReceiver instances */
    void processKeyEventReceivers(const KeyEvent &event) noexcept;

    /** @brief Traverse a table requiring clipped area */
    template<typename Component, typename Event, typename OnEvent, typename FixMSVCPLZ = Area> // @todo fix this ****
    ECS::Entity traverseClippedEventTable(const Event &event, ECS::Entity &entityLock, OnEvent &&onEvent) noexcept;


    /** @brief Process EventFlags returned by event components
     *  @return True if the event flags requires to stop event processing */
    template<typename Table>
    [[nodiscard]] bool processEventFlags(
            const Table &table, const typename Table::ValueType &value, ECS::Entity &lock, const EventFlags flags) noexcept;


    /** @brief Process system time elapsed */
    void processElapsedTime(void) noexcept;

    /** @brief Process all Timer instances */
    [[nodiscard]] bool processTimers(const std::int64_t elapsed) noexcept;

    /** @brief Process all Animator instances */
    [[nodiscard]] bool processAnimators(const std::int64_t elapsed) noexcept;


    /** @brief Process all PainterArea instances */
    void processPainterAreas(void) noexcept;


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
    // Cacheline N + 6 -> N + 7
    EventCache _eventCache {};
    // Cacheline N + 7 -> N + 10
    Renderer _renderer;
};
static_assert_alignof_double_cacheline(kF::UI::UISystem);
static_assert_sizeof(kF::UI::UISystem, kF::Core::CacheLineDoubleSize * 14);

#include "Item.ipp"
#include "UISystem.ipp"