#include "viewer_ui_state_adapter.h"

#include "renderer/parametric_model/primitive_factory.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int gFailures = 0;

void printCaseHeader(const std::string& name) {
    std::cout << "[CASE] " << name << '\n';
}

void expectTrue(const std::string& label, bool value) {
    std::cout << "  " << label << " = " << (value ? "true" : "false")
              << ", expected = true\n";
    if (!value) {
        ++gFailures;
        std::cout << "  [FAIL] " << label << '\n';
    }
}

template <typename T>
void expectEqual(const std::string& label, T actual, T expected) {
    std::cout << "  " << label << " = " << actual
              << ", expected = " << expected << '\n';
    if (actual != expected) {
        ++gFailures;
        std::cout << "  [FAIL] " << label << '\n';
    }
}

void expectNear(const std::string& label, float actual, float expected) {
    std::cout << "  " << label << " = " << actual
              << ", expected = " << expected << '\n';
    if (std::abs(actual - expected) > 0.0001F) {
        ++gFailures;
        std::cout << "  [FAIL] " << label << '\n';
    }
}

void printCaseResult(const std::string& name, int failuresBefore) {
    if (gFailures == failuresBefore) {
        std::cout << "[PASS] " << name << "\n\n";
    } else {
        std::cout << "[FAIL] " << name << "\n\n";
    }
}

viewer_ui::SceneObjectUiState buildState(
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor)
{
    viewer_ui::SceneObjectAdapterInput input;
    input.descriptor = descriptor;
    input.visible = true;
    input.rotationSpeed = 1.25F;
    input.color = {0.2F, 0.4F, 0.6F, 1.0F};
    input.bounds = {{-0.5F, -0.5F, -0.5F}, {0.5F, 0.5F, 0.5F}, true};
    return viewer_ui::ViewerUiStateAdapter::buildSceneObjectState(input);
}

void testBoxCenterSizeState() {
    const std::string name = "box center-size adapter state";
    const int failuresBefore = gFailures;
    printCaseHeader(name);

    auto descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricBoxFromCenterSize(
        {0.0F, 0.0F, 0.0F},
        1.0F,
        2.0F,
        3.0F);
    descriptor.features.push_back(renderer::parametric_model::PrimitiveFactory::makeMirrorFeature());
    descriptor.features.push_back(renderer::parametric_model::PrimitiveFactory::makeLinearArrayFeature());

    const auto state = buildState(descriptor);
    expectEqual("object id", state.id, descriptor.metadata.id);
    expectEqual("feature count", state.features.size(), static_cast<std::size_t>(3U));
    expectEqual("unit count", state.units.size(), static_cast<std::size_t>(3U));
    expectEqual("unit input count", state.unitInputs.size(), static_cast<std::size_t>(8U));
    expectEqual("node usage count", state.nodeUsages.size(), static_cast<std::size_t>(1U));
    expectEqual("construction link count", state.constructionLinks.size(), static_cast<std::size_t>(0U));
    expectEqual("derived parameter count", state.derivedParameters.size(), static_cast<std::size_t>(0U));
    expectTrue("evaluation succeeded", state.evaluationSummary.succeeded);
    expectTrue("vertex count nonzero", state.evaluationSummary.vertexCount > 0U);
    expectEqual("diagnostic count", state.evaluationSummary.diagnosticCount, static_cast<std::size_t>(0U));
    expectNear("rotation speed", state.rotationSpeed, 1.25F);

    printCaseResult(name, failuresBefore);
}

void testBoxCenterCornerDerivedState() {
    const std::string name = "box center-corner derived state";
    const int failuresBefore = gFailures;
    printCaseHeader(name);

    const auto descriptor =
        renderer::parametric_model::PrimitiveFactory::makeParametricBoxFromCenterCornerPoint(
            {0.0F, 0.0F, 0.0F},
            {1.0F, 2.0F, 3.0F});

    const auto state = buildState(descriptor);
    expectEqual("feature count", state.features.size(), static_cast<std::size_t>(1U));
    expectEqual("unit input count", state.unitInputs.size(), static_cast<std::size_t>(2U));
    expectEqual("node usage count", state.nodeUsages.size(), static_cast<std::size_t>(2U));
    expectEqual("construction link count", state.constructionLinks.size(), static_cast<std::size_t>(1U));
    expectEqual("derived parameter count", state.derivedParameters.size(), static_cast<std::size_t>(3U));
    expectNear("derived width", state.derivedParameters[0].value, 2.0F);
    expectNear("derived height", state.derivedParameters[1].value, 4.0F);
    expectNear("derived depth", state.derivedParameters[2].value, 6.0F);

    printCaseResult(name, failuresBefore);
}

void testEvaluationDiagnosticsState() {
    const std::string name = "invalid sphere diagnostic state";
    const int failuresBefore = gFailures;
    printCaseHeader(name);

    const auto descriptor =
        renderer::parametric_model::PrimitiveFactory::makeParametricSphereFromCenterRadius(
            {0.0F, 0.0F, 0.0F},
            -1.0F,
            1U,
            1U);

    const auto state = buildState(descriptor);
    expectTrue("evaluation succeeded after clamp", state.evaluationSummary.succeeded);
    expectEqual("diagnostic count", state.evaluationSummary.diagnosticCount, static_cast<std::size_t>(3U));
    expectEqual("warning count", state.evaluationSummary.warningCount, static_cast<std::size_t>(3U));
    expectEqual("error count", state.evaluationSummary.errorCount, static_cast<std::size_t>(0U));
    expectEqual(
        "unit evaluation warning count",
        state.unitEvaluations.front().warningCount,
        static_cast<std::uint32_t>(3U));

    printCaseResult(name, failuresBefore);
}

}  // namespace

int main() {
    testBoxCenterSizeState();
    testBoxCenterCornerDerivedState();
    testEvaluationDiagnosticsState();

    if (gFailures == 0) {
        std::cout << "[PASS] viewer_ui_state_adapter_smoke_tests\n";
        return 0;
    }

    std::cout << "[FAIL] viewer_ui_state_adapter_smoke_tests failures=" << gFailures << '\n';
    return 1;
}
