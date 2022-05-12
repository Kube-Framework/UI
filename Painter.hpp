/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Painter
 */

#pragma once

#include <Kube/Core/SmallVector.hpp>

#include "Base.hpp"
#include "PrimitiveProcessor.hpp"

namespace kF::UI
{
    class Painter;
}

/** @brief Painter is responsible to manage primitive queues */
class alignas_cacheline kF::UI::Painter
{
public:
    /** @brief Initial allocation count of each primitive */
    static constexpr std::uint32_t InitialAllocationCount { 8 };

    /** @brief Small optimized vector of primitive names */
    using Names = Core::TinySmallVector<Core::HashedName, (Core::CacheLineEighthSize * 3) / sizeof(Core::HashedName), UIAllocator>;

    /** @brief Offset of an instance */
    struct InstanceOffset
    {
        std::uint32_t vertexOffset {};
        std::uint32_t indexOffset {};
    };

    /** @brief Primitive queue cache */
    struct alignas_half_cacheline Queue
    {
        std::uint32_t instanceSize {};
        std::uint32_t instanceAlignment {};
        std::uint32_t verticesPerInstance {};
        std::uint32_t indicesPerInstance {};
        std::uint8_t *data {};
        std::uint32_t size {};
        std::uint32_t capacity {};


        /** @brief Get the offset of the queue */
        [[nodiscard]] inline Painter::InstanceOffset *offsets(void) const noexcept
            { return reinterpret_cast<Painter::InstanceOffset *>(data + capacity * instanceSize); }


        /** @brief Compute instances byte size of queue */
        [[nodiscard]] inline std::uint32_t instancesByteSize(void) const noexcept
            { return size * instanceSize; }

        /** @brief Compute offsets byte size of queue */
        [[nodiscard]] inline std::uint32_t offsetsByteSize(void) const noexcept
            { return size * sizeof(Painter::InstanceOffset); }

        /** @brief Compute total byte capacity of queue */
        [[nodiscard]] inline std::uint32_t totalByteCapacity(void) const noexcept
            { return capacity * (instanceSize + sizeof(InstanceOffset)); }
    };
    static_assert_fit_half_cacheline(Queue);

    /** @brief Vector of primitive queues */
    using Queues = Core::TinyVector<Queue, UIAllocator>;


    /** @brief Destructor */
    ~Painter(void) noexcept;

    /** @brief Constructor */
    Painter(void) noexcept = default;


    /** @brief Draw a single primitive */
    template<kF::UI::PrimitiveKind Primitive>
    inline void draw(const Primitive &primitive) noexcept
        { draw(&primitive, &primitive + 1); }

    /** @brief Draw a list of primitives */
    template<kF::UI::PrimitiveKind Primitive>
    void draw(const Primitive * const primitiveBegin, const Primitive * const primitiveEnd) noexcept;


    /** @brief Get current vertex count of painter */
    [[nodiscard]] std::uint32_t vertexCount(void) noexcept { return _offset.vertexOffset; }

    /** @brief Get current index count of painter */
    [[nodiscard]] std::uint32_t indexCount(void) noexcept { return _offset.indexOffset; }


public: // Renderer reserved functions
    /** @brief Register a primitive type inside the painter */
    void registerPrimitive(const Core::HashedName name, const PrimitiveProcessorModel &model) noexcept;

    /** @brief Clear the painter caches */
    void clear(void) noexcept;


    /** @brief Get painter primitive queues */
    [[nodiscard]] inline const auto &queues(void) const noexcept { return _queues; }

private:
    /** @brief Grow a queue */
    void growQueue(Queue &queue, const std::uint32_t minCapacity) noexcept;


    /** @brief Deallocate queue without modifying members */
    static void DeallocateQueueData(const Queue &queue) noexcept;


    // Cacheline 0
    Names _names {};
    InstanceOffset _offset {};
    Queues _queues {};
};
static_assert_fit_cacheline(kF::UI::Painter);

#include "Painter.ipp"