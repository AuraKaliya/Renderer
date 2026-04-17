#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "renderer/parametric_model/types.h"

namespace renderer::parametric_model {

struct ParametricConstructionInputTemplate {
    ParametricInputKind kind = ParametricInputKind::float_value;
    ParametricInputSemantic semantic = ParametricInputSemantic::radius;
};

struct ParametricConstructionLinkTemplate {
    ParametricInputSemantic startSemantic = ParametricInputSemantic::center;
    ParametricInputSemantic endSemantic = ParametricInputSemantic::surface_point;
};

class ParametricConstructionSchema {
public:
    [[nodiscard]] static std::string_view constructionLabel(
        ParametricConstructionKind constructionKind);
    [[nodiscard]] static std::string_view inputSemanticLabel(
        ParametricInputSemantic semantic);
    [[nodiscard]] static std::string_view inputSemanticOverlayLabel(
        ParametricInputSemantic semantic);
    [[nodiscard]] static std::vector<ParametricConstructionInputTemplate> inputTemplates(
        ParametricConstructionKind constructionKind);
    [[nodiscard]] static std::optional<ParametricConstructionLinkTemplate> constructionLink(
        ParametricConstructionKind constructionKind);
};

}  // namespace renderer::parametric_model
