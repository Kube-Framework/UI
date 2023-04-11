/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Layout processor
 */

#pragma once

#include "TraverseContext.hpp"

namespace kF::UI
{
    class UISystem;

    namespace Internal
    {
        class LayoutBuilder;
        class TraverseContext;
    }
}

/** @brief Item layout builder */
class kF::UI::Internal::LayoutBuilder
{
public:
    /** @brief Destructor */
    inline ~LayoutBuilder(void) noexcept = default;

    /** @brief Constructor */
    inline LayoutBuilder(UISystem &uiSystem, TraverseContext &traverseContext) noexcept
        : _uiSystem(uiSystem), _traverseContext(traverseContext) {}


    /** @brief Build item layouts of UISystem
     *  @return Maximum depth */
    [[nodiscard]] DepthUnit build(void) noexcept;

private:
    /** @brief Discover and resolve constraints from the current traverse context entity to the bottom of item tree
     *  @note An entity must be setup using TraverseContext::setupEntity
     *  Some complex constraints can fail to resolve, resolveSizes will resolve them later with more context */
    void discoverConstraints(void) noexcept;


    /** @brief Resolve constraints from the current traverse context entity to the bottom of item tree
     *  @note An entity must be setup using TraverseContext::setupEntity */
    void resolveConstraints(const TraverseContext::ResolveData &parentData) noexcept;

    /** @brief Query size from the current traverse context entity
     *  @note An entity must be setup using TraverseContext::setupEntity
     *  This function may take further recursion if the node constraints are still undefined */
    [[nodiscard]] Size querySize(const Size &parentSize) noexcept;


    /** @brief Resolve areas from the current traverse context entity to the bottom of item tree
     *  @note An entity must be setup using TraverseContext::setupEntity */
    void resolveAreas(void) noexcept;


    /** @brief Apply transform to item area */
    void applyTransform(const ECS::EntityIndex entityIndex, Area &area) noexcept;


    UISystem &_uiSystem;
    TraverseContext &_traverseContext;
    DepthUnit _maxDepth {};
    Layout _defaultLayout {};
};