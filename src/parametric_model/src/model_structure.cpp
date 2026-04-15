#include "renderer/parametric_model/model_structure.h"

namespace renderer::parametric_model {
namespace {

ParametricUnitId effectiveUnitId(const FeatureDescriptor& feature) {
    return feature.unitId != 0U ? feature.unitId : feature.id;
}

}  // namespace

ParametricUnitKind ParametricModelStructure::unitKindForFeatureKind(FeatureKind kind) {
    switch (kind) {
    case FeatureKind::primitive:
        return ParametricUnitKind::primitive_generator;
    case FeatureKind::mirror:
        return ParametricUnitKind::mirror_operator;
    case FeatureKind::linear_array:
        return ParametricUnitKind::linear_array_operator;
    }

    return ParametricUnitKind::primitive_generator;
}

ParametricUnitRole ParametricModelStructure::unitRoleForFeatureKind(FeatureKind kind) {
    switch (kind) {
    case FeatureKind::primitive:
        return ParametricUnitRole::generator;
    case FeatureKind::mirror:
    case FeatureKind::linear_array:
        return ParametricUnitRole::operator_unit;
    }

    return ParametricUnitRole::generator;
}

std::vector<ParametricUnitDescriptor> ParametricModelStructure::describeUnits(
    const ParametricObjectDescriptor& descriptor)
{
    std::vector<ParametricUnitDescriptor> units;
    units.reserve(descriptor.features.size());

    for (const auto& feature : descriptor.features) {
        ParametricUnitDescriptor unit;
        unit.id = effectiveUnitId(feature);
        unit.featureId = feature.id;
        unit.kind = unitKindForFeatureKind(feature.kind);
        unit.role = unitRoleForFeatureKind(feature.kind);
        unit.enabled = feature.enabled;
        units.push_back(unit);
    }

    return units;
}

}  // namespace renderer::parametric_model
