#pragma once

#include <cstdint>

#include "renderer/parametric_model/types.h"
#include "renderer/scene_contract/types.h"

namespace viewer_ui {

struct AddBoxModelInput {
    renderer::parametric_model::BoxSpec::ConstructionMode constructionMode =
        renderer::parametric_model::BoxSpec::ConstructionMode::center_size;
    renderer::scene_contract::Vec3f center {};
    renderer::scene_contract::Vec3f cornerPoint {0.5F, 0.5F, 0.5F};
    renderer::scene_contract::Vec3f cornerStart {-0.5F, -0.5F, -0.5F};
    renderer::scene_contract::Vec3f cornerEnd {0.5F, 0.5F, 0.5F};
    float width = 1.0F;
    float height = 1.0F;
    float depth = 1.0F;
};

struct AddCylinderModelInput {
    renderer::parametric_model::CylinderSpec::ConstructionMode constructionMode =
        renderer::parametric_model::CylinderSpec::ConstructionMode::center_radius_height;
    renderer::scene_contract::Vec3f center {};
    renderer::scene_contract::Vec3f radiusPoint {0.5F, 0.0F, 0.0F};
    renderer::scene_contract::Vec3f axisStart {0.0F, -0.5F, 0.0F};
    renderer::scene_contract::Vec3f axisEnd {0.0F, 0.5F, 0.0F};
    float radius = 0.5F;
    float height = 1.0F;
    std::uint32_t segments = 24U;
};

struct AddSphereModelInput {
    renderer::parametric_model::SphereSpec::ConstructionMode constructionMode =
        renderer::parametric_model::SphereSpec::ConstructionMode::center_radius;
    renderer::scene_contract::Vec3f center {};
    renderer::scene_contract::Vec3f surfacePoint {0.5F, 0.0F, 0.0F};
    renderer::scene_contract::Vec3f diameterStart {-0.5F, 0.0F, 0.0F};
    renderer::scene_contract::Vec3f diameterEnd {0.5F, 0.0F, 0.0F};
    float radius = 0.5F;
    std::uint32_t slices = 24U;
    std::uint32_t stacks = 16U;
};

class ParametricModelAddAdapter {
public:
    [[nodiscard]] static renderer::parametric_model::ParametricObjectDescriptor makeBox(
        const AddBoxModelInput& input);

    [[nodiscard]] static renderer::parametric_model::ParametricObjectDescriptor makeCylinder(
        const AddCylinderModelInput& input);

    [[nodiscard]] static renderer::parametric_model::ParametricObjectDescriptor makeSphere(
        const AddSphereModelInput& input);
};

}  // namespace viewer_ui
