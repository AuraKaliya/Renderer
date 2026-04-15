#include "renderer/parametric_model/model_structure.h"

namespace renderer::parametric_model {
namespace {

ParametricUnitId effectiveUnitId(const FeatureDescriptor& feature) {
    return feature.unitId != 0U ? feature.unitId : feature.id;
}

void appendInput(
    std::vector<ParametricUnitInputDescriptor>& inputs,
    const FeatureDescriptor& feature,
    ParametricInputKind kind,
    ParametricInputSemantic semantic,
    ParametricNodeId nodeId = 0U)
{
    ParametricUnitInputDescriptor input;
    input.unitId = effectiveUnitId(feature);
    input.featureId = feature.id;
    input.kind = kind;
    input.semantic = semantic;
    input.nodeId = nodeId;
    inputs.push_back(input);
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

ParametricConstructionKind ParametricModelStructure::constructionKindForFeature(
    const FeatureDescriptor& feature)
{
    switch (feature.kind) {
    case FeatureKind::primitive:
        switch (feature.primitive.kind) {
        case PrimitiveKind::box:
            return ParametricConstructionKind::box_center_size;
        case PrimitiveKind::cylinder:
            return ParametricConstructionKind::cylinder_center_radius_height;
        case PrimitiveKind::sphere:
            return feature.primitive.sphere.constructionMode == SphereSpec::ConstructionMode::center_surface_point
                ? ParametricConstructionKind::sphere_center_surface_point
                : ParametricConstructionKind::sphere_center_radius;
        }
        break;
    case FeatureKind::mirror:
        return ParametricConstructionKind::mirror_axis_plane;
    case FeatureKind::linear_array:
        return ParametricConstructionKind::linear_array_count_offset;
    }

    return ParametricConstructionKind::box_center_size;
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
        unit.constructionKind = constructionKindForFeature(feature);
        unit.enabled = feature.enabled;
        units.push_back(unit);
    }

    return units;
}

std::vector<ParametricUnitInputDescriptor> ParametricModelStructure::describeUnitInputs(
    const ParametricObjectDescriptor& descriptor)
{
    std::vector<ParametricUnitInputDescriptor> inputs;

    for (const auto& feature : descriptor.features) {
        switch (feature.kind) {
        case FeatureKind::primitive:
            switch (feature.primitive.kind) {
            case PrimitiveKind::box:
                appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::center, feature.primitive.box.center.id);
                appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::width);
                appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::height);
                appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::depth);
                break;
            case PrimitiveKind::cylinder:
                appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::center, feature.primitive.cylinder.center.id);
                appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::radius);
                appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::height);
                appendInput(inputs, feature, ParametricInputKind::integer_value, ParametricInputSemantic::segments);
                break;
            case PrimitiveKind::sphere:
                appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::center, feature.primitive.sphere.center.id);
                if (feature.primitive.sphere.constructionMode == SphereSpec::ConstructionMode::center_surface_point) {
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::surface_point, feature.primitive.sphere.surfacePoint.id);
                } else {
                    appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::radius);
                }
                appendInput(inputs, feature, ParametricInputKind::integer_value, ParametricInputSemantic::slices);
                appendInput(inputs, feature, ParametricInputKind::integer_value, ParametricInputSemantic::stacks);
                break;
            }
            break;
        case FeatureKind::mirror:
            appendInput(inputs, feature, ParametricInputKind::enum_value, ParametricInputSemantic::axis);
            appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::plane_offset);
            break;
        case FeatureKind::linear_array:
            appendInput(inputs, feature, ParametricInputKind::integer_value, ParametricInputSemantic::count);
            appendInput(inputs, feature, ParametricInputKind::vector3, ParametricInputSemantic::offset);
            break;
        }
    }

    return inputs;
}

std::vector<ParametricNodeUsageDescriptor> ParametricModelStructure::describeNodeUsages(
    const ParametricObjectDescriptor& descriptor)
{
    std::vector<ParametricNodeUsageDescriptor> usages;
    const auto inputs = describeUnitInputs(descriptor);

    for (const auto& input : inputs) {
        if (input.kind != ParametricInputKind::node || input.nodeId == 0U) {
            continue;
        }

        ParametricNodeUsageDescriptor usage;
        usage.nodeId = input.nodeId;
        usage.unitId = input.unitId;
        usage.featureId = input.featureId;
        usage.semantic = input.semantic;
        usages.push_back(usage);
    }

    return usages;
}

}  // namespace renderer::parametric_model
