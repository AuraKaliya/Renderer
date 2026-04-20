#include "viewer_ui_state_adapter.h"

#include "renderer/parametric_model/model_structure.h"
#include "renderer/parametric_model/parametric_evaluator.h"

namespace viewer_ui {
namespace {

const renderer::parametric_model::FeatureDescriptor* findFeatureByKind(
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
    renderer::parametric_model::FeatureKind kind)
{
    for (const auto& feature : descriptor.features) {
        if (feature.kind == kind) {
            return &feature;
        }
    }
    return nullptr;
}

const renderer::parametric_model::FeatureDescriptor* findFeatureById(
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
    renderer::parametric_model::ParametricFeatureId featureId)
{
    for (const auto& feature : descriptor.features) {
        if (feature.id == featureId) {
            return &feature;
        }
    }
    return nullptr;
}

const renderer::parametric_model::PrimitiveDescriptor* findPrimitive(
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor)
{
    const auto* feature = findFeatureByKind(
        descriptor,
        renderer::parametric_model::FeatureKind::primitive);
    return feature != nullptr ? &feature->primitive : nullptr;
}

}  // namespace

SceneObjectUiState ViewerUiStateAdapter::buildSceneObjectState(
    const SceneObjectAdapterInput& input)
{
    const auto& descriptor = input.descriptor;

    SceneObjectUiState state;
    state.id = descriptor.metadata.id;
    state.primitiveKind = descriptor.metadata.objectKind;
    if (const auto* primitive = findPrimitive(descriptor); primitive != nullptr) {
        state.primitive = *primitive;
    }
    state.nodes = descriptor.nodes;
    state.visible = input.visible;
    state.rotationSpeed = input.rotationSpeed;
    state.color = input.color;
    state.bounds = input.bounds;

    if (const auto* mirrorFeature = findFeatureByKind(
            descriptor,
            renderer::parametric_model::FeatureKind::mirror);
        mirrorFeature != nullptr) {
        state.mirror.enabled = mirrorFeature->enabled;
        state.mirror.axis = static_cast<int>(mirrorFeature->mirror.axis);
        state.mirror.planeOffset = mirrorFeature->mirror.planeOffset;
    }

    if (const auto* linearArrayFeature = findFeatureByKind(
            descriptor,
            renderer::parametric_model::FeatureKind::linear_array);
        linearArrayFeature != nullptr) {
        state.linearArray.enabled = linearArrayFeature->enabled;
        state.linearArray.count = static_cast<int>(linearArrayFeature->linearArray.count);
        state.linearArray.offset = linearArrayFeature->linearArray.offset;
    }

    const auto units =
        renderer::parametric_model::ParametricModelStructure::describeUnits(descriptor);
    state.features.reserve(units.size());
    state.units.reserve(units.size());
    for (const auto& unit : units) {
        const auto* feature = findFeatureById(descriptor, unit.featureId);
        if (feature != nullptr) {
            state.features.push_back({
                feature->id,
                unit.id,
                feature->kind,
                feature->enabled
            });
        }

        state.units.push_back({
            unit.id,
            unit.featureId,
            unit.kind,
            unit.role,
            unit.constructionKind,
            unit.enabled
        });
    }

    const auto unitInputs =
        renderer::parametric_model::ParametricModelStructure::describeUnitInputs(descriptor);
    state.unitInputs.reserve(unitInputs.size());
    for (const auto& unitInput : unitInputs) {
        state.unitInputs.push_back({
            unitInput.unitId,
            unitInput.featureId,
            unitInput.kind,
            unitInput.semantic,
            unitInput.nodeId
        });
    }

    const auto nodeUsages =
        renderer::parametric_model::ParametricModelStructure::describeNodeUsages(descriptor);
    state.nodeUsages.reserve(nodeUsages.size());
    for (const auto& usage : nodeUsages) {
        state.nodeUsages.push_back({
            usage.nodeId,
            usage.unitId,
            usage.featureId,
            usage.semantic
        });
    }

    const auto constructionLinks =
        renderer::parametric_model::ParametricModelStructure::describeConstructionLinks(descriptor);
    state.constructionLinks.reserve(constructionLinks.size());
    for (const auto& link : constructionLinks) {
        state.constructionLinks.push_back({
            link.unitId,
            link.featureId,
            link.constructionKind,
            link.startNodeId,
            link.endNodeId,
            link.startSemantic,
            link.endSemantic
        });
    }

    const auto derivedParameters =
        renderer::parametric_model::ParametricModelStructure::describeDerivedParameters(descriptor);
    state.derivedParameters.reserve(derivedParameters.size());
    for (const auto& parameter : derivedParameters) {
        state.derivedParameters.push_back({
            parameter.unitId,
            parameter.featureId,
            parameter.constructionKind,
            parameter.semantic,
            parameter.value,
            parameter.referenceNodeId,
            parameter.sourceNodeId
        });
    }

    const auto evaluationResult =
        renderer::parametric_model::ParametricEvaluator::evaluate(descriptor);
    state.evaluationSummary.succeeded = evaluationResult.succeeded;
    state.evaluationSummary.vertexCount = evaluationResult.mesh.vertices.size();
    state.evaluationSummary.indexCount = evaluationResult.mesh.indices.size();
    state.evaluationSummary.diagnosticCount = evaluationResult.diagnostics.size();

    const auto unitEvaluations =
        renderer::parametric_model::ParametricModelStructure::describeUnitEvaluations(
            descriptor,
            evaluationResult);
    state.unitEvaluations.reserve(unitEvaluations.size());
    for (const auto& evaluation : unitEvaluations) {
        state.unitEvaluations.push_back({
            evaluation.unitId,
            evaluation.featureId,
            evaluation.status,
            evaluation.diagnosticCount,
            evaluation.warningCount,
            evaluation.errorCount
        });
    }

    state.evaluationDiagnostics.reserve(evaluationResult.diagnostics.size());
    for (const auto& diagnostic : evaluationResult.diagnostics) {
        if (diagnostic.severity == renderer::parametric_model::EvaluationDiagnosticSeverity::warning) {
            ++state.evaluationSummary.warningCount;
        } else if (diagnostic.severity == renderer::parametric_model::EvaluationDiagnosticSeverity::error) {
            ++state.evaluationSummary.errorCount;
        }

        state.evaluationDiagnostics.push_back({
            diagnostic.severity,
            diagnostic.code,
            diagnostic.featureId,
            diagnostic.nodeId,
            diagnostic.message
        });
    }

    return state;
}

}  // namespace viewer_ui
