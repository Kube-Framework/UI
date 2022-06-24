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

    namespace Internal
    {
        /** @brief Accumulate template option */
        enum class Accumulate { No, Yes };

        /** @brief Axis template option */
        enum class Axis { Horizontal, Vertical };

        /** @brief Bound template option */
        enum class BoundType { Unknown, Fixed, Infinite };
    }
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


    /** @brief Process system time elapsed */
    void processElapsedTime(void) noexcept;

    /** @brief Process all Timer instances */
    [[nodiscard]] bool processTimers(const std::int64_t elapsed) noexcept;

    /** @brief Process all Animator instances */
    [[nodiscard]] bool processAnimators(const std::int64_t elapsed) noexcept;


    /** @brief Process all PainterArea instances */
    void processPainterAreas(void) noexcept;


    /** @brief Process all Area instances */
    void processAreas(void) noexcept;

    /** @brief Process item constraints in recursive bottom to top order */
    void traverseConstraints(void) noexcept;

    /** @brief Process constraints of a layout item */
    void buildLayoutConstraints(Constraints &constraints) noexcept;

    /** @brief Process item areas in recursive top to bottom order */
    void traverseAreas(void) noexcept;

    /** @brief Process area of a layout item children */
    void buildLayoutArea(const Area &contextArea) noexcept;


    /** @brief Sort every component tables that requires strong ordering */
    void sortTables(void) noexcept;


    /** @brief Compute children constraints to given constraints */
    template<Internal::Accumulate AccumulateX, Internal::Accumulate AccumulateY>
    void computeChildrenConstraints(Constraints &constraints, const bool hugWidth, const bool hugHeight) noexcept;

    /** @brief Compute axis constraint (rhs) to another (lhs) */
    template<Internal::Accumulate AccumulateValue>
    static void ComputeAxisHugConstraint(Pixel &lhs, const Pixel rhs) noexcept;


    /** @brief Compute every children area within the given context area */
    void computeChildrenArea(const Area &contextArea, const Anchor anchor) noexcept;

    /** @brief Compute every children area within the given context area, distributing over axis */
    template<Internal::Axis DistributionAxis>
    void computeDistributedChildrenArea(const Area &contextArea, const Layout &layout) noexcept;

    /** @brief Compute distributed child size inside parent space according to min/max range and spacing */
    template<bool Distribute>
    [[nodiscard]] static Pixel ComputeDistributedSize(Pixel &flexCount, Pixel &freeSpace,
            const Pixel parent, const Pixel min, const Pixel max) noexcept;


    /** @brief Compute child size inside parent space according to min/max range */
    template<Internal::BoundType Bound>
    [[nodiscard]] static Pixel ComputeSize(const Pixel parent, const Pixel min, const Pixel max) noexcept;


    /** @brief Compute position of an item using its context area and an anchor */
    static void ComputePosition(Area &area, const Area &contextArea, const Anchor anchor) noexcept;

    /** @brief Compute position of a distributed item using its context area and an anchor */
    template<Internal::Axis DistributionAxis>
    static void ComputeDistributedPosition(Point &offset, Area &area, const Area &contextArea, const Pixel spacing, const Anchor anchor) noexcept;

    /** @brief Apply transform to item area */
    void applyTransform(const ECS::EntityIndex entityIndex, Area &area) noexcept;


    /** @brief Query current window Size */
    [[nodiscard]] static Size GetWindowSize(void) noexcept;

    /** @brief Query current window DPI */
    [[nodiscard]] static DPI GetWindowDPI(void) noexcept;


    // Cacheline N
    Size _windowSize {};
    DPI _windowDPI {};
    ItemPtr _root {};
    GPU::FrameIndex _invalidateFlags { ~static_cast<GPU::FrameIndex>(0) };
    bool _invalidateTree { true };
    std::int64_t _lastTick {};
    DepthUnit _maxDepth {};
    // Cacheline N + 1
    Internal::TraverseContext _traverseContext {};
    // Cacheline N + 2 -> N + 3
    SpriteManager _spriteManager {};
    // Cacheline N + 4
    FontManager _fontManager {};
    // Cacheline N + 5
    EventQueuePtr<MouseEvent> _mouseQueue {};
    EventQueuePtr<MotionEvent> _motionQueue {};
    EventQueuePtr<WheelEvent> _wheelQueue {};
    EventQueuePtr<KeyEvent> _keyQueue {};
    // Cacheline N + 6 -> N + 9
    Renderer _renderer;
};
static_assert_alignof_double_cacheline(kF::UI::UISystem);
static_assert_sizeof(kF::UI::UISystem, kF::Core::CacheLineDoubleSize * 14);

#include "Item.ipp"
#include "UISystem.ipp"
