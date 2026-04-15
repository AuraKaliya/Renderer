#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "renderer/parametric_model/types.h"
#include "renderer/scene_contract/types.h"

namespace renderer::parametric_model {

enum class EvaluationDiagnosticSeverity : std::uint8_t {
    info,
    warning,
    error
};

enum class EvaluationDiagnosticCode : std::uint8_t {
    value_clamped,
    missing_node,
    invalid_feature_order,
    empty_model
};

struct EvaluationDiagnostic {
    EvaluationDiagnosticSeverity severity = EvaluationDiagnosticSeverity::info;
    EvaluationDiagnosticCode code = EvaluationDiagnosticCode::empty_model;
    ParametricFeatureId featureId = 0U;
    ParametricNodeId nodeId = 0U;
    std::string message;
};

struct EvaluationResult {
    scene_contract::MeshData mesh;
    std::vector<EvaluationDiagnostic> diagnostics;
    bool succeeded = true;
};

}  // namespace renderer::parametric_model
