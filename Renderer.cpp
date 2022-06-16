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
    _perFrameCaches.release();
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
                    DescriptorSetLayoutBinding(0, DescriptorType::StorageBuffer, 1, ShaderStageFlags::Compute), // Context
                    DescriptorSetLayoutBinding(1, DescriptorType::StorageBufferDynamic, 1, ShaderStageFlags::Compute), // Instances
                    DescriptorSetLayoutBinding(2, DescriptorType::StorageBufferDynamic, 1, ShaderStageFlags::Compute), // Offsets
                    DescriptorSetLayoutBinding(3, DescriptorType::StorageBuffer, 1, ShaderStageFlags::Compute), // Vertices
                    DescriptorSetLayoutBinding(4, DescriptorType::StorageBuffer, 1, ShaderStageFlags::Compute) // Indices
                }
            ),
            .computePipelineLayout = PipelineLayout::Make({ cache.computeSetLayout, _uiSystem->spriteManager().descriptorSetLayout() }),
            .graphicPipelineLayout = PipelineLayout::Make({ _uiSystem->spriteManager().descriptorSetLayout() }),
            .graphicPipeline = createGraphicPipeline(cache.graphicPipelineLayout)
        };
        return cache;
    }())
{
    using namespace GPU;

    // Setup per frame descriptor pools
    _perFrameCaches.resize(parent().frameCount(), [this] {
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
        _perFrameCaches.setCurrentFrame(frameIndex);
    });

    // Make sure that we set the current frame when acquired
    parent().viewSizeDispatcher().add([this] {
        _cache.graphicPipeline = createGraphicPipeline(_cache.graphicPipelineLayout);
    });
}

void UI::Renderer::setClearColor(const Color &color) noexcept
{
    _clearColorValue = GPU::ClearColorValue {
        static_cast<float>(color.r) / static_cast<float>(UINT8_MAX),
        static_cast<float>(color.g) / static_cast<float>(UINT8_MAX),
        static_cast<float>(color.b) / static_cast<float>(UINT8_MAX),
        static_cast<float>(color.a) / static_cast<float>(UINT8_MAX)
    };
}

void UI::Renderer::registerPrimitive(const Core::HashedName name, const QueryModelSignature queryModel) noexcept
{
    using namespace GPU;

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
        ))
    };

    // Register primitive inside painter
    _painter.registerPrimitive(name, cache.model);

    // Register primitive inside renderer
    _primitiveCaches.push(std::move(cache));
}

GPU::Pipeline UI::Renderer::createGraphicPipeline(const GPU::PipelineLayoutHandle pipelineLayout) const noexcept
{
    constexpr std::string_view PrimitiveVertexShader = ":/UI/Shaders/Primitive.vert.spv";
    constexpr std::string_view PrimitiveFragmentShader = ":/UI/Shaders/Primitive.frag.spv";

    using namespace GPU;

    const auto extent = Parent().swapchain().extent();
    const std::uint32_t maxSpriteCount = _uiSystem->spriteManager().maxSpriteCount();
    const SpecializationMapEntry framgentSpecializationMapEntry(0u, 0u, sizeof(std::uint32_t));
    const SpecializationInfo fragmentSpecializationInfo(
        &framgentSpecializationMapEntry, &framgentSpecializationMapEntry + 1,
        &maxSpriteCount, &maxSpriteCount + 1
    );
    const Shader vertexShader(IO::File(PrimitiveVertexShader).queryResource(), PrimitiveVertexShader);
    const Shader fragmentShader(IO::File(PrimitiveFragmentShader).queryResource(), PrimitiveFragmentShader);
    const ShaderStageModel shaderStageModels[] {
        ShaderStageModel(ShaderStageFlags::Vertex, vertexShader),
        ShaderStageModel(ShaderStageFlags::Fragment, fragmentShader, &fragmentSpecializationInfo)
    };
    const VertexInputBinding vertexInputBindings[] {
        VertexInputBinding(0, sizeof(PrimitiveVertex), VertexInputRate::Vertex),
    };
    const VertexInputAttribute vertexInputAttributes[] {
        VertexInputAttribute(0, 0, Format::R32G32_SFLOAT, offsetof(PrimitiveVertex, vertPos)),
        VertexInputAttribute(0, 1, Format::R32G32_SFLOAT, offsetof(PrimitiveVertex, vertCenter)),
        VertexInputAttribute(0, 2, Format::R32G32_SFLOAT, offsetof(PrimitiveVertex, vertHalfSize)),
        VertexInputAttribute(0, 3, Format::R32G32B32A32_SFLOAT, offsetof(PrimitiveVertex, vertRadius)),
        VertexInputAttribute(0, 4, Format::R32G32_SFLOAT, offsetof(PrimitiveVertex, vertUV)),
        VertexInputAttribute(0, 5, Format::R32_UINT, offsetof(PrimitiveVertex, vertSpriteIndex)),
        VertexInputAttribute(0, 6, Format::R32_UINT, offsetof(PrimitiveVertex, vertColor)),
        VertexInputAttribute(0, 7, Format::R32_SFLOAT, offsetof(PrimitiveVertex, vertEdgeSoftness)),
        VertexInputAttribute(0, 8, Format::R32G32_SFLOAT, offsetof(PrimitiveVertex, vertRotationCosSin))
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
            std::begin(shaderStageModels), std::end(shaderStageModels),
            VertexInputModel(
                std::begin(vertexInputBindings), std::end(vertexInputBindings),
                std::begin(vertexInputAttributes), std::end(vertexInputAttributes)
            ),
            InputAssemblyModel(PrimitiveTopology::TriangleList),
            TessellationModel(),
            ViewportModel(&viewport, &viewport + 1, &scissor, &scissor + 1),
            RasterizationModel(PolygonMode::Fill, CullModeFlags::Back, FrontFace::Clockwise),
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

bool UI::Renderer::prepare(void) noexcept
{
    using namespace GPU;

    // If the painter has no vertex, cancel rendering preparation
    if (!_painter.vertexCount())
        return false;

    // Compute all sections' sizes
    const auto contextSectionSize = Core::AlignOffset(
        static_cast<std::uint32_t>(sizeof(PrimitiveContext)), _cache.minAlignment
    );
    const auto instancesSectionSize = computeDynamicOffsets();
    const auto verticesSectionSize = Core::AlignOffset(
        static_cast<std::uint32_t>(sizeof(PrimitiveVertex)) * _painter.vertexCount(), _cache.minAlignment
    );
    const auto indicesSectionSize = Core::AlignOffset(
        static_cast<std::uint32_t>(sizeof(PrimitiveIndex)) * _painter.indexCount(), _cache.minAlignment
    );
    FrameCache &frameCache = _perFrameCaches.current();

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
        primitiveCache.offsetsDynamicOffset = Core::AlignOffset(
            primitiveCache.instancesDynamicOffset + queue.instancesByteSize(), alignment
        );
        dynamicOffset = Core::AlignOffset(
            primitiveCache.offsetsDynamicOffset + queue.offsetsByteSize(), alignment
        );
    }
    return dynamicOffset;
}

void UI::Renderer::transferPrimitives(void) noexcept
{
    auto &frameCache = _perFrameCaches.current();

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

    FrameCache &frameCache = _perFrameCaches.current();

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

    const auto &frameCache = _perFrameCaches.current();

    // Dispatch each primitive pipeline
    for (const PrimitiveCache &primitiveCache : _primitiveCaches) {
        if (primitiveCache.instancesDynamicOffset == primitiveCache.offsetsDynamicOffset)
            continue;

        // Bind compute pipeline & descriptor sets
        const std::uint32_t dynamicOffsets[] { primitiveCache.instancesDynamicOffset, primitiveCache.offsetsDynamicOffset };
        recorder.bindPipeline(PipelineBindPoint::Compute, primitiveCache.computePipeline);
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

    FrameCache &frameCache = _perFrameCaches.current();

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

    FrameCache &frameCache = _perFrameCaches.current();
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
            ClearValue { _clearColorValue }
        },
        SubpassContents::Inline
    );

    // Prepare pipeline
    recorder.bindPipeline(PipelineBindPoint::Graphics, _cache.graphicPipeline);
    recorder.bindVertexBuffer(0, frameCache.buffers.deviceBuffer, frameCache.buffers.verticesOffset);
    recorder.bindIndexBuffer(frameCache.buffers.deviceBuffer, IndexType::Uint32, frameCache.buffers.indicesOffset);
    recorder.bindDescriptorSet(
        PipelineBindPoint::Graphics, _cache.graphicPipelineLayout,
        0, _uiSystem->spriteManager().descriptorSet()
    );

    // Utility to conve<rt from clip Area to scissor Rect2D
    const auto toScissor = [extent](const auto &area) {
        if (area == Area())
            return Rect2D { Offset2D(), extent };
        else
            return Rect2D(
                Offset2D(static_cast<int>(area.pos.x), static_cast<int>(area.pos.y)),
                Extent2D(static_cast<int>(area.size.width), static_cast<int>(area.size.height))
            );
    };

    // Reset scissor
    recorder.setScissor(toScissor(Area()));

    // Loop over each clip and draw all vertices between them
    const auto indexCount = _painter.indexCount();
    auto clip = _painter.clips().begin();
    const auto clipEnd = _painter.clips().end();
    std::uint32_t indexOffset {};
    const auto drawSection = [](const auto &recorder, const auto indexOffset, const auto drawCount) {
        if (drawCount) [[likely]]
            recorder.drawIndexed(drawCount, 1, indexOffset);
    };

    while (true) {
        // Clip list not exhausted
        if (clip != clipEnd) [[likely]] {
            // Draw from current offset to clip offset
            drawSection(recorder, indexOffset, clip->indexOffset - indexOffset);
            // Set next offset and scissor
            indexOffset = clip->indexOffset;
            const auto scissor = toScissor(clip->area);
            recorder.setScissor(scissor);
            ++clip;
        // Clip list exhausted
        } else {
            // Draw from current offset to the end
            drawSection(recorder, indexOffset, indexCount - indexOffset);
            break;
        }
    }

    // End render pass
    recorder.endRenderPass();
}
