#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "renderer/parametric_model/evaluation_result.h"
#include "renderer/parametric_model/types.h"
#include "renderer/scene_contract/types.h"

namespace viewer_ui {

struct MirrorUiState {
    bool enabled = false;
    int axis = 0;
    float planeOffset = 0.0F;
};

struct LinearArrayUiState {
    bool enabled = false;
    int count = 1;
    renderer::scene_contract::Vec3f offset {};
};

struct FeatureUiState {
    renderer::parametric_model::ParametricFeatureId id = 0U;
    renderer::parametric_model::ParametricUnitId unitId = 0U;
    renderer::parametric_model::FeatureKind kind = renderer::parametric_model::FeatureKind::primitive;
    bool enabled = true;
};

struct UnitUiState {
    renderer::parametric_model::ParametricUnitId id = 0U;
    renderer::parametric_model::ParametricFeatureId featureId = 0U;
    renderer::parametric_model::ParametricUnitKind kind =
        renderer::parametric_model::ParametricUnitKind::primitive_generator;
    renderer::parametric_model::ParametricUnitRole role =
        renderer::parametric_model::ParametricUnitRole::generator;
    renderer::parametric_model::ParametricConstructionKind constructionKind =
        renderer::parametric_model::ParametricConstructionKind::box_center_size;
    bool enabled = true;
};

struct UnitEvaluationUiState {
    renderer::parametric_model::ParametricUnitId unitId = 0U;
    renderer::parametric_model::ParametricFeatureId featureId = 0U;
    renderer::parametric_model::ParametricUnitEvaluationStatus status =
        renderer::parametric_model::ParametricUnitEvaluationStatus::valid;
    std::uint32_t diagnosticCount = 0U;
    std::uint32_t warningCount = 0U;
    std::uint32_t errorCount = 0U;
};

struct UnitInputUiState {
    renderer::parametric_model::ParametricUnitId unitId = 0U;
    renderer::parametric_model::ParametricFeatureId featureId = 0U;
    renderer::parametric_model::ParametricInputKind kind =
        renderer::parametric_model::ParametricInputKind::float_value;
    renderer::parametric_model::ParametricInputSemantic semantic =
        renderer::parametric_model::ParametricInputSemantic::radius;
    renderer::parametric_model::ParametricNodeId nodeId = 0U;
};

struct NodeUsageUiState {
    renderer::parametric_model::ParametricNodeId nodeId = 0U;
    renderer::parametric_model::ParametricUnitId unitId = 0U;
    renderer::parametric_model::ParametricFeatureId featureId = 0U;
    renderer::parametric_model::ParametricInputSemantic semantic =
        renderer::parametric_model::ParametricInputSemantic::center;
};

struct ConstructionLinkUiState {
    renderer::parametric_model::ParametricUnitId unitId = 0U;
    renderer::parametric_model::ParametricFeatureId featureId = 0U;
    renderer::parametric_model::ParametricConstructionKind constructionKind =
        renderer::parametric_model::ParametricConstructionKind::box_center_size;
    renderer::parametric_model::ParametricNodeId startNodeId = 0U;
    renderer::parametric_model::ParametricNodeId endNodeId = 0U;
    renderer::parametric_model::ParametricInputSemantic startSemantic =
        renderer::parametric_model::ParametricInputSemantic::center;
    renderer::parametric_model::ParametricInputSemantic endSemantic =
        renderer::parametric_model::ParametricInputSemantic::surface_point;
};

struct DerivedParameterUiState {
    renderer::parametric_model::ParametricUnitId unitId = 0U;
    renderer::parametric_model::ParametricFeatureId featureId = 0U;
    renderer::parametric_model::ParametricConstructionKind constructionKind =
        renderer::parametric_model::ParametricConstructionKind::box_center_size;
    renderer::parametric_model::ParametricInputSemantic semantic =
        renderer::parametric_model::ParametricInputSemantic::radius;
    float value = 0.0F;
    renderer::parametric_model::ParametricNodeId referenceNodeId = 0U;
    renderer::parametric_model::ParametricNodeId sourceNodeId = 0U;
};

struct EvaluationDiagnosticUiState {
    renderer::parametric_model::EvaluationDiagnosticSeverity severity =
        renderer::parametric_model::EvaluationDiagnosticSeverity::info;
    renderer::parametric_model::EvaluationDiagnosticCode code =
        renderer::parametric_model::EvaluationDiagnosticCode::empty_model;
    renderer::parametric_model::ParametricFeatureId featureId = 0U;
    renderer::parametric_model::ParametricNodeId nodeId = 0U;
    std::string message;
};

struct EvaluationSummaryUiState {
    bool succeeded = false;
    std::size_t vertexCount = 0U;
    std::size_t indexCount = 0U;
    std::size_t diagnosticCount = 0U;
    std::size_t warningCount = 0U;
    std::size_t errorCount = 0U;
};

struct SceneObjectUiState {
    renderer::parametric_model::ParametricObjectId id = 0U;
    renderer::parametric_model::PrimitiveKind primitiveKind =
        renderer::parametric_model::PrimitiveKind::box;
    renderer::parametric_model::PrimitiveDescriptor primitive {};
    std::vector<renderer::parametric_model::ParametricNodeDescriptor> nodes;
    bool visible = true;
    float rotationSpeed = 0.0F;
    renderer::scene_contract::ColorRgba color {};
    renderer::scene_contract::Aabb bounds {};
    MirrorUiState mirror {};
    LinearArrayUiState linearArray {};
    std::vector<FeatureUiState> features;
    std::vector<UnitUiState> units;
    std::vector<UnitEvaluationUiState> unitEvaluations;
    std::vector<UnitInputUiState> unitInputs;
    std::vector<NodeUsageUiState> nodeUsages;
    std::vector<ConstructionLinkUiState> constructionLinks;
    std::vector<DerivedParameterUiState> derivedParameters;
    std::vector<EvaluationDiagnosticUiState> evaluationDiagnostics;
    EvaluationSummaryUiState evaluationSummary {};
};

struct SceneObjectAdapterInput {
    renderer::parametric_model::ParametricObjectDescriptor descriptor {};
    bool visible = true;
    float rotationSpeed = 0.0F;
    renderer::scene_contract::ColorRgba color {};
    renderer::scene_contract::Aabb bounds {};
};

}  // namespace viewer_ui
