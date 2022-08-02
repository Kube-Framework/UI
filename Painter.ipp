/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Painter
 */

#include <Kube/Core/Assert.hpp>

#include "Painter.hpp"

template<kF::UI::PrimitiveKind Primitive>
inline void kF::UI::Painter::draw(const Primitive * const primitiveBegin, const Primitive * const primitiveEnd) noexcept
{
    // Find primitive index
    const std::uint32_t primitiveIndex = [this] {
        for (std::uint32_t index { 0u }; const auto name : _names) {
            if (name != Primitive::Hash) [[likely]]
                ++index;
            else [[unlikely]]
                return index;
        }
        kFDebugAbort("UI::Painter::draw: Primitive '", Primitive::Name, "' not registered");
        return ~0u;
    }();

    // Get instance count
    const std::uint32_t instanceCount = PrimitiveProcessor::GetInstanceCount(primitiveBegin, primitiveEnd);

    // Ensure we don't run out of space in queue
    auto &queue = _queues[primitiveIndex];
    if (queue.size + instanceCount > queue.capacity) [[unlikely]]
        growQueue(queue, std::max(queue.size + instanceCount, InitialAllocationCount));

    // Insert instances
    const auto insertedInstanceCount = PrimitiveProcessor::InsertInstances(
        primitiveBegin,
        primitiveEnd,
        queue.data + queue.size * queue.instanceSize
    );

    // Ensure inserted count is less or equal to reserved instance count
    kFAssert(instanceCount >= insertedInstanceCount,
        "UI::Painter::draw: 'PrimitiveProcessor::GetInstanceCount' returned ", instanceCount,
        " but 'PrimitiveProcessor::InsertInstances' returned ", insertedInstanceCount);

    // Insert offsets
    const auto offsets = queue.offsets() + queue.size;
    for (auto index = 0u; index != insertedInstanceCount; ++index) {
        offsets[index] = InstanceOffset {
            .vertexOffset = _offset.vertexOffset + index * queue.verticesPerInstance,
            .indexOffset = _offset.indexOffset + index * queue.indicesPerInstance
        };
    }
    _offset.vertexOffset += insertedInstanceCount * queue.verticesPerInstance;
    _offset.indexOffset += insertedInstanceCount * queue.indicesPerInstance;

    // Assign new queue size
    queue.size += insertedInstanceCount;
}