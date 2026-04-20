#pragma once

#include <string>
#include <vector>

#include "renderer/parametric_model/types.h"
#include "renderer/scene_contract/types.h"

namespace viewer_ui {

struct ParametricFieldUiState {
    renderer::parametric_model::ParametricInputKind kind =
        renderer::parametric_model::ParametricInputKind::float_value;
    renderer::parametric_model::ParametricInputSemantic semantic =
        renderer::parametric_model::ParametricInputSemantic::radius;
    std::string label;
    bool editable = true;
    renderer::parametric_model::ParametricNodeId nodeId = 0U;
    bool hasNodePosition = false;
    renderer::scene_contract::Vec3f vectorValue {};
    float floatValue = 0.0F;
    int integerValue = 0;
    int enumValue = 0;
};

struct ParametricFeatureFieldsUiState {
    renderer::parametric_model::ParametricFeatureId featureId = 0U;
    renderer::parametric_model::ParametricUnitId unitId = 0U;
    renderer::parametric_model::FeatureKind featureKind =
        renderer::parametric_model::FeatureKind::primitive;
    renderer::parametric_model::ParametricConstructionKind constructionKind =
        renderer::parametric_model::ParametricConstructionKind::box_center_size;
    std::string constructionLabel;
    bool enabled = true;
    std::vector<ParametricFieldUiState> fields;
};

class ParametricUiSchemaAdapter {
public:
    [[nodiscard]] static ParametricFeatureFieldsUiState buildFeatureFields(
        const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
        const renderer::parametric_model::FeatureDescriptor& feature);

    [[nodiscard]] static std::vector<ParametricFeatureFieldsUiState> buildObjectFeatureFields(
        const renderer::parametric_model::ParametricObjectDescriptor& descriptor);
};

}  // namespace viewer_ui
