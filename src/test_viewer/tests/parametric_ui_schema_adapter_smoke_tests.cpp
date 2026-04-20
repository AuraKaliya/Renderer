#include "parametric_ui_schema_adapter.h"

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

const viewer_ui::ParametricFieldUiState* findField(
    const viewer_ui::ParametricFeatureFieldsUiState& state,
    renderer::parametric_model::ParametricInputSemantic semantic)
{
    for (const auto& field : state.fields) {
        if (field.semantic == semantic) {
            return &field;
        }
    }
    return nullptr;
}

void testBoxCenterSizeFields() {
    const std::string name = "box center-size schema fields";
    const int failuresBefore = gFailures;
    printCaseHeader(name);

    const auto descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricBoxFromCenterSize(
        {1.0F, 2.0F, 3.0F},
        4.0F,
        5.0F,
        6.0F);

    const auto state = viewer_ui::ParametricUiSchemaAdapter::buildFeatureFields(
        descriptor,
        descriptor.features.front());
    expectEqual("construction label", state.constructionLabel, std::string("Box: center + size"));
    expectEqual("field count", state.fields.size(), static_cast<std::size_t>(4U));

    const auto* center = findField(state, renderer::parametric_model::ParametricInputSemantic::center);
    const auto* width = findField(state, renderer::parametric_model::ParametricInputSemantic::width);
    const auto* height = findField(state, renderer::parametric_model::ParametricInputSemantic::height);
    const auto* depth = findField(state, renderer::parametric_model::ParametricInputSemantic::depth);

    expectTrue("center field exists", center != nullptr);
    expectTrue("center node position exists", center != nullptr && center->hasNodePosition);
    expectNear("center x", center != nullptr ? center->vectorValue.x : 0.0F, 1.0F);
    expectNear("center y", center != nullptr ? center->vectorValue.y : 0.0F, 2.0F);
    expectNear("center z", center != nullptr ? center->vectorValue.z : 0.0F, 3.0F);
    expectNear("width value", width != nullptr ? width->floatValue : 0.0F, 4.0F);
    expectNear("height value", height != nullptr ? height->floatValue : 0.0F, 5.0F);
    expectNear("depth value", depth != nullptr ? depth->floatValue : 0.0F, 6.0F);

    printCaseResult(name, failuresBefore);
}

void testCylinderAxisEndpointFields() {
    const std::string name = "cylinder axis endpoint schema fields";
    const int failuresBefore = gFailures;
    printCaseHeader(name);

    const auto descriptor =
        renderer::parametric_model::PrimitiveFactory::makeParametricCylinderFromAxisEndpointsRadius(
            {0.0F, -1.0F, 0.0F},
            {0.0F, 2.0F, 0.0F},
            0.75F,
            32U);

    const auto state = viewer_ui::ParametricUiSchemaAdapter::buildFeatureFields(
        descriptor,
        descriptor.features.front());
    expectEqual(
        "construction kind",
        static_cast<int>(state.constructionKind),
        static_cast<int>(renderer::parametric_model::ParametricConstructionKind::cylinder_axis_endpoints_radius));
    expectEqual("field count", state.fields.size(), static_cast<std::size_t>(4U));

    const auto* axisStart = findField(state, renderer::parametric_model::ParametricInputSemantic::axis_start);
    const auto* axisEnd = findField(state, renderer::parametric_model::ParametricInputSemantic::axis_end);
    const auto* radius = findField(state, renderer::parametric_model::ParametricInputSemantic::radius);
    const auto* segments = findField(state, renderer::parametric_model::ParametricInputSemantic::segments);

    expectTrue("axis start node exists", axisStart != nullptr && axisStart->nodeId != 0U);
    expectTrue("axis end node exists", axisEnd != nullptr && axisEnd->nodeId != 0U);
    expectNear("axis start y", axisStart != nullptr ? axisStart->vectorValue.y : 0.0F, -1.0F);
    expectNear("axis end y", axisEnd != nullptr ? axisEnd->vectorValue.y : 0.0F, 2.0F);
    expectNear("radius value", radius != nullptr ? radius->floatValue : 0.0F, 0.75F);
    expectEqual("segments value", segments != nullptr ? segments->integerValue : 0, 32);

    printCaseResult(name, failuresBefore);
}

void testOperatorFields() {
    const std::string name = "operator schema fields";
    const int failuresBefore = gFailures;
    printCaseHeader(name);

    auto descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricSphereFromCenterRadius(
        {0.0F, 0.0F, 0.0F},
        1.0F,
        24U,
        16U);
    auto mirror = renderer::parametric_model::PrimitiveFactory::makeMirrorFeature();
    mirror.enabled = true;
    mirror.mirror.axis = renderer::parametric_model::Axis::z;
    mirror.mirror.planeOffset = 2.5F;
    descriptor.features.push_back(mirror);

    auto array = renderer::parametric_model::PrimitiveFactory::makeLinearArrayFeature();
    array.enabled = true;
    array.linearArray.count = 5U;
    array.linearArray.offset = {2.0F, 0.5F, -1.0F};
    descriptor.features.push_back(array);

    const auto states = viewer_ui::ParametricUiSchemaAdapter::buildObjectFeatureFields(descriptor);
    expectEqual("feature field group count", states.size(), static_cast<std::size_t>(3U));

    const auto& mirrorState = states[1];
    const auto& arrayState = states[2];
    const auto* axis = findField(mirrorState, renderer::parametric_model::ParametricInputSemantic::axis);
    const auto* planeOffset = findField(
        mirrorState,
        renderer::parametric_model::ParametricInputSemantic::plane_offset);
    const auto* count = findField(arrayState, renderer::parametric_model::ParametricInputSemantic::count);
    const auto* offset = findField(arrayState, renderer::parametric_model::ParametricInputSemantic::offset);

    expectEqual("mirror field count", mirrorState.fields.size(), static_cast<std::size_t>(2U));
    expectEqual("mirror axis enum", axis != nullptr ? axis->enumValue : -1, 2);
    expectNear("mirror plane offset", planeOffset != nullptr ? planeOffset->floatValue : 0.0F, 2.5F);
    expectEqual("array field count", arrayState.fields.size(), static_cast<std::size_t>(2U));
    expectEqual("array count", count != nullptr ? count->integerValue : 0, 5);
    expectNear("array offset x", offset != nullptr ? offset->vectorValue.x : 0.0F, 2.0F);
    expectNear("array offset y", offset != nullptr ? offset->vectorValue.y : 0.0F, 0.5F);
    expectNear("array offset z", offset != nullptr ? offset->vectorValue.z : 0.0F, -1.0F);

    printCaseResult(name, failuresBefore);
}

}  // namespace

int main() {
    testBoxCenterSizeFields();
    testCylinderAxisEndpointFields();
    testOperatorFields();

    if (gFailures == 0) {
        std::cout << "[PASS] parametric_ui_schema_adapter_smoke_tests\n";
        return 0;
    }

    std::cout << "[FAIL] parametric_ui_schema_adapter_smoke_tests failures=" << gFailures << '\n';
    return 1;
}
