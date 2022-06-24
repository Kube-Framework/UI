/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Painter
 */

#include <Kube/Core/Abort.hpp>

#include "Painter.hpp"

using namespace kF;

UI::Painter::~Painter(void) noexcept
{
    for (const auto &queue : _queues)
        DeallocateQueueData(queue);
}

void UI::Painter::setClip(const Area &area) noexcept
{
    _clips.push(ClipCache {
        .area = area,
        .indexOffset = _offset.indexOffset
    });
}

void UI::Painter::registerPrimitive(const Core::HashedName name, const PrimitiveProcessorModel &model) noexcept
{
    // Ensure the primitive is not already registered
    for (const auto primitiveName : _names) {
        kFEnsure(primitiveName != name, "UI::Renderer::registerPrimitive: Primitive already registered");
    }

    _names.push(name);
    _queues.push(Queue {
        .instanceSize = model.instanceSize,
        .instanceAlignment = model.instanceAlignment,
        .verticesPerInstance = model.verticesPerInstance,
        .indicesPerInstance = model.indicesPerInstance
    });
}

void UI::Painter::clear(void) noexcept
{
    // Reset vertex & index offsets
    _offset = InstanceOffset {};

    // Reset clips
    _clips.clear();

    // Set each queue size to 0 as any primitive is ensured to be trivial
    for (auto &queue : _queues)
        queue.size = 0u;
}

void UI::Painter::growQueue(Queue &queue, const std::uint32_t minCapacity) noexcept
{
    // Allocate the necessary size
    const auto capacity = std::max(minCapacity, queue.capacity * 2u);
    const auto instancesByteSize = capacity * queue.instanceSize;
    const auto data = Core::AlignedAlloc<std::uint8_t>(
        instancesByteSize + capacity * sizeof(InstanceOffset),
        queue.instanceAlignment
    );
    auto * const offsets = reinterpret_cast<InstanceOffset *>(data + instancesByteSize);

    // If the queue already has an allocation we must move then free
    if (queue.data) [[likely]] {
        // Move old data to new allocation
        std::memcpy(data, queue.data, queue.instancesByteSize());
        std::memcpy(offsets, queue.offsets(), queue.offsetsByteSize());

        // Delete old data
        DeallocateQueueData(queue);
    }

    // Assign queue's data
    queue.data = data;
    queue.capacity = capacity;
}

void UI::Painter::DeallocateQueueData(const Queue &queue) noexcept
{
    Core::AlignedFree(
        queue.data,
        queue.totalByteCapacity(),
        queue.instanceAlignment
    );
}
