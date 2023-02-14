/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI primitive processor
 */

#pragma once

#include <Kube/Core/Abort.hpp>

#include <Kube/GPU/Shader.hpp>

#include "RendererBase.hpp"

namespace kF::UI
{
    /** @brief Describes a primitive processor model */
    struct alignas_half_cacheline PrimitiveProcessorModel
    {
        GPU::Shader computeShader;
        std::uint32_t computeLocalGroupSize;
        std::uint32_t instanceSize;
        std::uint32_t instanceAlignment;
        std::uint32_t verticesPerInstance;
        std::uint32_t indicesPerInstance;
    };
    static_assert_fit_half_cacheline(PrimitiveProcessorModel);

    /** @brief Templated namespace allowing static primitive processors function calls */
    namespace PrimitiveProcessor
    {
        /** @brief Query the primitive pipeline
         *  @note You must specialize this function for each primitive processor */
        template<kF::UI::PrimitiveKind Primitive>
        [[nodiscard]] consteval PrimitiveName QueryGraphicPipeline(void) noexcept
            { static_assert(std::is_same_v<Primitive, void>, "PrimitiveProcessor::QueryGraphicPipeline: Not implemented"); return PrimitiveName(); }

        /** @brief Query the primitive processor model
         *  @note You must specialize this function for each primitive processor */
        template<kF::UI::PrimitiveKind Primitive>
        [[nodiscard]] inline PrimitiveProcessorModel QueryModel(void) noexcept
            { kFAbort("PrimitiveProcessor::QueryModel: Not implemented"); }

        /** @brief Get the number of instances that would be inserted from a list of primitives
         *  @note You can specialize this function for each primitive processor
         *  @note If you don't specialize, the default behavior is to return primitive count (1:1 mapping) */
        template<kF::UI::PrimitiveKind Primitive>
        [[nodiscard]] inline std::uint32_t GetInstanceCount(
                const Primitive * const primitiveBegin, const Primitive * const primitiveEnd) noexcept
            { return Core::Distance<std::uint32_t>(primitiveBegin, primitiveEnd); }

        /** @brief Insert instances from a list of primitives
         *  @note If you don't specialize, the default behavior is to copy primitive as instances (1:1 mapping)
         *  @return Returns the number of inserted instances
         *          This number can't exceed count queried from 'GetInstanceCount', if lesser then the difference is discarded */
        template<kF::UI::PrimitiveKind Primitive>
        inline std::uint32_t InsertInstances(
                const Primitive * const primitiveBegin, const Primitive * const primitiveEnd,
                std::uint8_t * const instanceBegin) noexcept
        {
            std::copy(primitiveBegin, primitiveEnd, reinterpret_cast<Primitive *>(instanceBegin));
            return Core::Distance<std::uint32_t>(primitiveBegin, primitiveEnd);
        }
    }

    // Utility functions of the primitive processor namespace
    namespace PrimitiveProcessor
    {
        /** @brief Query the vertex size of a primitive */
        template<kF::UI::PrimitiveKind Primitive>
        [[nodiscard]] consteval std::uint32_t QueryVertexSize(void) noexcept { return sizeof(DeclareGraphicPipelineVertexType<QueryGraphicPipeline<Primitive>()>); }

        /** @brief Query the vertex alignment of a primitive */
        template<kF::UI::PrimitiveKind Primitive>
        [[nodiscard]] consteval std::uint32_t QueryVertexAlignment(void) noexcept { return static_cast<std::uint32_t>(Core::CacheLineQuarterSize); }
    }
}
