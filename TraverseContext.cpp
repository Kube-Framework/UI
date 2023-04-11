/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI System Traverse Context
 */

#include "TraverseContext.hpp"

using namespace kF;

void UI::Internal::TraverseContext::setupContext(
    const std::uint32_t count,
    const ECS::Entity * const entityBegin,
    const TreeNode * const nodeBegin,
    Area * const areaBegin,
    Depth * const depthBegin
) noexcept
{
    _constraints.resize(count);
    _resolveDatas.resize(count);
    _entityBegin = entityBegin;
    _nodeBegin = nodeBegin;
    _areaBegin = areaBegin;
    _depthBegin = depthBegin;
    _clipAreas.clear();
    _clipDepths.clear();
}