#include "renderer/parametric_model/model_structure.h"

#include "renderer/parametric_model/construction_schema.h"

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
    const ParametricConstructionLinkTemplate& linkTemplate)
{
    const auto* startInput = findNodeInput(inputs, feature.id, linkTemplate.startSemantic);
    const auto* endInput = findNodeInput(inputs, feature.id, linkTemplate.endSemantic);
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

ParametricNodeId nodeIdForInput(
    const FeatureDescriptor& feature,
    ParametricInputSemantic semantic)
{
    if (feature.kind != FeatureKind::primitive) {
        return 0U;
    }

    switch (feature.primitive.kind) {
    case PrimitiveKind::box:
        switch (semantic) {
        case ParametricInputSemantic::center:
            return feature.primitive.box.center.id;
        case ParametricInputSemantic::corner_point:
            return feature.primitive.box.cornerPoint.id;
        case ParametricInputSemantic::corner_start:
            return feature.primitive.box.cornerStart.id;
        case ParametricInputSemantic::corner_end:
            return feature.primitive.box.cornerEnd.id;
        default:
            return 0U;
        }
    case PrimitiveKind::cylinder:
        switch (semantic) {
        case ParametricInputSemantic::center:
            return feature.primitive.cylinder.center.id;
        case ParametricInputSemantic::radius_point:
            return feature.primitive.cylinder.radiusPoint.id;
        case ParametricInputSemantic::axis_start:
            return feature.primitive.cylinder.axisStart.id;
        case ParametricInputSemantic::axis_end:
            return feature.primitive.cylinder.axisEnd.id;
        default:
            return 0U;
        }
    case PrimitiveKind::sphere:
        switch (semantic) {
        case ParametricInputSemantic::center:
            return feature.primitive.sphere.center.id;
        case ParametricInputSemantic::surface_point:
            return feature.primitive.sphere.surfacePoint.id;
        case ParametricInputSemantic::diameter_start:
            return feature.primitive.sphere.diameterStart.id;
        case ParametricInputSemantic::diameter_end:
            return feature.primitive.sphere.diameterEnd.id;
        default:
            return 0U;
        }
    }

    return 0U;
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
        const auto constructionKind = constructionKindForFeature(feature);
        const auto inputTemplates = ParametricConstructionSchema::inputTemplates(constructionKind);
        for (const auto& inputTemplate : inputTemplates) {
            appendInput(
                inputs,
                feature,
                inputTemplate.kind,
                inputTemplate.semantic,
                inputTemplate.kind == ParametricInputKind::node
                    ? nodeIdForInput(feature, inputTemplate.semantic)
                    : 0U);
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
        const auto linkTemplate = ParametricConstructionSchema::constructionLink(constructionKind);
        if (linkTemplate.has_value()) {
            appendConstructionLink(links, feature, inputs, constructionKind, *linkTemplate);
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
