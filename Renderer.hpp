/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Renderer
 */

#pragma once

#include <Kube/GPU/PerFrameCache.hpp>
#include <Kube/GPU/Fence.hpp>
#include <Kube/GPU/PipelineLayout.hpp>
#include <Kube/GPU/Pipeline.hpp>
#include <Kube/GPU/Buffer.hpp>
#include <Kube/GPU/Semaphore.hpp>
#include <Kube/GPU/MemoryAllocation.hpp>
#include <Kube/GPU/DescriptorPool.hpp>
#include <Kube/GPU/DescriptorSetLayout.hpp>
#include <Kube/GPU/CommandPool.hpp>

#include "RendererBase.hpp"
#include "Painter.hpp"

namespace kF::UI
{
    class Renderer;
    class UISystem;
}

/** @brief UI Renderer is responsible of manipulating GPU data of 2D primitives */
class alignas_double_cacheline kF::UI::Renderer : public GPU::GPUObject
{
public:
    /** @brief Descriptor of a graphic pipeline */
    struct GraphicPipelineRendererModel
    {
        Core::HashedName name {};
        std::string_view vertexShader {};
        std::string_view fragmentShader {};
        GPU::VertexInputBinding vertexInputBinding; // Only 1 binding is available
        Core::Vector<GPU::VertexInputAttribute, UIAllocator> vertexInputAttributes {};
        GPU::InputAssemblyModel inputAssemblyModel;
        GPU::RasterizationModel rasterizationModel;
    };


    /** @brief Destructor */
    ~Renderer(void) noexcept;

    /** @brief Constructor */
    Renderer(UISystem &uiSystem) noexcept;


    /** @brief Set the clear color of the renderer */
    inline void setClearColor(const Color &color) noexcept { _clearColor = color; }


    /** @brief Get painter */
    [[nodiscard]] inline Painter &painter(void) noexcept { return _painter; }
    [[nodiscard]] inline const Painter &painter(void) const noexcept { return _painter; }

    /** @brief Get current frame index */
    [[nodiscard]] inline GPU::FrameIndex currentFrame(void) const noexcept { return _perFrameCache.currentFrame(); }


    /** @brief Register a graphic pipeline from a renderer model
     *  @note Every custom pipeline share the same descriptor set layout.
     *  Fragment shader must declare sprite count constant and the sprite descriptor set
     *  -> layout(constant_id = 0) const uint MaxSpriteCount = 1;
     *  -> layout(set = 0, binding = 0) uniform sampler2D sprites[MaxSpriteCount]; */
    void registerGraphicPipeline(const GraphicPipelineRendererModel &model) noexcept;


    /** @brief Register a primitive to the Renderer and its Painter */
    template<kF::UI::PrimitiveKind Primitive>
    void registerPrimitive(void) noexcept;


    /** @brief Prepare primary draw command
     *  @return Return true if renderer has anything to draw */
    [[nodiscard]] bool prepare(void) noexcept;

    /** @brief Transfer all primitives' instances to mapped memory
     *  @note 'prepare' must has been called and returned true before this function can be called
     *  @note This function can be execute concurently with 'batchPrimitives' */
    void transferPrimitives(void) noexcept;

    /** @brief Batch primitives together in a single compute command
     *  @note 'prepare' must has been called and return true before this function can be called
     *  @note This function can be execute concurently with 'transferPrimitives' */
    void batchPrimitives(void) noexcept;

    /** @brief Dispatch primary draw command */
    inline void dispatchInvalidFrame(void) noexcept { dispatch(true); }

    /** @brief Dispatch primary draw command */
    inline void dispatchValidFrame(void) noexcept { dispatch(false); }


private:
    /** @brief Index type of vertices */
    using PrimitiveIndex = std::uint32_t;

    /** @brief Graphic pipeline key/value pair */
    struct alignas_quarter_cacheline GraphicPipelinePair
    {
        GraphicPipelineName name;
        GPU::Pipeline instance;
    };
    static_assert_fit_quarter_cacheline(GraphicPipelinePair);

    /** @brief Cache of renderer */
    struct alignas_cacheline Cache
    {
        // Cacheline 0
        //   General
        std::uint32_t minAlignment {};
        std::uint32_t maxDispatchCount {};
        //   Compute
        GPU::DescriptorSetLayout computeSetLayout;
        GPU::PipelineLayout computePipelineLayout;
        //   Graphic
        GPU::PipelineLayout graphicPipelineLayout;
        Core::Vector<GraphicPipelinePair, UIAllocator> graphicPipelines {};
        Core::Vector<GraphicPipelineRendererModel, UIAllocator> graphicPipelineModels {};
    };
    static_assert_fit_cacheline(Cache);

    /** @brief Frame GPU Cache */
    struct alignas_cacheline FrameBuffers
    {
        std::uint32_t stagingSize {};
        std::uint32_t stagingCapacity {};
        std::uint32_t deviceCapacity {};
        std::uint32_t instancesOffset {};
        std::uint32_t verticesOffset {};
        std::uint32_t indicesOffset {};
        GPU::Buffer stagingBuffer {};
        GPU::MemoryAllocation stagingAllocation {};
        GPU::Buffer deviceBuffer {};
        GPU::MemoryAllocation deviceAllocation {};
    };
    static_assert_fit_cacheline(FrameBuffers);

    /** @brief Cache of a frame */
    struct alignas_double_cacheline FrameCache
    {
        // Cacheline 0
        GPU::CommandPool commandPool;
        // Compute
        GPU::DescriptorPool computeSetPool;
        GPU::DescriptorSetHandle computeSet;
        GPU::CommandHandle computeCommand;
        // Primary
        GPU::CommandHandle primaryCommand;
        GPU::Fence frameFence {};
        GPU::Semaphore frameSemaphore {};

        // Cacheline 1
        FrameBuffers buffers {};
    };
    static_assert_fit_double_cacheline(FrameCache);

    /** @brief Cache of a primitive */
    struct alignas_cacheline PrimitiveCache
    {
        PrimitiveProcessorModel model;
        GPU::Pipeline computePipeline;
        Core::HashedName name {};
        std::uint32_t instanceCount {};
        std::uint32_t instancesDynamicOffset {};
        std::uint32_t offsetsDynamicOffset {};
    };
    static_assert_fit_cacheline(PrimitiveCache);

    /** @brief List of PrimitiveCache */
    using PrimitiveCaches = Core::Vector<PrimitiveCache, UIAllocator>;

    /** @brief Primitive context layout */
    struct PrimitiveContext
    {
        Size windowSize;
        Size halfWindowSize;
    };

    /** @brief QueryModel function signature */
    using QueryModelSignature = PrimitiveProcessorModel(*)(void) noexcept;


    /** @brief Create graphic pipeline */
    [[nodiscard]] GPU::Pipeline createGraphicPipeline(const GPU::PipelineLayoutHandle pipelineLayout, const GraphicPipelineRendererModel &model) const noexcept;


    /** @brief Opaque implementation of the registerPrimitive function */
    void registerPrimitive(const PrimitiveName name, const GraphicPipelineName graphicPipelineName, const QueryModelSignature queryModel) noexcept;


    /** @brief Dispatch primary draw command */
    void dispatch(const bool isInvalidated) noexcept;


    /** @brief Compute dynamic offsets and return aligned section size */
    [[nodiscard]] std::uint32_t computeDynamicOffsets(void) noexcept;

    /** @brief Record primary command to dispatch */
    void recordPrimaryCommand(const GPU::CommandRecorder &recorder, const bool isInvalidated) noexcept;

    /** @brief Record a single compute command with all primitive processor compute pipelines */
    void recordComputeCommand(const GPU::CommandRecorder &recorder) noexcept;


    // Cacheline 0 & 1
    Painter _painter {};
    // Cacheline 2
    UISystem *_uiSystem {};
    UI::Color _clearColor {};
    GPU::PerFrameCache<FrameCache, UIAllocator> _perFrameCache {};
    PrimitiveCaches _primitiveCaches {};
    // Cacheline 3
    Cache _cache;
};
static_assert_alignof_double_cacheline(kF::UI::Renderer);
static_assert_sizeof(kF::UI::Renderer, kF::Core::CacheLineDoubleSize * 2);

#include "Renderer.ipp"