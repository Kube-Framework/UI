/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Renderer base declarations
 */

#pragma once

#include <Kube/Core/Hash.hpp>
#include <Kube/Core/FixedString.hpp>

#include "Base.hpp"

namespace kF::UI
{
    /** @brief Name of a primitive */
    using PrimitiveName = Core::HashedName;

    /** @brief Primitive unique type tag
     *  @param Literal Primitive's unique name */
    template<Core::FixedString Literal>
    struct PrimitiveTag
    {
        /** @brief Primitive name */
        static constexpr std::string_view Name = Literal.toView();

        /** @brief Primitive hashed name */
        static constexpr PrimitiveName Hash = Core::Hash(Name);
    };

    /** @brief Primitive concept */
    template<typename Type>
    concept PrimitiveKind = std::is_trivially_destructible_v<Type> && Core::IsTag<Type, PrimitiveTag>;


    /** @brief Name of a graphic pipeline */
    using GraphicPipelineName = Core::HashedName;

    /** @brief Graphic pipeline vertex type factory */
    template<GraphicPipelineName Name>
    struct DeclareGraphicPipelineVertexType { static_assert(Name & !Name, "DeclareGraphicPipelineVertexType: Not implemented"); };


    /** @brief Filled quad pipeline name */
    constexpr GraphicPipelineName FilledQuadGraphicPipeline = Core::Hash("FilledQuad");

    /** @brief Declare filled quad graphic pipeline vertex */
    template<>
    struct alignas_quarter_cacheline DeclareGraphicPipelineVertexType<FilledQuadGraphicPipeline>
    {
        Point vertPos;
        Point vertCenter;
        Size vertHalfSize;
        Point vertUV;
        Radius vertRadius;
        std::uint32_t vertSpriteIndex;
        Color vertColor;
        Color vertBorderColor;
        Pixel vertBorderWidth;
        Pixel vertEdgeSoftness;
        std::uint32_t padding;
        Point vertRotationCosSin;
    };
}