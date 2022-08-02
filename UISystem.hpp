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
        KeyEventReceiver,
        // Time
        Timer,
        Animator
    >
{
public:
    static_assert(std::is_same_v<UI::ComponentsTuple, ComponentsTuple>, "UI::UISystem: Mismatching component list");

    /** @brief Virtual destructor */
    ~UISystem(void) noexcept override;

    /** @brief Constructor */
    UISystem(void) noexcept;


    /** @brief Get window size */
    [[nodiscard]] Size windowSize(void) const noexcept { return _windowSize; }

    /** @brief Get window DPI */
    [[nodiscard]] DPI windowDPI(void) const noexcept { return _windowDPI; }


    /** @brief Get scene max depth */
    [[nodiscard]] DepthUnit maxDepth(void) const noexcept { return _maxDepth; }


    /** @brief Get root item */
    [[nodiscard]] Item &root(void) noexcept { return *_root; }
    [[nodiscard]] const Item &root(void) const noexcept { return *_root; }


    /** @brief Set clear color of UI renderer */
    inline void setClearColor(const Color &color) noexcept { _renderer.setClearColor(color); }


    /** @brief Register renderer primitive */
    template<kF::UI::PrimitiveKind Primitive>
    inline void registerPrimitive(void) noexcept { _renderer.registerPrimitive<Primitive>(); }

    /** @brief Construct root Item instance */
    template<typename Derived, typename ...Args>
        requires std::derived_from<Derived, Item>
    inline Derived &emplaceRoot(Args &&...args) noexcept
        { _root = Core::UniquePtr<Derived, UIAllocator>::Make(std::forward<Args>(args)...); return reinterpret_cast<Derived &>(*_root); }


    /** @brief Invalidate UI scene */
    void invalidate(void) noexcept;


    /** @brief Get the sprite manager */
    [[nodiscard]] SpriteManager &spriteManager(void) noexcept { return _spriteManager; }

    /** @brief Add a sprite to the SpriteManager using its path if it doesn't exists
     *  @note If the sprite already loaded this function does not duplicate its memory */
    [[nodiscard]] inline Sprite addSprite(const std::string_view &path) noexcept { return _spriteManager.add(path); }


    /** @brief Get the font manager */
    [[nodiscard]] FontManager &fontManager(void) noexcept { return _fontManager; }

    /** @brief Add a font to the FontManager using its path if it doesn't exists
     *  @note If the font already loaded this function does not duplicate its memory */
    [[nodiscard]] inline Font addFont(const std::string_view &path, const FontModel &model) noexcept
        { return _fontManager.add(path, model); }


    /** @brief Virtual tick callback */
    [[nodiscard]] bool tick(void) noexcept override;


private:
    /** @brief Check if a frame is invalid */
    inline bool isFrameInvalid(const GPU::FrameIndex frame) const noexcept
        { return _invalidateFlags & (static_cast<GPU::FrameIndex>(1) << frame); }

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
    template<typename Component, typename Event>
    void traverseClippedEventTable(const Event &event, ECS::Entity &entityLock) noexcept;


    /** @brief Process EventFlags returned by event components
     *  @return True if the event flags requires to stop event processing */
    template<typename Table>
    [[nodiscard]] bool processEventFlags(
            const Table &table, const Table::ValueType &value, ECS::Entity &lock, const EventFlags flags) noexcept;


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
    // Cacheline N + 5 -> N + 6
    ItemPtr _root {};
    Size _windowSize {};
    DPI _windowDPI {};
    DepthUnit _maxDepth {};
    GPU::FrameIndex _invalidateFlags { ~static_cast<GPU::FrameIndex>(0) };
    bool _invalidateTree { true };
    std::int64_t _lastTick {};
    EventQueuePtr<MouseEvent> _mouseQueue {};
    EventQueuePtr<MotionEvent> _motionQueue {};
    EventQueuePtr<WheelEvent> _wheelQueue {};
    EventQueuePtr<KeyEvent> _keyQueue {};
    ECS::Entity _mouseLock { ECS::NullEntity };
    ECS::Entity _motionLock { ECS::NullEntity };
    ECS::Entity _wheelLock { ECS::NullEntity };
    ECS::Entity _keyLock { ECS::NullEntity };
    // Cacheline N + 7 -> N + 10
    Renderer _renderer;
};
static_assert_alignof_double_cacheline(kF::UI::UISystem);
static_assert_sizeof(kF::UI::UISystem, kF::Core::CacheLineDoubleSize * 14);

#include "Item.ipp"
#include "UISystem.ipp"
