#pragma once

#include "renderer/parametric_model/evaluation_result.h"
#include "renderer/parametric_model/types.h"

namespace renderer::parametric_model {

class ParametricEvaluator {
public:
    [[nodiscard]] static EvaluationResult evaluate(const PrimitiveDescriptor& descriptor);
    [[nodiscard]] static EvaluationResult evaluate(const ParametricObjectDescriptor& descriptor);
};

}  // namespace renderer::parametric_model
