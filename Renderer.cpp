/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Renderer
 */

#include <Kube/GPU/GPU.hpp>
#include <Kube/GPU/DescriptorSetUpdate.hpp>
#include <Kube/IO/File.hpp>
#include <Kube/UI/UISystem.hpp>

#include "Renderer.hpp"

using namespace kF;

UI::Renderer::~Renderer(void) noexcept
{
    _perFrameCache.release();
    _primitiveCaches.clear();
}

UI::Renderer::Renderer(UISystem &uiSystem) noexcept
    :   _uiSystem(&uiSystem),
        _cache([this] {
        using namespace GPU;
        Cache cache {
            // General
            .minAlignment = [] {
                const auto &limits = Parent().physicalDevice().limits();
                return static_cast<decltype(Cache::minAlignment)>(std::max({
                    limits.minMemoryMapAlignment,
                    limits.minTexelBufferOffsetAlignment,
                    limits.minUniformBufferOffsetAlignment,
                    limits.minStorageBufferOffsetAlignment
                }));
            }(),
            .maxDispatchCount = Parent().physicalDevice().limits().maxComputeWorkGroupCount[0],
            // Compute
            .computeSetLayout = DescriptorSetLayout::Make(
                DescriptorSetLayoutCreateFlags::None,
                {
                    DescriptorSetLayoutBinding(0, DescriptorType::StorageBuffer, 1, Core::MakeFlags(ShaderStageFlags::Compute, ShaderStageFlags::Fragment)), // Context
                    DescriptorSetLayoutBinding(1, DescriptorType::StorageBufferDynamic, 1, ShaderStageFlags::Compute), // Instances
                    DescriptorSetLayoutBinding(2, DescriptorType::StorageBufferDynamic, 1, ShaderStageFlags::Compute), // Offsets
                    DescriptorSetLayoutBinding(3, DescriptorType::StorageBuffer, 1, ShaderStageFlags::Compute), // Vertices
                    DescriptorSetLayoutBinding(4, DescriptorType::StorageBuffer, 1, ShaderStageFlags::Compute) // Indices
                }
            ),
            .computePipelineLayout = PipelineLayout::Make({ cache.computeSetLayout, _uiSystem->spriteManager().descriptorSetLayout() }),
            .graphicPipelineLayout = PipelineLayout::Make({ cache.computeSetLayout, _uiSystem->spriteManager().descriptorSetLayout() })
        };
        return cache;
    }())
{
    using namespace GPU;

    // Setup per frame descriptor pools
    _perFrameCache.resize(parent().frameCount(), [this] {
        FrameCache cache {
            .commandPool = CommandPool(QueueType::Graphics, CommandPoolCreateFlags::Transient),
            .computeSetPool = DescriptorPool::Make(DescriptorPoolCreateFlags::None, 1, {
                DescriptorPoolSize(DescriptorType::StorageBuffer, 3),
                DescriptorPoolSize(DescriptorType::StorageBufferDynamic, 2)
            }),
            .computeSet = cache.computeSetPool.allocate(_cache.computeSetLayout),
            .computeCommand = cache.commandPool.add(CommandLevel::Secondary),
            .primaryCommand = cache.commandPool.add(CommandLevel::Primary)
        };
        return cache;
    });

    // Make sure that we set the current frame when acquired
    parent().frameAcquiredDispatcher().add([this](const FrameIndex frameIndex) {
        _perFrameCache.setCurrentFrame(frameIndex);
    });

    // Make sure that we set the current frame when acquired
    parent().viewSizeDispatcher().add([this] {
        for (auto index = 0u, count = _cache.graphicPipelines.size(); index != count; ++index)
            _cache.graphicPipelines.at(index).instance = createGraphicPipeline(_cache.graphicPipelineLayout, _cache.graphicPipelineModels.at(index));
    });

    // Register custom pipelines
    registerFilledQuadPipeline();
    registerQuadraticBezierPipeline();
    registerCubicBezierPipeline();
}

void UI::Renderer::registerGraphicPipeline(const GraphicPipelineRendererModel &model) noexcept
{
    kFEnsure(_cache.graphicPipelines.find([name = model.name](const auto &pair) { return pair.name == name; }) == _cache.graphicPipelines.end(),
        "UI::Renderer::registerGraphicPipeline: Graphic pipeline already registered");
    _cache.graphicPipelines.push(GraphicPipelinePair {
        .name = model.name,
        .instance = createGraphicPipeline(_cache.graphicPipelineLayout, model)
    });
    _cache.graphicPipelineModels.push(model);
}

GPU::Pipeline UI::Renderer::createGraphicPipeline(const GPU::PipelineLayoutHandle pipelineLayout, const GraphicPipelineRendererModel &model) const noexcept
{
    using namespace GPU;

    const auto extent = Parent().swapchain().extent();
    const std::uint32_t maxSpriteCount = _uiSystem->spriteManager().maxSpriteCount();
    const SpecializationMapEntry framgentSpecializationMapEntry(0u, 0u, sizeof(std::uint32_t));
    const SpecializationInfo fragmentSpecializationInfo(
        &framgentSpecializationMapEntry, &framgentSpecializationMapEntry + 1,
        &maxSpriteCount, &maxSpriteCount + 1
    );
    const Shader vertexShader(IO::File(model.vertexShader).queryResource(), model.vertexShader);
    const Shader fragmentShader(IO::File(model.fragmentShader).queryResource(), model.fragmentShader);
    ShaderStageModel shaderStageModels[3] {
        ShaderStageModel(ShaderStageFlags::Vertex, vertexShader),
        ShaderStageModel(ShaderStageFlags::Fragment, fragmentShader, &fragmentSpecializationInfo),
        ShaderStageModel(ShaderStageFlags::Geometry, NullHandle)
    };
    if (!model.geometryShader.empty())
        shaderStageModels[2].module = Shader(IO::File(model.geometryShader).queryResource(), model.geometryShader);
    const VertexInputBinding vertexInputBindings[] {
        model.vertexInputBinding
    };
    const Viewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    const Rect2D scissor {
        Offset2D { 0, 0 },
        Extent2D { extent }
    };
    const ColorBlendAttachment colorBlendAttachments[] {
        ColorBlendAttachment(
            true,
            BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha, BlendOp::Add,
            BlendFactor::One, BlendFactor::Zero, BlendOp::Add
        )
    };
    const DynamicState dynamicStates[] { DynamicState::Scissor };

    return Pipeline(
        GraphicPipelineModel(
            PipelineCreateFlags::None,
            std::begin(shaderStageModels), std::end(shaderStageModels) - model.geometryShader.empty(),
            VertexInputModel(
                std::begin(vertexInputBindings), std::end(vertexInputBindings),
                std::begin(model.vertexInputAttributes), std::end(model.vertexInputAttributes)
            ),
            model.inputAssemblyModel,
            TessellationModel(),
            ViewportModel(&viewport, &viewport + 1, &scissor, &scissor + 1),
            model.rasterizationModel,
            MultisampleModel(),
            DepthStencilModel(),
            ColorBlendModel(std::begin(colorBlendAttachments), std::end(colorBlendAttachments)),
            DynamicStateModel(std::begin(dynamicStates), std::end(dynamicStates)),
            pipelineLayout,
            Parent().renderPassManager().renderPassAt(RenderPassIndex),
            GraphicSubpassIndex
        )
    );
}

void UI::Renderer::registerPrimitive(const PrimitiveName name, const GraphicPipelineName graphicPipelineName, const QueryModelSignature queryModel) noexcept
{
    using namespace GPU;

    // Ensure primtive's graphic pipeline is registered
    kFEnsure(_cache.graphicPipelines.find([name = graphicPipelineName](const auto &pair) { return pair.name == name; }) != _cache.graphicPipelines.end(),
        "UI::Renderer::registerPrimitive: Primitive's graphic pipeline is not registered");

    // Push constant specialization
    const std::uint32_t maxSpriteCount = _uiSystem->spriteManager().maxSpriteCount();
    const SpecializationMapEntry computeSpecializationMapEntry(0u, 0u, sizeof(std::uint32_t));
    const SpecializationInfo computeSpecializationInfo(
        &computeSpecializationMapEntry, &computeSpecializationMapEntry + 1,
        &maxSpriteCount, &maxSpriteCount + 1
    );

    // Create primitive cache
    PrimitiveCache cache {
        .model = queryModel(),
        .computePipeline = Pipeline(ComputePipelineModel(
            PipelineCreateFlags::DispatchBase,
            ShaderStageModel(ShaderStageFlags::Compute, cache.model.computeShader, &computeSpecializationInfo),
            _cache.computePipelineLayout
        )),
        .name = name
    };

    // Register primitive inside painter
    _painter.registerPrimitive(name, cache.model);

    // Register primitive inside renderer
    _primitiveCaches.push(std::move(cache));
}

bool UI::Renderer::prepare(void) noexcept
{
    using namespace GPU;

    // If the painter has no vertex, cancel rendering preparation
    if (!_painter.vertexByteCount())
        return false;

    // Compute all sections' sizes
    const auto contextSectionSize = Core::AlignPowerOf2(
        static_cast<std::uint32_t>(sizeof(PrimitiveContext)), _cache.minAlignment
    );
    const auto instancesSectionSize = computeDynamicOffsets();
    const auto verticesSectionSize = Core::AlignPowerOf2(
        static_cast<std::uint32_t>(sizeof(DeclareGraphicPipelineVertexType<FilledQuadGraphicPipeline>)) * _painter.vertexByteCount(), _cache.minAlignment
    );
    const auto indicesSectionSize = Core::AlignPowerOf2(
        static_cast<std::uint32_t>(sizeof(PrimitiveIndex)) * _painter.indexCount(), _cache.minAlignment
    );
    FrameCache &frameCache = _perFrameCache.current();

    // Store offsets into frame buffers cache
    frameCache.buffers.instancesOffset = contextSectionSize;
    frameCache.buffers.verticesOffset = frameCache.buffers.instancesOffset + instancesSectionSize;
    frameCache.buffers.indicesOffset = frameCache.buffers.verticesOffset + verticesSectionSize;

    // Reserve staging memory if necessary
    const auto totalStagingSize = frameCache.buffers.verticesOffset;
    if (frameCache.buffers.stagingCapacity < totalStagingSize) {
        frameCache.buffers.stagingBuffer = Buffer::MakeStaging(totalStagingSize);
        frameCache.buffers.stagingAllocation = MemoryAllocation::MakeStaging(frameCache.buffers.stagingBuffer);
        frameCache.buffers.stagingCapacity = totalStagingSize;
    }
    frameCache.buffers.stagingSize = totalStagingSize;

    // Reserve device memory if necessary
    const auto totalDeviceSize = frameCache.buffers.indicesOffset + indicesSectionSize;
    if (frameCache.buffers.deviceCapacity < totalDeviceSize) {
        frameCache.buffers.deviceBuffer = Buffer::MakeExclusive(
            totalDeviceSize,
            Core::MakeFlags(BufferUsageFlags::TransferDst, BufferUsageFlags::StorageBuffer, BufferUsageFlags::VertexBuffer, BufferUsageFlags::IndexBuffer)
        );
        frameCache.buffers.deviceAllocation = MemoryAllocation::MakeLocal(frameCache.buffers.deviceBuffer);
        frameCache.buffers.deviceCapacity = totalDeviceSize;
    }

    // Write descriptors
    const DescriptorBufferInfo bufferInfos[] {
        DescriptorBufferInfo(frameCache.buffers.deviceBuffer, 0u, contextSectionSize),
        DescriptorBufferInfo(frameCache.buffers.deviceBuffer, contextSectionSize, instancesSectionSize),
        DescriptorBufferInfo(frameCache.buffers.deviceBuffer, contextSectionSize, instancesSectionSize),
        DescriptorBufferInfo(frameCache.buffers.deviceBuffer, totalStagingSize, verticesSectionSize),
        DescriptorBufferInfo(frameCache.buffers.deviceBuffer, totalStagingSize + verticesSectionSize, indicesSectionSize)
    };

    DescriptorSetUpdate::UpdateWrite({
        DescriptorSetWriteModel(
            frameCache.computeSet, 0, 0, DescriptorType::StorageBuffer,
            bufferInfos + 0, bufferInfos + 1
        ),
        DescriptorSetWriteModel(
            frameCache.computeSet, 1, 0, DescriptorType::StorageBufferDynamic,
            bufferInfos + 1, bufferInfos + 2
        ),
        DescriptorSetWriteModel(
            frameCache.computeSet, 2, 0, DescriptorType::StorageBufferDynamic,
            bufferInfos + 2, bufferInfos + 3
        ),
        DescriptorSetWriteModel(
            frameCache.computeSet, 3, 0, DescriptorType::StorageBuffer,
            bufferInfos + 3, bufferInfos + 4
        ),
        DescriptorSetWriteModel(
            frameCache.computeSet, 4, 0, DescriptorType::StorageBuffer,
            bufferInfos + 4, bufferInfos + 5
        )
    });

    return true;
}

std::uint32_t UI::Renderer::computeDynamicOffsets(void) noexcept
{
    const auto alignment = _cache.minAlignment;
    std::uint32_t dynamicOffset { 0u };

    // Compute primitive-specific section
    for (std::uint32_t primitiveIndex { 0u }; const auto &queue : _painter.queues()) {
        auto &primitiveCache = _primitiveCaches[primitiveIndex++];

        // Store instance count in primitive cache
        primitiveCache.instanceCount = queue.size;

        // If the queue is empty, ignore the primitive
        if (!queue.size) [[unlikely]] {
            primitiveCache.instancesDynamicOffset = 0u;
            primitiveCache.offsetsDynamicOffset = 0u;
            continue;
        }

        // Determine the dynamic offsets of instances section
        primitiveCache.instancesDynamicOffset = dynamicOffset;

        // Determine the dynamic offsets of offset section
        primitiveCache.offsetsDynamicOffset = Core::AlignPowerOf2(
            primitiveCache.instancesDynamicOffset + queue.instancesByteSize(), alignment
        );
        dynamicOffset = Core::AlignPowerOf2(
            primitiveCache.offsetsDynamicOffset + queue.offsetsByteSize(), alignment
        );
    }
    return dynamicOffset;
}

void UI::Renderer::transferPrimitives(void) noexcept
{
    auto &frameCache = _perFrameCache.current();

    // Begin memory map
    const auto mappedMemory = frameCache.buffers.stagingAllocation.beginMemoryMap<std::uint8_t>();

    // Write compute context
    const auto extent = parent().swapchain().extent();
    const auto windowSize = Size {
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height)
    };
    new (mappedMemory) PrimitiveContext {
        .windowSize = windowSize,
        .halfWindowSize = windowSize / 2.0f
    };

    // Transfer all primitives
    const auto mappedInstances = mappedMemory + frameCache.buffers.instancesOffset;
    for (std::uint32_t primitiveIndex { 0u }; const auto &queue : _painter.queues()) {
        auto &primitiveCache = _primitiveCaches[primitiveIndex++];

        // If the queue is empty, ignore the primitive
        if (!queue.size) [[unlikely]]
            continue;

        // Transfer instances & offsets sections
        std::memcpy(
            mappedInstances + primitiveCache.instancesDynamicOffset,
            queue.data,
            queue.instancesByteSize()
        );
        std::memcpy(
            mappedInstances + primitiveCache.offsetsDynamicOffset,
            queue.offsets(),
            queue.offsetsByteSize()
        );
    }

    // End memory map
    frameCache.buffers.stagingAllocation.endMemoryMap();
}

void UI::Renderer::batchPrimitives(void) noexcept
{
    using namespace GPU;

    FrameCache &frameCache = _perFrameCache.current();

    // Record compute command
    frameCache.commandPool.reset();
    frameCache.commandPool.record(
        frameCache.computeCommand,
        CommandBufferUsageFlags::OneTimeSubmit,
        CommandInheritanceInfo(),
        [this](const auto &recorder) { recordComputeCommand(recorder); }
    );
}

void UI::Renderer::recordComputeCommand(const GPU::CommandRecorder &recorder) noexcept
{
    using namespace GPU;

    const auto &frameCache = _perFrameCache.current();

    // Dispatch each primitive pipeline
    for (const PrimitiveCache &primitiveCache : _primitiveCaches) {
        if (primitiveCache.instancesDynamicOffset == primitiveCache.offsetsDynamicOffset)
            continue;

        // Bind compute pipeline & descriptor sets
        recorder.bindPipeline(PipelineBindPoint::Compute, primitiveCache.computePipeline);
        const std::uint32_t dynamicOffsets[] { primitiveCache.instancesDynamicOffset, primitiveCache.offsetsDynamicOffset };
        const DescriptorSetHandle sets[] { frameCache.computeSet, _uiSystem->spriteManager().descriptorSet() };
        recorder.bindDescriptorSets(
            PipelineBindPoint::Compute, _cache.computePipelineLayout,
            0, std::begin(sets), std::end(sets), std::begin(dynamicOffsets), std::end(dynamicOffsets)
        );

        // Dispatch local groups of 'localGroupSize' units with a maximum single dispatch count of 'maxDispatchCount'
        const auto instanceCount = primitiveCache.instanceCount;
        const auto maxDispatchCount = _cache.maxDispatchCount;
        const auto localGroupSize = primitiveCache.model.computeLocalGroupSize;
        const auto totalDispatchCount = (instanceCount / localGroupSize) + (static_cast<bool>(instanceCount % localGroupSize));
        for (auto left = totalDispatchCount, dispatchBase = 0u; left; dispatchBase = totalDispatchCount - left) {
            std::uint32_t dispatchCount;
            if (left >= maxDispatchCount) {
                dispatchCount = maxDispatchCount;
                left -= maxDispatchCount;
            } else {
                dispatchCount = left;
                left = 0u;
            }
            recorder.dispatchBase(dispatchBase, dispatchCount);
        }
    }
}

void UI::Renderer::dispatch(const bool isInvalidated) noexcept
{
    using namespace GPU;

    FrameCache &frameCache = _perFrameCache.current();

    // If no memory is mapped then no compute command has been recorded, we has to reset command pool
    if (!isInvalidated) [[likely]]
        frameCache.commandPool.reset();

    // Record primary command
    frameCache.commandPool.record(
        frameCache.primaryCommand,
        CommandBufferUsageFlags::OneTimeSubmit,
        [this, isInvalidated](const auto &recorder) { recordPrimaryCommand(recorder, isInvalidated); }
    );

    // Reset the frame fence
    frameCache.frameFence.reset();

    // Submit primary command
    auto &gpu = parent();
    gpu.commandDispatcher().dispatch(
        QueueType::Graphics,
        { frameCache.primaryCommand },
        { gpu.commandDispatcher().currentFrameAvailableSemaphore() },
        { PipelineStageFlags::FragmentShader },
        { frameCache.frameSemaphore },
        frameCache.frameFence
    );

    // Add frame dependencies
    gpu.commandDispatcher().addPresentDependencies(QueueType::Graphics, frameCache.frameSemaphore, frameCache.frameFence);
}

void UI::Renderer::recordPrimaryCommand(const GPU::CommandRecorder &recorder, const bool isInvalidated) noexcept
{
    using namespace GPU;

    FrameCache &frameCache = _perFrameCache.current();
    auto &gpu = parent();
    const auto extent = gpu.swapchain().extent();

    // Block all compute pipelines until transfer ended
    recorder.pipelineBarrier(
        PipelineStageFlags::AllCommands,
        PipelineStageFlags::AllCommands,
        DependencyFlags::None,
        MemoryBarrier(AccessFlags::None, AccessFlags::None)
    );

    // Check if frame has been recomputed by checking if memory was mapped
    if (isInvalidated) [[likely]] {
        // Transfer memory
        recorder.copyBuffer(frameCache.buffers.stagingBuffer, frameCache.buffers.deviceBuffer, BufferCopy(frameCache.buffers.stagingSize));

        // Block all compute pipelines until transfer ended
        recorder.pipelineBarrier(
            PipelineStageFlags::Transfer,
            PipelineStageFlags::ComputeShader,
            DependencyFlags::None,
            MemoryBarrier(AccessFlags::TransferWrite, AccessFlags::ShaderRead)
        );

        // Execute compute command
        recorder.executeCommand(frameCache.computeCommand);

        // Block all graphic pipelines until compute pipelines ended
        recorder.pipelineBarrier(
            PipelineStageFlags::ComputeShader,
            PipelineStageFlags::VertexInput,
            DependencyFlags::None,
            MemoryBarrier(
                AccessFlags::ShaderWrite,
                Core::MakeFlags(AccessFlags::VertexAttributeRead, AccessFlags::IndexRead)
            )
        );
    }

    // Begin render pass
    recorder.beginRenderPass(
        gpu.renderPassManager().renderPassAt(RenderPassIndex),
        gpu.framebufferManager().currentFramebuffer(RenderPassIndex),
        Rect2D { .offset = VkOffset2D {}, .extent = extent },
        {
            ClearValue {
                .color = ClearColorValue {
                    static_cast<float>(_clearColor.r) / static_cast<float>(std::numeric_limits<std::uint8_t>::max()),
                    static_cast<float>(_clearColor.g) / static_cast<float>(std::numeric_limits<std::uint8_t>::max()),
                    static_cast<float>(_clearColor.b) / static_cast<float>(std::numeric_limits<std::uint8_t>::max()),
                    static_cast<float>(_clearColor.a) / static_cast<float>(std::numeric_limits<std::uint8_t>::max())
                }
            }
        },
        SubpassContents::Inline
    );

    // Utility to convert from clip Area to scissor Rect2D
    const auto toScissor = [extent](const auto &area) {
        Rect2D rect;
        if (area == DefaultClip) {
            return Rect2D(Offset2D(), extent);
        } else {
            const auto clippedArea = Area::ApplyClip(area, UI::Area(UI::Point(), UI::Size(UI::Pixel(extent.width), UI::Pixel(extent.height))));
            return Rect2D(
                Offset2D(std::int32_t(clippedArea.pos.x), std::int32_t(clippedArea.pos.y)),
                Extent2D(std::uint32_t(clippedArea.size.width), std::uint32_t(clippedArea.size.height))
            );
        }
        return rect;
    };

    // Loop over each clip and draw all vertices between them
    auto clip = _painter.clips().begin();
    const auto clipEnd = _painter.clips().end();
    auto pipeline = _painter.pipelines().begin();
    const auto pipelineEnd = _painter.pipelines().end();
    const auto indexCount = _painter.indexCount();
    std::uint32_t indexOffset {};
    auto lastScissor = toScissor(DefaultClip);

    for (; pipeline != pipelineEnd; ++pipeline) {
        { // Prepare pipeline
            const auto targetPipeline = _cache.graphicPipelines.find([name = pipeline->name](const auto &pair) { return pair.name == name; });
            recorder.bindPipeline(PipelineBindPoint::Graphics, targetPipeline->instance);
            recorder.bindVertexBuffer(0, frameCache.buffers.deviceBuffer, frameCache.buffers.verticesOffset);
            recorder.bindIndexBuffer(frameCache.buffers.deviceBuffer, IndexType::Uint32, frameCache.buffers.indicesOffset);
            const DescriptorSetHandle sets[] { frameCache.computeSet, _uiSystem->spriteManager().descriptorSet() };
            const std::uint32_t dynamicOffsets[] { 0u, 0u };
            recorder.bindDescriptorSets(
                PipelineBindPoint::Graphics, _cache.graphicPipelineLayout,
                0, std::begin(sets), std::end(sets), std::begin(dynamicOffsets), std::end(dynamicOffsets)
            );
            recorder.setScissor(lastScissor);
        }
        // Exhaust pipeline
        const auto nextPipeline = pipeline + 1;
        const auto nextPipelineOffset = nextPipeline != pipelineEnd ? nextPipeline->indexOffset : indexCount;
        while (indexOffset != nextPipelineOffset) {
            const bool nextClipAvailable = clip != clipEnd;
            const auto pipelineMaxDrawCount = nextPipelineOffset - indexOffset;
            // If clip list is exhausted, draw from current offset to pipeline end, else draw to next clip
            const auto drawCount
                = nextClipAvailable
                ? std::min(clip->indexOffset - indexOffset, pipelineMaxDrawCount)
                : pipelineMaxDrawCount;
            // Draw and increment offset
            if (drawCount) [[likely]] {
                recorder.drawIndexed(drawCount, 1, indexOffset);
                indexOffset += drawCount;
            }
            // Set next scissor clip
            if (nextClipAvailable) [[likely]] {
                lastScissor = toScissor(clip->area);
                recorder.setScissor(lastScissor);
                ++clip;
            }
        }
    }

    // End render pass
    recorder.endRenderPass();
}

void UI::Renderer::registerFilledQuadPipeline(void) noexcept
{
    using namespace GPU;
    using Vertex = DeclareGraphicPipelineVertexType<FilledQuadGraphicPipeline>;

    registerGraphicPipeline(GraphicPipelineRendererModel {
        .name = FilledQuadGraphicPipeline,
        .vertexShader = ":/UI/Shaders/FilledQuad.vert.spv",
        .fragmentShader = ":/UI/Shaders/FilledQuad.frag.spv",
        .vertexInputBinding = VertexInputBinding(0, sizeof(Vertex), VertexInputRate::Vertex),
        .vertexInputAttributes = {
            VertexInputAttribute(0, 0,  Format::R32G32_SFLOAT,          offsetof(Vertex, vertPos)),
            VertexInputAttribute(0, 1,  Format::R32G32_SFLOAT,          offsetof(Vertex, vertCenter)),
            VertexInputAttribute(0, 2,  Format::R32G32_SFLOAT,          offsetof(Vertex, vertHalfSize)),
            VertexInputAttribute(0, 3,  Format::R32G32_SFLOAT,          offsetof(Vertex, vertUV)),
            VertexInputAttribute(0, 4,  Format::R32G32B32A32_SFLOAT,    offsetof(Vertex, vertRadius)),
            VertexInputAttribute(0, 5,  Format::R32_UINT,               offsetof(Vertex, vertSpriteIndex)),
            VertexInputAttribute(0, 6,  Format::R32_UINT,               offsetof(Vertex, vertColor)),
            VertexInputAttribute(0, 7,  Format::R32_UINT,               offsetof(Vertex, vertBorderColor)),
            VertexInputAttribute(0, 8,  Format::R32_SFLOAT,             offsetof(Vertex, vertBorderWidth)),
            VertexInputAttribute(0, 9,  Format::R32_SFLOAT,             offsetof(Vertex, vertEdgeSoftness)),
            VertexInputAttribute(0, 10, Format::R32G32_SFLOAT,          offsetof(Vertex, vertRotationCosSin))
        },
        .inputAssemblyModel = InputAssemblyModel(PrimitiveTopology::TriangleList),
        .rasterizationModel = RasterizationModel(PolygonMode::Fill)
    });
}

void UI::Renderer::registerQuadraticBezierPipeline(void) noexcept
{
    using namespace GPU;
    using Vertex = DeclareGraphicPipelineVertexType<QuadraticBezierGraphicPipeline>;

    registerGraphicPipeline(GraphicPipelineRendererModel {
        .name = QuadraticBezierGraphicPipeline,
        .vertexShader = ":/UI/Shaders/QuadraticBezier.vert.spv",
        .fragmentShader = ":/UI/Shaders/QuadraticBezier.frag.spv",
        .vertexInputBinding = VertexInputBinding(0, sizeof(Vertex), VertexInputRate::Vertex),
        .vertexInputAttributes = {
            VertexInputAttribute(0, 0,  Format::R32G32_SFLOAT,  offsetof(Vertex, vertPos)),
            VertexInputAttribute(0, 1,  Format::R32G32_SFLOAT,  offsetof(Vertex, vertLeft)),
            VertexInputAttribute(0, 2,  Format::R32G32_SFLOAT,  offsetof(Vertex, vertControl)),
            VertexInputAttribute(0, 3,  Format::R32G32_SFLOAT,  offsetof(Vertex, vertRight)),
            VertexInputAttribute(0, 4,  Format::R32_UINT,       offsetof(Vertex, vertColor)),
            VertexInputAttribute(0, 5,  Format::R32_UINT,       offsetof(Vertex, vertInnerColor)),
            VertexInputAttribute(0, 6,  Format::R32_SFLOAT,     offsetof(Vertex, vertThickness)),
            VertexInputAttribute(0, 7,  Format::R32_SFLOAT,     offsetof(Vertex, vertEdgeSoftness))
        },
        .inputAssemblyModel = InputAssemblyModel(PrimitiveTopology::TriangleList),
        .rasterizationModel = RasterizationModel(PolygonMode::Fill)
    });
}

void UI::Renderer::registerCubicBezierPipeline(void) noexcept
{
    using namespace GPU;
    using Vertex = DeclareGraphicPipelineVertexType<CubicBezierGraphicPipeline>;

    registerGraphicPipeline(GraphicPipelineRendererModel {
        .name = CubicBezierGraphicPipeline,
        .vertexShader = ":/UI/Shaders/CubicBezier.vert.spv",
        .fragmentShader = ":/UI/Shaders/CubicBezier.frag.spv",
        .vertexInputBinding = VertexInputBinding(0, sizeof(Vertex), VertexInputRate::Vertex),
        .vertexInputAttributes = {
            VertexInputAttribute(0, 0,  Format::R32G32_SFLOAT,  offsetof(Vertex, vertPos)),
            VertexInputAttribute(0, 1,  Format::R32G32_SFLOAT,  offsetof(Vertex, vertP0)),
            VertexInputAttribute(0, 2,  Format::R32G32_SFLOAT,  offsetof(Vertex, vertP1)),
            VertexInputAttribute(0, 3,  Format::R32G32_SFLOAT,  offsetof(Vertex, vertP2)),
            VertexInputAttribute(0, 4,  Format::R32G32_SFLOAT,  offsetof(Vertex, vertP3)),
            VertexInputAttribute(0, 5,  Format::R32_UINT,       offsetof(Vertex, vertColor)),
            VertexInputAttribute(0, 6,  Format::R32_SFLOAT,     offsetof(Vertex, vertThickness)),
            VertexInputAttribute(0, 7,  Format::R32_SFLOAT,     offsetof(Vertex, vertEdgeSoftness))
        },
        .inputAssemblyModel = InputAssemblyModel(PrimitiveTopology::TriangleList),
        .rasterizationModel = RasterizationModel(PolygonMode::Fill)
    });
}