#pragma once

#include "renderer/scene_contract/types.h"

namespace renderer::parametric_model {

class PrimitiveFactory {
public:
    [[nodiscard]] static scene_contract::MeshData makeBox(
        float width,
        float height,
        float depth);

    [[nodiscard]] static scene_contract::MeshData makeCylinder(
        float radius,
        float height,
        std::uint32_t segments = 24U);

    [[nodiscard]] static scene_contract::MeshData makeSphere(
        float radius,
        std::uint32_t slices = 24U,
        std::uint32_t stacks = 16U);
};

}  // namespace renderer::parametric_model
