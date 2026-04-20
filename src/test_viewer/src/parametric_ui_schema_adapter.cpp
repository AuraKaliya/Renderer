#include "parametric_ui_schema_adapter.h"

#include "renderer/parametric_model/construction_schema.h"
#include "renderer/parametric_model/model_structure.h"

namespace viewer_ui {
namespace {

using renderer::parametric_model::FeatureDescriptor;
using renderer::parametric_model::FeatureKind;
using renderer::parametric_model::ParametricConstructionSchema;
using renderer::parametric_model::ParametricInputSemantic;
using renderer::parametric_model::ParametricModelStructure;
using renderer::parametric_model::ParametricNodeDescriptor;
using renderer::parametric_model::ParametricNodeId;
using renderer::parametric_model::ParametricNodeKind;
using renderer::parametric_model::ParametricObjectDescriptor;
using renderer::parametric_model::PrimitiveKind;

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

ParametricNodeId nodeIdForPrimitiveInput(
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

float floatValueForInput(
    const FeatureDescriptor& feature,
    ParametricInputSemantic semantic)
{
    switch (feature.kind) {
    case FeatureKind::primitive:
        switch (feature.primitive.kind) {
        case PrimitiveKind::box:
            switch (semantic) {
            case ParametricInputSemantic::width:
                return feature.primitive.box.width;
            case ParametricInputSemantic::height:
                return feature.primitive.box.height;
            case ParametricInputSemantic::depth:
                return feature.primitive.box.depth;
            default:
                return 0.0F;
            }
        case PrimitiveKind::cylinder:
            switch (semantic) {
            case ParametricInputSemantic::radius:
                return feature.primitive.cylinder.radius;
            case ParametricInputSemantic::height:
                return feature.primitive.cylinder.height;
            default:
                return 0.0F;
            }
        case PrimitiveKind::sphere:
            if (semantic == ParametricInputSemantic::radius) {
                return feature.primitive.sphere.radius;
            }
            return 0.0F;
        }
        break;
    case FeatureKind::mirror:
        if (semantic == ParametricInputSemantic::plane_offset) {
            return feature.mirror.planeOffset;
        }
        break;
    case FeatureKind::linear_array:
        break;
    }

    return 0.0F;
}

int integerValueForInput(
    const FeatureDescriptor& feature,
    ParametricInputSemantic semantic)
{
    if (feature.kind == FeatureKind::primitive) {
        switch (feature.primitive.kind) {
        case PrimitiveKind::box:
            break;
        case PrimitiveKind::cylinder:
            if (semantic == ParametricInputSemantic::segments) {
                return static_cast<int>(feature.primitive.cylinder.segments);
            }
            break;
        case PrimitiveKind::sphere:
            if (semantic == ParametricInputSemantic::slices) {
                return static_cast<int>(feature.primitive.sphere.slices);
            }
            if (semantic == ParametricInputSemantic::stacks) {
                return static_cast<int>(feature.primitive.sphere.stacks);
            }
            break;
        }
    }

    if (feature.kind == FeatureKind::linear_array && semantic == ParametricInputSemantic::count) {
        return static_cast<int>(feature.linearArray.count);
    }

    return 0;
}

renderer::scene_contract::Vec3f vectorValueForInput(
    const FeatureDescriptor& feature,
    ParametricInputSemantic semantic)
{
    if (feature.kind == FeatureKind::linear_array && semantic == ParametricInputSemantic::offset) {
        return feature.linearArray.offset;
    }

    return {};
}

int enumValueForInput(
    const FeatureDescriptor& feature,
    ParametricInputSemantic semantic)
{
    if (feature.kind == FeatureKind::mirror && semantic == ParametricInputSemantic::axis) {
        return static_cast<int>(feature.mirror.axis);
    }

    return 0;
}

ParametricFieldUiState buildField(
    const ParametricObjectDescriptor& descriptor,
    const FeatureDescriptor& feature,
    const renderer::parametric_model::ParametricConstructionInputTemplate& inputTemplate)
{
    ParametricFieldUiState field;
    field.kind = inputTemplate.kind;
    field.semantic = inputTemplate.semantic;
    field.label = std::string(ParametricConstructionSchema::inputSemanticLabel(inputTemplate.semantic));
    field.floatValue = floatValueForInput(feature, inputTemplate.semantic);
    field.integerValue = integerValueForInput(feature, inputTemplate.semantic);
    field.vectorValue = vectorValueForInput(feature, inputTemplate.semantic);
    field.enumValue = enumValueForInput(feature, inputTemplate.semantic);

    if (inputTemplate.kind == renderer::parametric_model::ParametricInputKind::node) {
        field.nodeId = nodeIdForPrimitiveInput(feature, inputTemplate.semantic);
        if (const auto* node = findPointNode(descriptor, field.nodeId); node != nullptr) {
            field.hasNodePosition = true;
            field.vectorValue = node->point.position;
        }
    }

    return field;
}

}  // namespace

ParametricFeatureFieldsUiState ParametricUiSchemaAdapter::buildFeatureFields(
    const ParametricObjectDescriptor& descriptor,
    const FeatureDescriptor& feature)
{
    ParametricFeatureFieldsUiState state;
    state.featureId = feature.id;
    state.unitId = feature.unitId != 0U ? feature.unitId : feature.id;
    state.featureKind = feature.kind;
    state.constructionKind = ParametricModelStructure::constructionKindForFeature(feature);
    state.constructionLabel =
        std::string(ParametricConstructionSchema::constructionLabel(state.constructionKind));
    state.enabled = feature.enabled;

    const auto inputTemplates = ParametricConstructionSchema::inputTemplates(state.constructionKind);
    state.fields.reserve(inputTemplates.size());
    for (const auto& inputTemplate : inputTemplates) {
        state.fields.push_back(buildField(descriptor, feature, inputTemplate));
    }

    return state;
}

std::vector<ParametricFeatureFieldsUiState> ParametricUiSchemaAdapter::buildObjectFeatureFields(
    const ParametricObjectDescriptor& descriptor)
{
    std::vector<ParametricFeatureFieldsUiState> states;
    states.reserve(descriptor.features.size());

    for (const auto& feature : descriptor.features) {
        states.push_back(buildFeatureFields(descriptor, feature));
    }

    return states;
}

}  // namespace viewer_ui
