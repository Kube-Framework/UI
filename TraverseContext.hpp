/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System Traverse Context
 */

#pragma once

#include <Kube/Core/Vector.hpp>

#include "Components.hpp"

namespace kF::UI::Internal
{
    class TraverseContext;
}

/** @brief Traversal context */
class alignas_cacheline kF::UI::Internal::TraverseContext
{
public:
    /** @brief Entity children counter, used to retain indexes during traversing phase */
    struct alignas_half_cacheline Counter
        : public Core::SmallVector<ECS::EntityIndex, Core::CacheLineQuarterSize / sizeof(ECS::EntityIndex), UIAllocator> {};


    /** @brief Get context entity */
    [[nodiscard]] inline ECS::Entity entity(void) const noexcept { return _entity; }

    /** @brief Get context entity index */
    [[nodiscard]] inline ECS::EntityIndex entityIndex(void) const noexcept { return _entityIndex; }


    /** @brief Get the entity index of an entity using its node */
    [[nodiscard]] inline ECS::EntityIndex entityIndexOf(const TreeNode &node) const noexcept
        { return static_cast<ECS::EntityIndex>(std::distance(const_cast<const TreeNode *>(_nodeBegin), &node)); }

    /** @brief Get the constraints of an entity */
    [[nodiscard]] inline Constraints &constraintsAt(const ECS::EntityIndex entityIndex) noexcept { return _constraints.at(entityIndex); }
    [[nodiscard]] inline Constraints &constraints(void) noexcept { return constraintsAt(_entityIndex); }

    /** @brief Get the counter of an entity */
    [[nodiscard]] inline Counter &counterAt(const ECS::EntityIndex entityIndex) noexcept { return _counters.at(entityIndex); }
    [[nodiscard]] inline Counter &counter(void) noexcept { return counterAt(_entityIndex); }

    /** @brief Get the node of an entity */
    [[nodiscard]] inline TreeNode &nodeAt(const ECS::EntityIndex entityIndex) noexcept { return _nodeBegin[entityIndex]; }
    [[nodiscard]] inline TreeNode &node(void) noexcept { return nodeAt(_entityIndex); }

    /** @brief Get the area of an entity */
    [[nodiscard]] inline Area &areaAt(const ECS::EntityIndex entityIndex) noexcept { return _areaBegin[entityIndex]; }
    [[nodiscard]] inline Area &area(void) noexcept { return areaAt(_entityIndex); }

    /** @brief Get the depth of an entity */
    [[nodiscard]] inline Depth &depthAt(const ECS::EntityIndex entityIndex) noexcept { return _depthBegin[entityIndex]; }
    [[nodiscard]] inline Depth &depth(void) noexcept { return depthAt(_entityIndex); }


    /** @brief Setup initital context for traversal */
    void setupContext(const std::uint32_t count, TreeNode * const nodeBegin, Area * const areaBegin, Depth * const depthBegin) noexcept;

    /** @brief Setup the next entity for traversal recursion */
    inline void setupEntity(const ECS::Entity entity, const ECS::EntityIndex entityIndex) noexcept
        { _entity = entity; _entityIndex = entityIndex; }


private:
    Core::Vector<Constraints, UIAllocator> _constraints {};
    Core::Vector<Counter, UIAllocator> _counters {};
    TreeNode *_nodeBegin {};
    Area *_areaBegin {};
    Depth *_depthBegin {};
    ECS::Entity _entity {};
    ECS::EntityIndex _entityIndex {};
};
static_assert_fit_cacheline(kF::UI::Internal::TraverseContext);