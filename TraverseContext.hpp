/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System Traverse Context
 */

#pragma once

#include <Kube/Core/Vector.hpp>
#include <Kube/Core/FlatVector.hpp>

#include "Components.hpp"

namespace kF::UI::Internal
{
    class TraverseContext;
}

/** @brief Traversal context */
class alignas_double_cacheline kF::UI::Internal::TraverseContext
{
public:
    /** @brief Data used to resolve constraints */
    struct alignas_double_cacheline ResolveData
    {
        /** @brief A list of entity indexes */
        using Children = Core::SmallVector<ECS::EntityIndex, (Core::CacheLineSize - Core::CacheLineQuarterSize) / sizeof(ECS::EntityIndex), UIAllocator>;

        // Cacheline 0
        const TreeNode *node {};
        Constraints *constraints {};
        const Layout *layout {};
        Size totalFixed {};
        Size maxFixed {};
        Size fillCount {};
        Size unresolvedCount {};
        Size fillSize {};
        // Cacheline 1
        Children children {};
    };
    static_assert_fit_double_cacheline(ResolveData);

    /** @brief Get context entity */
    [[nodiscard]] inline ECS::Entity entity(void) const noexcept { return _entity; }

    /** @brief Get context entity index */
    [[nodiscard]] inline ECS::EntityIndex entityIndex(void) const noexcept { return _entityIndex; }

    /** @brief Get the entity of an entity */
    [[nodiscard]] inline ECS::Entity entityAt(const ECS::EntityIndex entityIndex) noexcept { return _entityBegin[entityIndex]; }

    /** @brief Get the entity index of an entity */
    [[nodiscard]] inline ECS::EntityIndex entityIndexOf(const ECS::Entity entity) const noexcept
        { return Core::Distance<ECS::EntityIndex>(_entityBegin, std::find(_entityBegin, _entityBegin + _constraints.size(), entity)); }

    /** @brief Get the entity index of an entity using its node */
    [[nodiscard]] inline ECS::EntityIndex entityIndexOf(const TreeNode &node) const noexcept
        { return Core::Distance<ECS::EntityIndex>(_nodeBegin, &node); }

    /** @brief Get the constraints of an entity */
    [[nodiscard]] inline Constraints &constraintsAt(const ECS::EntityIndex entityIndex) noexcept { return _constraints.at(entityIndex); }
    [[nodiscard]] inline Constraints &constraints(void) noexcept { return constraintsAt(_entityIndex); }

    /** @brief Get the resolveData of an entity */
    [[nodiscard]] inline ResolveData &resolveDataAt(const ECS::EntityIndex entityIndex) noexcept { return _resolveDatas.at(entityIndex); }
    [[nodiscard]] inline ResolveData &resolveData(void) noexcept { return resolveDataAt(_entityIndex); }

    /** @brief Get the node of an entity */
    [[nodiscard]] inline const TreeNode &nodeAt(const ECS::EntityIndex entityIndex) noexcept { return _nodeBegin[entityIndex]; }
    [[nodiscard]] inline const TreeNode &node(void) noexcept { return nodeAt(_entityIndex); }

    /** @brief Get the area of an entity */
    [[nodiscard]] inline Area &areaAt(const ECS::EntityIndex entityIndex) noexcept { return _areaBegin[entityIndex]; }
    [[nodiscard]] inline Area &area(void) noexcept { return areaAt(_entityIndex); }

    /** @brief Get the depth of an entity */
    [[nodiscard]] inline Depth &depthAt(const ECS::EntityIndex entityIndex) noexcept { return _depthBegin[entityIndex]; }
    [[nodiscard]] inline Depth &depth(void) noexcept { return depthAt(_entityIndex); }


    /** @brief Setup initital context for traversal */
    void setupContext(
        const std::uint32_t count,
        const ECS::Entity * const entityBegin,
        const TreeNode * const nodeBegin,
        Area * const areaBegin,
        Depth * const depthBegin
    ) noexcept;

    /** @brief Setup the next entity for traversal recursion */
    inline void setupEntity(const ECS::Entity entity, const ECS::EntityIndex entityIndex) noexcept
        { _entity = entity; _entityIndex = entityIndex; }


    /** @brief Get clip area range */
    [[nodiscard]] inline Core::IteratorRange<const Area *> clipAreas(void) const noexcept
        { return _clipAreas.toRange(); }

    /** @brief Get clip depths range */
    [[nodiscard]] inline Core::IteratorRange<const DepthUnit *> clipDepths(void) const noexcept
        { return _clipDepths.toRange(); }

    /** @brief Push a clip into the clip list */
    void setClip(const Area &area, const DepthUnit depth) noexcept
        { _clipAreas.push(area); _clipDepths.push(depth); }

    /** @brief Push a clip into the clip list */
    [[nodiscard]] Area currentClip(void) const noexcept
        { return _clipAreas.empty() ? DefaultClip : _clipAreas.back(); }

private:
    /** @brief Constraints cache */
    using ConstraintsCache = Core::Vector<Constraints, UIAllocator>;

    /** @brief Resolve data cache */
    using ResolveDatas = Core::FlatVector<ResolveData, UIAllocator>;

    /** @brief Clip areas */
    using ClipAreas = Core::Vector<Area, UIAllocator>;

    /** @brief Clip depths */
    using ClipDepths = Core::SmallVector<DepthUnit, Core::CacheLineHalfSize / sizeof(DepthUnit), UIAllocator>;

    // Cacheline 0
    ConstraintsCache _constraints {};
    ResolveDatas _resolveDatas {};
    ECS::Entity _entity {};
    ECS::EntityIndex _entityIndex {};
    const ECS::Entity *_entityBegin {};
    const TreeNode *_nodeBegin {};
    Area *_areaBegin {};
    Depth *_depthBegin {};
    // Cacheline 1
    alignas_quarter_cacheline ClipAreas _clipAreas {};
    ClipDepths _clipDepths {};
};
static_assert_fit_double_cacheline(kF::UI::Internal::TraverseContext);