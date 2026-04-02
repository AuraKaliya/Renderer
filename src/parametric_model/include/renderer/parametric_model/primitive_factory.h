#pragma once

#include <cstdint>

#include "renderer/scene_contract/types.h"

namespace renderer::parametric_model {

enum class PrimitiveKind : std::uint8_t {
    box,
    cylinder,
    sphere
};

struct BoxSpec {
    float width = 1.0F;
    float height = 1.0F;
    float depth = 1.0F;
};

struct CylinderSpec {
    float radius = 0.5F;
    float height = 1.0F;
    std::uint32_t segments = 24U;
};

struct SphereSpec {
    float radius = 0.5F;
    std::uint32_t slices = 24U;
    std::uint32_t stacks = 16U;
};

struct PrimitiveDescriptor {
    PrimitiveKind kind = PrimitiveKind::box;
    BoxSpec box {};
    CylinderSpec cylinder {};
    SphereSpec sphere {};
};

class PrimitiveFactory {
public:
    [[nodiscard]] static PrimitiveDescriptor makeBoxDescriptor(
        float width,
        float height,
        float depth);

    [[nodiscard]] static PrimitiveDescriptor makeCylinderDescriptor(
        float radius,
        float height,
        std::uint32_t segments = 24U);

    [[nodiscard]] static PrimitiveDescriptor makeSphereDescriptor(
        float radius,
        std::uint32_t slices = 24U,
        std::uint32_t stacks = 16U);

    [[nodiscard]] static scene_contract::MeshData build(const PrimitiveDescriptor& descriptor);

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
