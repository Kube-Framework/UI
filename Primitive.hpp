/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Primitive concept
 */

#pragma once

#include <Kube/Core/Hash.hpp>
#include <Kube/Core/FixedString.hpp>

namespace kF::UI
{
    /** @brief Primitive unique type tag
     *  @param Literal Primitive's unique name */
    template<Core::FixedString Literal>
    struct PrimitiveTag
    {
        /** @brief Primitive name */
        static constexpr auto Name = Literal.toView();

        /** @brief Primitive hashed name */
        static constexpr auto Hash = Core::Hash(Name);
    };

    /** @brief Primitive concept */
    template<typename Type>
    concept PrimitiveKind = std::is_trivially_destructible_v<Type> && Core::IsTag<Type, PrimitiveTag>;
}