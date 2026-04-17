#include "renderer/parametric_model/model_structure.h"

#include <cmath>

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

const ParametricUnitInputDescriptor* findNodeInput(
    const std::vector<ParametricUnitInputDescriptor>& inputs,
    ParametricFeatureId featureId,
    ParametricInputSemantic semantic)
{
    for (const auto& input : inputs) {
        if (input.featureId == featureId
            && input.kind == ParametricInputKind::node
            && input.semantic == semantic
            && input.nodeId != 0U) {
            return &input;
        }
    }
    return nullptr;
}

void appendConstructionLink(
    std::vector<ParametricConstructionLinkDescriptor>& links,
    const FeatureDescriptor& feature,
    const std::vector<ParametricUnitInputDescriptor>& inputs,
    ParametricConstructionKind constructionKind,
    ParametricInputSemantic endSemantic)
{
    const auto* centerInput = findNodeInput(inputs, feature.id, ParametricInputSemantic::center);
    const auto* endInput = findNodeInput(inputs, feature.id, endSemantic);
    if (centerInput == nullptr || endInput == nullptr) {
        return;
    }

    ParametricConstructionLinkDescriptor link;
    link.unitId = effectiveUnitId(feature);
    link.featureId = feature.id;
    link.constructionKind = constructionKind;
    link.startNodeId = centerInput->nodeId;
    link.endNodeId = endInput->nodeId;
    link.startSemantic = centerInput->semantic;
    link.endSemantic = endInput->semantic;
    links.push_back(link);
}

void appendConstructionLinkBySemantics(
    std::vector<ParametricConstructionLinkDescriptor>& links,
    const FeatureDescriptor& feature,
    const std::vector<ParametricUnitInputDescriptor>& inputs,
    ParametricConstructionKind constructionKind,
    ParametricInputSemantic startSemantic,
    ParametricInputSemantic endSemantic)
{
    const auto* startInput = findNodeInput(inputs, feature.id, startSemantic);
    const auto* endInput = findNodeInput(inputs, feature.id, endSemantic);
    if (startInput == nullptr || endInput == nullptr) {
        return;
    }

    ParametricConstructionLinkDescriptor link;
    link.unitId = effectiveUnitId(feature);
    link.featureId = feature.id;
    link.constructionKind = constructionKind;
    link.startNodeId = startInput->nodeId;
    link.endNodeId = endInput->nodeId;
    link.startSemantic = startInput->semantic;
    link.endSemantic = endInput->semantic;
    links.push_back(link);
}

const ParametricNodeDescriptor* findPointNode(
    const ParametricObjectDescriptor& descriptor,
    ParametricNodeId nodeId)
{
    for (const auto& node : descriptor.nodes) {
        if (node.id == nodeId && node.kind == ParametricNodeKind::point) {
            return &node;
        }
    }
    return nullptr;
}

void appendDerivedParameter(
    std::vector<ParametricDerivedParameterDescriptor>& parameters,
    const FeatureDescriptor& feature,
    ParametricConstructionKind constructionKind,
    ParametricInputSemantic semantic,
    float value,
    ParametricNodeId referenceNodeId,
    ParametricNodeId sourceNodeId)
{
    ParametricDerivedParameterDescriptor parameter;
    parameter.unitId = effectiveUnitId(feature);
    parameter.featureId = feature.id;
    parameter.constructionKind = constructionKind;
    parameter.semantic = semantic;
    parameter.value = value;
    parameter.referenceNodeId = referenceNodeId;
    parameter.sourceNodeId = sourceNodeId;
    parameters.push_back(parameter);
}

float distance3(
    const scene_contract::Vec3f& left,
    const scene_contract::Vec3f& right)
{
    const float dx = left.x - right.x;
    const float dy = left.y - right.y;
    const float dz = left.z - right.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float distanceInCylinderPlane(
    const scene_contract::Vec3f& left,
    const scene_contract::Vec3f& right)
{
    const float dx = left.x - right.x;
    const float dz = left.z - right.z;
    return std::sqrt(dx * dx + dz * dz);
}

void applyDiagnosticToUnitEvaluation(
    ParametricUnitEvaluationDescriptor& evaluation,
    const EvaluationDiagnostic& diagnostic)
{
    ++evaluation.diagnosticCount;
    if (diagnostic.severity == EvaluationDiagnosticSeverity::warning) {
        ++evaluation.warningCount;
        if (evaluation.status == ParametricUnitEvaluationStatus::valid) {
            evaluation.status = ParametricUnitEvaluationStatus::warning;
        }
    } else if (diagnostic.severity == EvaluationDiagnosticSeverity::error) {
        ++evaluation.errorCount;
        evaluation.status = ParametricUnitEvaluationStatus::error;
    }
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
            switch (feature.primitive.box.constructionMode) {
            case BoxSpec::ConstructionMode::center_corner_point:
                return ParametricConstructionKind::box_center_corner_point;
            case BoxSpec::ConstructionMode::corner_points:
                return ParametricConstructionKind::box_corner_points;
            case BoxSpec::ConstructionMode::center_size:
                return ParametricConstructionKind::box_center_size;
            }
            break;
        case PrimitiveKind::cylinder:
            switch (feature.primitive.cylinder.constructionMode) {
            case CylinderSpec::ConstructionMode::center_radius_point_height:
                return ParametricConstructionKind::cylinder_center_radius_point_height;
            case CylinderSpec::ConstructionMode::axis_endpoints_radius:
                return ParametricConstructionKind::cylinder_axis_endpoints_radius;
            case CylinderSpec::ConstructionMode::center_radius_height:
                return ParametricConstructionKind::cylinder_center_radius_height;
            }
            break;
        case PrimitiveKind::sphere:
            switch (feature.primitive.sphere.constructionMode) {
            case SphereSpec::ConstructionMode::center_surface_point:
                return ParametricConstructionKind::sphere_center_surface_point;
            case SphereSpec::ConstructionMode::diameter_points:
                return ParametricConstructionKind::sphere_diameter_points;
            case SphereSpec::ConstructionMode::center_radius:
                return ParametricConstructionKind::sphere_center_radius;
            }
            break;
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
                if (feature.primitive.box.constructionMode == BoxSpec::ConstructionMode::corner_points) {
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::corner_start, feature.primitive.box.cornerStart.id);
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::corner_end, feature.primitive.box.cornerEnd.id);
                } else if (feature.primitive.box.constructionMode == BoxSpec::ConstructionMode::center_corner_point) {
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::center, feature.primitive.box.center.id);
                    appendInput(
                        inputs,
                        feature,
                        ParametricInputKind::node,
                        ParametricInputSemantic::corner_point,
                        feature.primitive.box.cornerPoint.id);
                } else {
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::center, feature.primitive.box.center.id);
                    appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::width);
                    appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::height);
                    appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::depth);
                }
                break;
            case PrimitiveKind::cylinder:
                if (feature.primitive.cylinder.constructionMode
                    == CylinderSpec::ConstructionMode::axis_endpoints_radius) {
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::axis_start, feature.primitive.cylinder.axisStart.id);
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::axis_end, feature.primitive.cylinder.axisEnd.id);
                    appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::radius);
                } else {
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::center, feature.primitive.cylinder.center.id);
                    if (feature.primitive.cylinder.constructionMode
                        == CylinderSpec::ConstructionMode::center_radius_point_height) {
                        appendInput(
                            inputs,
                            feature,
                            ParametricInputKind::node,
                            ParametricInputSemantic::radius_point,
                            feature.primitive.cylinder.radiusPoint.id);
                    } else {
                        appendInput(inputs, feature, ParametricInputKind::float_value, ParametricInputSemantic::radius);
                    }
                    appendInput(
                        inputs,
                        feature,
                        ParametricInputKind::float_value,
                        ParametricInputSemantic::height);
                }
                appendInput(inputs, feature, ParametricInputKind::integer_value, ParametricInputSemantic::segments);
                break;
            case PrimitiveKind::sphere:
                if (feature.primitive.sphere.constructionMode == SphereSpec::ConstructionMode::diameter_points) {
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::diameter_start, feature.primitive.sphere.diameterStart.id);
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::diameter_end, feature.primitive.sphere.diameterEnd.id);
                } else if (feature.primitive.sphere.constructionMode == SphereSpec::ConstructionMode::center_surface_point) {
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::center, feature.primitive.sphere.center.id);
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::surface_point, feature.primitive.sphere.surfacePoint.id);
                } else {
                    appendInput(inputs, feature, ParametricInputKind::node, ParametricInputSemantic::center, feature.primitive.sphere.center.id);
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

std::vector<ParametricConstructionLinkDescriptor> ParametricModelStructure::describeConstructionLinks(
    const ParametricObjectDescriptor& descriptor)
{
    std::vector<ParametricConstructionLinkDescriptor> links;
    const auto inputs = describeUnitInputs(descriptor);

    for (const auto& feature : descriptor.features) {
        if (!feature.enabled || feature.kind != FeatureKind::primitive) {
            continue;
        }

        const auto constructionKind = constructionKindForFeature(feature);
        switch (constructionKind) {
        case ParametricConstructionKind::box_center_corner_point:
            appendConstructionLink(
                links,
                feature,
                inputs,
                constructionKind,
                ParametricInputSemantic::corner_point);
            break;
        case ParametricConstructionKind::box_corner_points:
            appendConstructionLinkBySemantics(
                links,
                feature,
                inputs,
                constructionKind,
                ParametricInputSemantic::corner_start,
                ParametricInputSemantic::corner_end);
            break;
        case ParametricConstructionKind::cylinder_center_radius_point_height:
            appendConstructionLink(
                links,
                feature,
                inputs,
                constructionKind,
                ParametricInputSemantic::radius_point);
            break;
        case ParametricConstructionKind::cylinder_axis_endpoints_radius:
            appendConstructionLinkBySemantics(
                links,
                feature,
                inputs,
                constructionKind,
                ParametricInputSemantic::axis_start,
                ParametricInputSemantic::axis_end);
            break;
        case ParametricConstructionKind::sphere_center_surface_point:
            appendConstructionLink(
                links,
                feature,
                inputs,
                constructionKind,
                ParametricInputSemantic::surface_point);
            break;
        case ParametricConstructionKind::sphere_diameter_points:
            appendConstructionLinkBySemantics(
                links,
                feature,
                inputs,
                constructionKind,
                ParametricInputSemantic::diameter_start,
                ParametricInputSemantic::diameter_end);
            break;
        case ParametricConstructionKind::box_center_size:
        case ParametricConstructionKind::cylinder_center_radius_height:
        case ParametricConstructionKind::sphere_center_radius:
        case ParametricConstructionKind::mirror_axis_plane:
        case ParametricConstructionKind::linear_array_count_offset:
            break;
        }
    }

    return links;
}

std::vector<ParametricDerivedParameterDescriptor> ParametricModelStructure::describeDerivedParameters(
    const ParametricObjectDescriptor& descriptor)
{
    std::vector<ParametricDerivedParameterDescriptor> parameters;

    for (const auto& feature : descriptor.features) {
        if (!feature.enabled || feature.kind != FeatureKind::primitive) {
            continue;
        }

        const auto constructionKind = constructionKindForFeature(feature);
        switch (constructionKind) {
        case ParametricConstructionKind::box_center_corner_point: {
            const auto& box = feature.primitive.box;
            const auto* centerNode = findPointNode(descriptor, box.center.id);
            const auto* cornerNode = findPointNode(descriptor, box.cornerPoint.id);
            if (centerNode == nullptr || cornerNode == nullptr) {
                break;
            }

            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::width,
                std::abs((cornerNode->point.position.x - centerNode->point.position.x) * 2.0F),
                centerNode->id,
                cornerNode->id);
            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::height,
                std::abs((cornerNode->point.position.y - centerNode->point.position.y) * 2.0F),
                centerNode->id,
                cornerNode->id);
            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::depth,
                std::abs((cornerNode->point.position.z - centerNode->point.position.z) * 2.0F),
                centerNode->id,
                cornerNode->id);
            break;
        }
        case ParametricConstructionKind::box_corner_points: {
            const auto& box = feature.primitive.box;
            const auto* startNode = findPointNode(descriptor, box.cornerStart.id);
            const auto* endNode = findPointNode(descriptor, box.cornerEnd.id);
            if (startNode == nullptr || endNode == nullptr) {
                break;
            }

            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::width,
                std::abs(endNode->point.position.x - startNode->point.position.x),
                startNode->id,
                endNode->id);
            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::height,
                std::abs(endNode->point.position.y - startNode->point.position.y),
                startNode->id,
                endNode->id);
            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::depth,
                std::abs(endNode->point.position.z - startNode->point.position.z),
                startNode->id,
                endNode->id);
            break;
        }
        case ParametricConstructionKind::cylinder_center_radius_point_height: {
            const auto& cylinder = feature.primitive.cylinder;
            const auto* centerNode = findPointNode(descriptor, cylinder.center.id);
            const auto* radiusNode = findPointNode(descriptor, cylinder.radiusPoint.id);
            if (centerNode == nullptr || radiusNode == nullptr) {
                break;
            }

            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::radius,
                distanceInCylinderPlane(radiusNode->point.position, centerNode->point.position),
                centerNode->id,
                radiusNode->id);
            break;
        }
        case ParametricConstructionKind::cylinder_axis_endpoints_radius: {
            const auto& cylinder = feature.primitive.cylinder;
            const auto* startNode = findPointNode(descriptor, cylinder.axisStart.id);
            const auto* endNode = findPointNode(descriptor, cylinder.axisEnd.id);
            if (startNode == nullptr || endNode == nullptr) {
                break;
            }

            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::height,
                distance3(endNode->point.position, startNode->point.position),
                startNode->id,
                endNode->id);
            break;
        }
        case ParametricConstructionKind::sphere_center_surface_point: {
            const auto& sphere = feature.primitive.sphere;
            const auto* centerNode = findPointNode(descriptor, sphere.center.id);
            const auto* surfaceNode = findPointNode(descriptor, sphere.surfacePoint.id);
            if (centerNode == nullptr || surfaceNode == nullptr) {
                break;
            }

            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::radius,
                distance3(surfaceNode->point.position, centerNode->point.position),
                centerNode->id,
                surfaceNode->id);
            break;
        }
        case ParametricConstructionKind::sphere_diameter_points: {
            const auto& sphere = feature.primitive.sphere;
            const auto* startNode = findPointNode(descriptor, sphere.diameterStart.id);
            const auto* endNode = findPointNode(descriptor, sphere.diameterEnd.id);
            if (startNode == nullptr || endNode == nullptr) {
                break;
            }

            appendDerivedParameter(
                parameters,
                feature,
                constructionKind,
                ParametricInputSemantic::radius,
                distance3(endNode->point.position, startNode->point.position) * 0.5F,
                startNode->id,
                endNode->id);
            break;
        }
        case ParametricConstructionKind::box_center_size:
        case ParametricConstructionKind::cylinder_center_radius_height:
        case ParametricConstructionKind::sphere_center_radius:
        case ParametricConstructionKind::mirror_axis_plane:
        case ParametricConstructionKind::linear_array_count_offset:
            break;
        }
    }

    return parameters;
}

std::vector<ParametricUnitEvaluationDescriptor> ParametricModelStructure::describeUnitEvaluations(
    const ParametricObjectDescriptor& descriptor,
    const EvaluationResult& evaluationResult)
{
    const auto units = describeUnits(descriptor);
    std::vector<ParametricUnitEvaluationDescriptor> evaluations;
    evaluations.reserve(units.size());

    for (const auto& unit : units) {
        ParametricUnitEvaluationDescriptor evaluation;
        evaluation.unitId = unit.id;
        evaluation.featureId = unit.featureId;
        evaluation.status = unit.enabled
            ? ParametricUnitEvaluationStatus::valid
            : ParametricUnitEvaluationStatus::skipped;

        for (const auto& diagnostic : evaluationResult.diagnostics) {
            if (diagnostic.featureId == unit.featureId) {
                applyDiagnosticToUnitEvaluation(evaluation, diagnostic);
            }
        }

        evaluations.push_back(evaluation);
    }

    return evaluations;
}

}  // namespace renderer::parametric_model
