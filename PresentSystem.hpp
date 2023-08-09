/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Present System
 */

#pragma once

#include <Kube/ECS/System.hpp>

#include "PresentPipeline.hpp"

namespace kF::UI
{
    class PresentSystem;
}

class kF::UI::PresentSystem : public ECS::System<"PresentSystem", PresentPipeline>
{
public:
    /** @brief Virtual destructor */
    virtual ~PresentSystem(void) noexcept override = default;

    /** @brief Constructor */
    PresentSystem(void) noexcept;

    /** @brief Virtual tick callback */
    [[nodiscard]] virtual bool tick(void) noexcept override;

private:
};