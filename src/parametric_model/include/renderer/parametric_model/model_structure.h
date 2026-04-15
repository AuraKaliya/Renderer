#pragma once

#include <vector>

#include "renderer/parametric_model/types.h"

namespace renderer::parametric_model {

class ParametricModelStructure {
public:
    [[nodiscard]] static ParametricUnitKind unitKindForFeatureKind(FeatureKind kind);
    [[nodiscard]] static ParametricUnitRole unitRoleForFeatureKind(FeatureKind kind);
    [[nodiscard]] static ParametricConstructionKind constructionKindForFeature(
        const FeatureDescriptor& feature);
    [[nodiscard]] static std::vector<ParametricUnitDescriptor> describeUnits(
        const ParametricObjectDescriptor& descriptor);
    [[nodiscard]] static std::vector<ParametricUnitInputDescriptor> describeUnitInputs(
        const ParametricObjectDescriptor& descriptor);
    [[nodiscard]] static std::vector<ParametricNodeUsageDescriptor> describeNodeUsages(
        const ParametricObjectDescriptor& descriptor);
};

}  // namespace renderer::parametric_model
