/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Text processor
 */

#pragma once

#include <Kube/UI/Base.hpp>
#include <Kube/UI/Font.hpp>
#include <Kube/UI/PrimitiveProcessor.hpp>

namespace kF::UI
{
    /** @brief Text primitive */
    struct alignas_cacheline Text : public PrimitiveTag<"Text">
    {
        Area area {};
        std::wstring_view str {};
        FontIndex fontIndex {};
        Color color {};
        Anchor anchor {};
        TextAlignment alignment {};
        bool justify {};
    };
    static_assert_fit_cacheline(Text);

    namespace PrimitiveProcessor
    {
        /** @brief Text processor query model */
        template<>
        [[nodiscard]] PrimitiveProcessorModel QueryModel<Text>(void) noexcept;

        /** @brief Text processor instance count */
        template<>
        [[nodiscard]] std::uint32_t GetInstanceCount(const Text * const primitiveBegin, const Text * const primitiveEnd) noexcept;

        /** @brief Text processor insert model */
        template<>
        void InsertInstances<Text>(const Text * const primitiveBegin, const Text * const primitiveEnd,
                std::uint8_t * const instanceBegin) noexcept;
    }
}