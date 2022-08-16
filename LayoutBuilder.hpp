/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Layout processor
 */

#pragma once

#include "Components.hpp"

namespace kF::UI
{
    class UISystem;

    namespace Internal
    {
        class LayoutBuilder;
        class TraverseContext;

        /** @brief Accumulate template option */
        enum class Accumulate { No, Yes };

        /** @brief Axis template option */
        enum class Axis { Horizontal, Vertical };

        /** @brief Bound template option */
        enum class BoundType { Unknown, Fixed, Infinite };

        /** @brief Types of flex flow */
        enum class FlexFlow { Horizontal, Vertical };
    }
}

/** @brief Item layout builder */
class kF::UI::Internal::LayoutBuilder
{
public:
    /** @brief Alias to entity index range */
    using EntityIndexRange = Core::IteratorRange<const ECS::EntityIndex *>;


    /** @brief Destructor */
    inline ~LayoutBuilder(void) noexcept = default;

    /** @brief Constructor */
    inline LayoutBuilder(UISystem &uiSystem, TraverseContext &traverseContext) noexcept
        : _uiSystem(&uiSystem), _traverseContext(&traverseContext) {}


    /** @brief Build item layouts of UISystem
     *  @return Maximum depth */
    [[nodiscard]] DepthUnit build(void) noexcept;


private:
    /** @brief Process item constraints in recursive bottom to top order */
    void traverseConstraints(void) noexcept;

    /** @brief Get the counter insert index of a current entity according to its parent children and the already inserted children in counter */
    [[nodiscard]] std::uint32_t getCounterInsertIndex(const TreeNode &parentNode) noexcept;

    /** @brief Process constraints of a layout item */
    void buildLayoutConstraints(Constraints &constraints) noexcept;

    /** @brief Compute 'constraints' using children constraints */
    template<Accumulate AccumulateX, Accumulate AccumulateY>
    void computeChildrenHugConstraints(Constraints &constraints, const Pixel spacing, const bool hugWidth, const bool hugHeight) noexcept;

    /** @brief Compute flex 'constraints' using children constraints */
    template<Axis DistributionAxis, auto GetX, auto GetY>
    void computeFlexChildrenHugConstraints(Constraints &constraints, const Layout &layout, const bool hugWidth, const bool hugHeight) noexcept;


    /** @brief Process item areas in recursive top to bottom order */
    void traverseAreas(void) noexcept;

    /** @brief Process area of a layout item children */
    void buildLayoutArea(const Area &contextArea) noexcept;

    /** @brief Compute every children area within the given context area */
    void computeChildrenArea(const Area &contextArea, const Anchor anchor) noexcept;

    /** @brief Compute every children area within the given context area, distributing over axis */
    template<Axis DistributionAxis, auto GetX, auto GetY>
    void computeLayoutChildrenArea(const Area &contextArea, const Layout &layout, const EntityIndexRange &childEntityRange) noexcept;

    /** @brief Compute every children area within the given context area, distributing over axis */
    template<Axis DistributionAxis, auto GetX, auto GetY>
    void computeFlexLayoutChildrenArea(const Area &contextArea, const Layout &layout) noexcept;

    /** @brief Compute a flex layout line */
    template<auto GetX, auto GetY>
    void computeFlexLayoutChildrenLineMetrics(
            EntityIndexRange &childIndexRange, const Pixel spacing, const Pixel lineWidth, Pixel &lineHeight, Pixel &lineRemain) noexcept;


    /** @brief Apply transform to item area */
    void applyTransform(const ECS::EntityIndex entityIndex, Area &area) noexcept;


    UISystem *_uiSystem {};
    TraverseContext *_traverseContext {};
    DepthUnit _maxDepth {};
};