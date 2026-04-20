#include "parametric_model_add_adapter.h"

#include "renderer/parametric_model/primitive_factory.h"

namespace viewer_ui {
namespace {

renderer::scene_contract::Vec3f midpoint(
    const renderer::scene_contract::Vec3f& left,
    const renderer::scene_contract::Vec3f& right)
{
    return {
        (left.x + right.x) * 0.5F,
        (left.y + right.y) * 0.5F,
        (left.z + right.z) * 0.5F
    };
}

const renderer::parametric_model::ParametricNodeDescriptor* findPointNode(
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
    renderer::parametric_model::ParametricNodeId nodeId)
{
    for (const auto& node : descriptor.nodes) {
        if (node.id == nodeId && node.kind == renderer::parametric_model::ParametricNodeKind::point) {
            return &node;
        }
    }
    return nullptr;
}

renderer::scene_contract::Vec3f pointPosition(
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
    renderer::parametric_model::NodeReference reference,
    const renderer::scene_contract::Vec3f& fallback)
{
    const auto* node = findPointNode(descriptor, reference.id);
    return node != nullptr ? node->point.position : fallback;
}

void updatePrimitivePivot(renderer::parametric_model::ParametricObjectDescriptor& descriptor) {
    if (descriptor.features.empty()
        || descriptor.features.front().kind != renderer::parametric_model::FeatureKind::primitive) {
        return;
    }

    const auto& primitive = descriptor.features.front().primitive;
    switch (primitive.kind) {
    case renderer::parametric_model::PrimitiveKind::box:
        if (primitive.box.constructionMode
            == renderer::parametric_model::BoxSpec::ConstructionMode::corner_points) {
            descriptor.metadata.pivot = midpoint(
                pointPosition(descriptor, primitive.box.cornerStart, {}),
                pointPosition(descriptor, primitive.box.cornerEnd, {}));
        } else {
            descriptor.metadata.pivot = pointPosition(descriptor, primitive.box.center, {});
        }
        break;
    case renderer::parametric_model::PrimitiveKind::cylinder:
        if (primitive.cylinder.constructionMode
            == renderer::parametric_model::CylinderSpec::ConstructionMode::axis_endpoints_radius) {
            descriptor.metadata.pivot = midpoint(
                pointPosition(descriptor, primitive.cylinder.axisStart, {}),
                pointPosition(descriptor, primitive.cylinder.axisEnd, {}));
        } else {
            descriptor.metadata.pivot = pointPosition(descriptor, primitive.cylinder.center, {});
        }
        break;
    case renderer::parametric_model::PrimitiveKind::sphere:
        if (primitive.sphere.constructionMode
            == renderer::parametric_model::SphereSpec::ConstructionMode::diameter_points) {
            descriptor.metadata.pivot = midpoint(
                pointPosition(descriptor, primitive.sphere.diameterStart, {}),
                pointPosition(descriptor, primitive.sphere.diameterEnd, {}));
        } else {
            descriptor.metadata.pivot = pointPosition(descriptor, primitive.sphere.center, {});
        }
        break;
    }
}

void appendDefaultOperators(renderer::parametric_model::ParametricObjectDescriptor& descriptor) {
    updatePrimitivePivot(descriptor);
    descriptor.features.push_back(renderer::parametric_model::PrimitiveFactory::makeMirrorFeature());
    descriptor.features.push_back(renderer::parametric_model::PrimitiveFactory::makeLinearArrayFeature());
}

}  // namespace

renderer::parametric_model::ParametricObjectDescriptor ParametricModelAddAdapter::makeBox(
    const AddBoxModelInput& input)
{
    renderer::parametric_model::ParametricObjectDescriptor descriptor;
    switch (input.constructionMode) {
    case renderer::parametric_model::BoxSpec::ConstructionMode::center_size:
        descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricBoxFromCenterSize(
            input.center,
            input.width,
            input.height,
            input.depth);
        break;
    case renderer::parametric_model::BoxSpec::ConstructionMode::center_corner_point:
        descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricBoxFromCenterCornerPoint(
            input.center,
            input.cornerPoint);
        break;
    case renderer::parametric_model::BoxSpec::ConstructionMode::corner_points:
        descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricBoxFromCornerPoints(
            input.cornerStart,
            input.cornerEnd);
        break;
    }

    appendDefaultOperators(descriptor);
    return descriptor;
}

renderer::parametric_model::ParametricObjectDescriptor ParametricModelAddAdapter::makeCylinder(
    const AddCylinderModelInput& input)
{
    renderer::parametric_model::ParametricObjectDescriptor descriptor;
    switch (input.constructionMode) {
    case renderer::parametric_model::CylinderSpec::ConstructionMode::center_radius_height:
        descriptor =
            renderer::parametric_model::PrimitiveFactory::makeParametricCylinderFromCenterRadiusHeight(
                input.center,
                input.radius,
                input.height,
                input.segments);
        break;
    case renderer::parametric_model::CylinderSpec::ConstructionMode::center_radius_point_height:
        descriptor =
            renderer::parametric_model::PrimitiveFactory::makeParametricCylinderFromCenterRadiusPointHeight(
                input.center,
                input.radiusPoint,
                input.height,
                input.segments);
        break;
    case renderer::parametric_model::CylinderSpec::ConstructionMode::axis_endpoints_radius:
        descriptor =
            renderer::parametric_model::PrimitiveFactory::makeParametricCylinderFromAxisEndpointsRadius(
                input.axisStart,
                input.axisEnd,
                input.radius,
                input.segments);
        break;
    }

    appendDefaultOperators(descriptor);
    return descriptor;
}

renderer::parametric_model::ParametricObjectDescriptor ParametricModelAddAdapter::makeSphere(
    const AddSphereModelInput& input)
{
    renderer::parametric_model::ParametricObjectDescriptor descriptor;
    switch (input.constructionMode) {
    case renderer::parametric_model::SphereSpec::ConstructionMode::center_radius:
        descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricSphereFromCenterRadius(
            input.center,
            input.radius,
            input.slices,
            input.stacks);
        break;
    case renderer::parametric_model::SphereSpec::ConstructionMode::center_surface_point:
        descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricSphereFromCenterSurfacePoint(
            input.center,
            input.surfacePoint,
            input.slices,
            input.stacks);
        break;
    case renderer::parametric_model::SphereSpec::ConstructionMode::diameter_points:
        descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricSphereFromDiameterPoints(
            input.diameterStart,
            input.diameterEnd,
            input.slices,
            input.stacks);
        break;
    }

    appendDefaultOperators(descriptor);
    return descriptor;
}

}  // namespace viewer_ui
