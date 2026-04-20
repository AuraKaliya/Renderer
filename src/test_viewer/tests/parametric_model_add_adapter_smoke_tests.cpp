#include "parametric_model_add_adapter.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int gFailures = 0;

void printCaseHeader(const std::string& name) {
    std::cout << "[CASE] " << name << '\n';
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

void testAddBoxCenterCorner() {
    const std::string name = "add box center-corner descriptor";
    const int failuresBefore = gFailures;
    printCaseHeader(name);

    viewer_ui::AddBoxModelInput input;
    input.constructionMode =
        renderer::parametric_model::BoxSpec::ConstructionMode::center_corner_point;
    input.center = {1.0F, 2.0F, 3.0F};
    input.cornerPoint = {2.0F, 4.0F, 6.0F};

    const auto descriptor = viewer_ui::ParametricModelAddAdapter::makeBox(input);
    expectEqual("feature count", descriptor.features.size(), static_cast<std::size_t>(3U));
    expectEqual("node count", descriptor.nodes.size(), static_cast<std::size_t>(2U));
    expectEqual(
        "box construction mode",
        static_cast<int>(descriptor.features.front().primitive.box.constructionMode),
        static_cast<int>(renderer::parametric_model::BoxSpec::ConstructionMode::center_corner_point));
    expectNear("box width", descriptor.features.front().primitive.box.width, 2.0F);
    expectNear("box height", descriptor.features.front().primitive.box.height, 4.0F);
    expectNear("box depth", descriptor.features.front().primitive.box.depth, 6.0F);
    expectNear("box pivot x", descriptor.metadata.pivot.x, 1.0F);
    expectNear("box pivot y", descriptor.metadata.pivot.y, 2.0F);
    expectNear("box pivot z", descriptor.metadata.pivot.z, 3.0F);

    printCaseResult(name, failuresBefore);
}

void testAddCylinderAxisEndpoints() {
    const std::string name = "add cylinder axis-endpoints descriptor";
    const int failuresBefore = gFailures;
    printCaseHeader(name);

    viewer_ui::AddCylinderModelInput input;
    input.constructionMode =
        renderer::parametric_model::CylinderSpec::ConstructionMode::axis_endpoints_radius;
    input.axisStart = {0.0F, -1.0F, 0.0F};
    input.axisEnd = {0.0F, 2.0F, 0.0F};
    input.radius = 0.75F;
    input.segments = 32U;

    const auto descriptor = viewer_ui::ParametricModelAddAdapter::makeCylinder(input);
    expectEqual("feature count", descriptor.features.size(), static_cast<std::size_t>(3U));
    expectEqual("node count", descriptor.nodes.size(), static_cast<std::size_t>(2U));
    expectEqual(
        "cylinder construction mode",
        static_cast<int>(descriptor.features.front().primitive.cylinder.constructionMode),
        static_cast<int>(renderer::parametric_model::CylinderSpec::ConstructionMode::axis_endpoints_radius));
    expectNear("cylinder radius", descriptor.features.front().primitive.cylinder.radius, 0.75F);
    expectNear("cylinder height", descriptor.features.front().primitive.cylinder.height, 3.0F);
    expectEqual("cylinder segments", descriptor.features.front().primitive.cylinder.segments, 32U);
    expectNear("cylinder pivot x", descriptor.metadata.pivot.x, 0.0F);
    expectNear("cylinder pivot y", descriptor.metadata.pivot.y, 0.5F);
    expectNear("cylinder pivot z", descriptor.metadata.pivot.z, 0.0F);

    printCaseResult(name, failuresBefore);
}

void testAddSphereDiameter() {
    const std::string name = "add sphere diameter descriptor";
    const int failuresBefore = gFailures;
    printCaseHeader(name);

    viewer_ui::AddSphereModelInput input;
    input.constructionMode =
        renderer::parametric_model::SphereSpec::ConstructionMode::diameter_points;
    input.diameterStart = {-2.0F, 0.0F, 0.0F};
    input.diameterEnd = {2.0F, 0.0F, 0.0F};
    input.slices = 40U;
    input.stacks = 20U;

    const auto descriptor = viewer_ui::ParametricModelAddAdapter::makeSphere(input);
    expectEqual("feature count", descriptor.features.size(), static_cast<std::size_t>(3U));
    expectEqual("node count", descriptor.nodes.size(), static_cast<std::size_t>(2U));
    expectEqual(
        "sphere construction mode",
        static_cast<int>(descriptor.features.front().primitive.sphere.constructionMode),
        static_cast<int>(renderer::parametric_model::SphereSpec::ConstructionMode::diameter_points));
    expectNear("sphere radius", descriptor.features.front().primitive.sphere.radius, 2.0F);
    expectEqual("sphere slices", descriptor.features.front().primitive.sphere.slices, 40U);
    expectEqual("sphere stacks", descriptor.features.front().primitive.sphere.stacks, 20U);
    expectNear("sphere pivot x", descriptor.metadata.pivot.x, 0.0F);
    expectNear("sphere pivot y", descriptor.metadata.pivot.y, 0.0F);
    expectNear("sphere pivot z", descriptor.metadata.pivot.z, 0.0F);

    printCaseResult(name, failuresBefore);
}

}  // namespace

int main() {
    testAddBoxCenterCorner();
    testAddCylinderAxisEndpoints();
    testAddSphereDiameter();

    if (gFailures == 0) {
        std::cout << "[PASS] parametric_model_add_adapter_smoke_tests\n";
        return 0;
    }

    std::cout << "[FAIL] parametric_model_add_adapter_smoke_tests failures=" << gFailures << '\n';
    return 1;
}
