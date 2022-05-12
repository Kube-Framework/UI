/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Present pipeline
 */

#pragma once

#include <Kube/ECS/Pipeline.hpp>

namespace kF::UI
{
    /** @brief Pipeline in charge of presentation */
    using PresentPipeline = ECS::PipelineTag<"PresentPipeline">;
}