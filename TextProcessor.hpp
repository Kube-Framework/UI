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
    /** @brief Number of elide dots */
    constexpr std::uint32_t ElideDotCount = 2;

    /** @brief Text primitive */
    struct alignas_cacheline Text : public PrimitiveTag<"Text">
    {
        Area area {}; // Text area
        std::string_view str {}; // Text string
        FontIndex fontIndex {}; // Text font
        Color color {}; // Text color
        Anchor anchor {}; // Text anchor inside its area
        TextAlignment textAlignment {}; // Text alignment
        bool vertical {}; // Vertical text layout
        bool fit {}; // Text fit area
        bool elide {}; // Text eliding
        float rotationAngle {}; // Rotation in radians
        Pixel spacesPerTab { DefaultSpacesPerTab }; // Spaces per tabulation
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
        [[nodiscard]] std::uint32_t InsertInstances<Text>(const Text * const primitiveBegin, const Text * const primitiveEnd,
                std::uint8_t * const instanceBegin) noexcept;
    }
}