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
        Point vertRotationOrigin;
        Point vertRotationCosSin;
    };

    /** @brief Quadratic bezier pipeline name */
    constexpr GraphicPipelineName QuadraticBezierGraphicPipeline = Core::Hash("QuadraticBezier");

    /** @brief Declare quadratic bezier graphic pipeline vertex */
    template<>
    struct alignas_quarter_cacheline DeclareGraphicPipelineVertexType<QuadraticBezierGraphicPipeline>
    {
        Point vertPos;
        Point vertLeft;
        Point vertControl;
        Point vertRight;
        Color vertColor;
        Color vertInnerColor;
        Pixel vertThickness;
        Pixel vertEdgeSoftness;
    };

    /** @brief Cubic bezier pipeline name */
    constexpr GraphicPipelineName CubicBezierGraphicPipeline = Core::Hash("CubicBezier");

    /** @brief Declare cubic bezier graphic pipeline vertex */
    template<>
    struct alignas_quarter_cacheline DeclareGraphicPipelineVertexType<CubicBezierGraphicPipeline>
    {
        Point vertPos;
        Point vertP0;
        Point vertP1;
        Point vertP2;
        Point vertP3;
        Color vertColor;
        Pixel vertThickness;
        Pixel vertEdgeSoftness;
        std::uint32_t _padding;
    };
}