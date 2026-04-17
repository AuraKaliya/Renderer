#include "renderer/parametric_model/primitive_factory.h"

#include "renderer/parametric_model/parametric_evaluator.h"

#include <cmath>

namespace renderer::parametric_model {
namespace {

ParametricObjectId nextParametricObjectId() {
    static ParametricObjectId nextId = 1U;
    return nextId++;
}

ParametricFeatureId nextParametricFeatureId() {
    static ParametricFeatureId nextId = 1U;
    return nextId++;
}

ParametricUnitId nextParametricUnitId() {
    static ParametricUnitId nextId = 1U;
    return nextId++;
}

ParametricNodeId nextParametricNodeId() {
    static ParametricNodeId nextId = 1U;
    return nextId++;
}

scene_contract::Vec3f subtractVec3(
    const scene_contract::Vec3f& left,
    const scene_contract::Vec3f& right)
{
    return {
        left.x - right.x,
        left.y - right.y,
        left.z - right.z
    };
}

float lengthVec3(const scene_contract::Vec3f& value) {
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

float radiusInCylinderPlane(
    const scene_contract::Vec3f& radiusPoint,
    const scene_contract::Vec3f& center)
{
    const float dx = radiusPoint.x - center.x;
    const float dz = radiusPoint.z - center.z;
    return std::sqrt(dx * dx + dz * dz);
}

}  // namespace

ParametricNodeDescriptor PrimitiveFactory::makePointNode(
    const scene_contract::Vec3f& position)
{
    ParametricNodeDescriptor node;
    node.id = nextParametricNodeId();
    node.kind = ParametricNodeKind::point;
    node.point.position = position;
    return node;
}

ParametricNodeDescriptor PrimitiveFactory::makeDirectionNode(
    const scene_contract::Vec3f& direction)
{
    ParametricNodeDescriptor node;
    node.id = nextParametricNodeId();
    node.kind = ParametricNodeKind::direction;
    node.direction.direction = direction;
    return node;
}

ParametricNodeDescriptor PrimitiveFactory::makeAxisNode(
    const scene_contract::Vec3f& origin,
    const scene_contract::Vec3f& direction)
{
    ParametricNodeDescriptor node;
    node.id = nextParametricNodeId();
    node.kind = ParametricNodeKind::axis;
    node.axis.origin = origin;
    node.axis.direction = direction;
    return node;
}

ParametricNodeDescriptor PrimitiveFactory::makePlaneNode(
    const scene_contract::Vec3f& origin,
    const scene_contract::Vec3f& normal)
{
    ParametricNodeDescriptor node;
    node.id = nextParametricNodeId();
    node.kind = ParametricNodeKind::plane;
    node.plane.origin = origin;
    node.plane.normal = normal;
    return node;
}

ParametricNodeDescriptor PrimitiveFactory::makeScalarNode(float value) {
    ParametricNodeDescriptor node;
    node.id = nextParametricNodeId();
    node.kind = ParametricNodeKind::scalar;
    node.scalar.value = value;
    return node;
}

PrimitiveDescriptor PrimitiveFactory::makeBoxDescriptor(
    float width,
    float height,
    float depth)
{
    PrimitiveDescriptor descriptor;
    descriptor.kind = PrimitiveKind::box;
    descriptor.box = {width, height, depth};
    return descriptor;
}

PrimitiveDescriptor PrimitiveFactory::makeCylinderDescriptor(
    float radius,
    float height,
    std::uint32_t segments)
{
    PrimitiveDescriptor descriptor;
    descriptor.kind = PrimitiveKind::cylinder;
    descriptor.cylinder = {radius, height, segments};
    return descriptor;
}

PrimitiveDescriptor PrimitiveFactory::makeSphereDescriptor(
    float radius,
    std::uint32_t slices,
    std::uint32_t stacks)
{
    PrimitiveDescriptor descriptor;
    descriptor.kind = PrimitiveKind::sphere;
    descriptor.sphere = {radius, slices, stacks};
    return descriptor;
}

FeatureDescriptor PrimitiveFactory::makePrimitiveFeature(
    const PrimitiveDescriptor& primitive)
{
    FeatureDescriptor feature;
    feature.id = nextParametricFeatureId();
    feature.unitId = nextParametricUnitId();
    feature.kind = FeatureKind::primitive;
    feature.enabled = true;
    feature.primitive = primitive;
    return feature;
}

FeatureDescriptor PrimitiveFactory::makeMirrorFeature() {
    FeatureDescriptor feature;
    feature.id = nextParametricFeatureId();
    feature.unitId = nextParametricUnitId();
    feature.kind = FeatureKind::mirror;
    feature.enabled = false;
    feature.mirror.axis = Axis::x;
    return feature;
}

FeatureDescriptor PrimitiveFactory::makeLinearArrayFeature() {
    FeatureDescriptor feature;
    feature.id = nextParametricFeatureId();
    feature.unitId = nextParametricUnitId();
    feature.kind = FeatureKind::linear_array;
    feature.enabled = false;
    feature.linearArray.count = 1U;
    feature.linearArray.offset = {1.0F, 0.0F, 0.0F};
    return feature;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricObject(
    const PrimitiveDescriptor& basePrimitive)
{
    ParametricObjectDescriptor descriptor;
    descriptor.metadata.id = nextParametricObjectId();
    descriptor.metadata.objectKind = basePrimitive.kind;
    descriptor.features.push_back(makePrimitiveFeature(basePrimitive));
    return descriptor;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricBoxFromCenterSize(
    const scene_contract::Vec3f& center,
    float width,
    float height,
    float depth)
{
    auto descriptor = makeParametricObject(makeBoxDescriptor(width, height, depth));
    auto centerNode = makePointNode(center);
    auto* primitiveFeature = descriptor.features.empty() ? nullptr : &descriptor.features.front();
    if (primitiveFeature != nullptr && primitiveFeature->kind == FeatureKind::primitive) {
        primitiveFeature->primitive.box.constructionMode = BoxSpec::ConstructionMode::center_size;
        primitiveFeature->primitive.box.center = {centerNode.id};
    }
    descriptor.nodes.push_back(centerNode);
    return descriptor;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricBoxFromCenterCornerPoint(
    const scene_contract::Vec3f& center,
    const scene_contract::Vec3f& cornerPoint)
{
    const float width = std::abs((cornerPoint.x - center.x) * 2.0F);
    const float height = std::abs((cornerPoint.y - center.y) * 2.0F);
    const float depth = std::abs((cornerPoint.z - center.z) * 2.0F);
    auto descriptor = makeParametricObject(makeBoxDescriptor(width, height, depth));
    auto centerNode = makePointNode(center);
    auto cornerNode = makePointNode(cornerPoint);
    auto* primitiveFeature = descriptor.features.empty() ? nullptr : &descriptor.features.front();
    if (primitiveFeature != nullptr && primitiveFeature->kind == FeatureKind::primitive) {
        primitiveFeature->primitive.box.constructionMode = BoxSpec::ConstructionMode::center_corner_point;
        primitiveFeature->primitive.box.center = {centerNode.id};
        primitiveFeature->primitive.box.cornerPoint = {cornerNode.id};
    }
    descriptor.nodes.push_back(centerNode);
    descriptor.nodes.push_back(cornerNode);
    return descriptor;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricBoxFromCornerPoints(
    const scene_contract::Vec3f& cornerStart,
    const scene_contract::Vec3f& cornerEnd)
{
    const float width = std::abs(cornerEnd.x - cornerStart.x);
    const float height = std::abs(cornerEnd.y - cornerStart.y);
    const float depth = std::abs(cornerEnd.z - cornerStart.z);
    auto descriptor = makeParametricObject(makeBoxDescriptor(width, height, depth));
    auto startNode = makePointNode(cornerStart);
    auto endNode = makePointNode(cornerEnd);
    auto* primitiveFeature = descriptor.features.empty() ? nullptr : &descriptor.features.front();
    if (primitiveFeature != nullptr && primitiveFeature->kind == FeatureKind::primitive) {
        primitiveFeature->primitive.box.constructionMode = BoxSpec::ConstructionMode::corner_points;
        primitiveFeature->primitive.box.cornerStart = {startNode.id};
        primitiveFeature->primitive.box.cornerEnd = {endNode.id};
    }
    descriptor.nodes.push_back(startNode);
    descriptor.nodes.push_back(endNode);
    return descriptor;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricCylinderFromCenterRadiusHeight(
    const scene_contract::Vec3f& center,
    float radius,
    float height,
    std::uint32_t segments)
{
    auto descriptor = makeParametricObject(makeCylinderDescriptor(radius, height, segments));
    auto centerNode = makePointNode(center);
    auto* primitiveFeature = descriptor.features.empty() ? nullptr : &descriptor.features.front();
    if (primitiveFeature != nullptr && primitiveFeature->kind == FeatureKind::primitive) {
        primitiveFeature->primitive.cylinder.constructionMode = CylinderSpec::ConstructionMode::center_radius_height;
        primitiveFeature->primitive.cylinder.center = {centerNode.id};
    }
    descriptor.nodes.push_back(centerNode);
    return descriptor;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricCylinderFromCenterRadiusPointHeight(
    const scene_contract::Vec3f& center,
    const scene_contract::Vec3f& radiusPoint,
    float height,
    std::uint32_t segments)
{
    const float radius = radiusInCylinderPlane(radiusPoint, center);
    auto descriptor = makeParametricObject(makeCylinderDescriptor(radius, height, segments));
    auto centerNode = makePointNode(center);
    auto radiusNode = makePointNode(radiusPoint);
    auto* primitiveFeature = descriptor.features.empty() ? nullptr : &descriptor.features.front();
    if (primitiveFeature != nullptr && primitiveFeature->kind == FeatureKind::primitive) {
        primitiveFeature->primitive.cylinder.constructionMode = CylinderSpec::ConstructionMode::center_radius_point_height;
        primitiveFeature->primitive.cylinder.center = {centerNode.id};
        primitiveFeature->primitive.cylinder.radiusPoint = {radiusNode.id};
    }
    descriptor.nodes.push_back(centerNode);
    descriptor.nodes.push_back(radiusNode);
    return descriptor;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricCylinderFromAxisEndpointsRadius(
    const scene_contract::Vec3f& axisStart,
    const scene_contract::Vec3f& axisEnd,
    float radius,
    std::uint32_t segments)
{
    const float height = lengthVec3(subtractVec3(axisEnd, axisStart));
    auto descriptor = makeParametricObject(makeCylinderDescriptor(radius, height, segments));
    auto startNode = makePointNode(axisStart);
    auto endNode = makePointNode(axisEnd);
    auto* primitiveFeature = descriptor.features.empty() ? nullptr : &descriptor.features.front();
    if (primitiveFeature != nullptr && primitiveFeature->kind == FeatureKind::primitive) {
        primitiveFeature->primitive.cylinder.constructionMode = CylinderSpec::ConstructionMode::axis_endpoints_radius;
        primitiveFeature->primitive.cylinder.axisStart = {startNode.id};
        primitiveFeature->primitive.cylinder.axisEnd = {endNode.id};
    }
    descriptor.nodes.push_back(startNode);
    descriptor.nodes.push_back(endNode);
    return descriptor;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricSphereFromCenterRadius(
    const scene_contract::Vec3f& center,
    float radius,
    std::uint32_t slices,
    std::uint32_t stacks)
{
    auto descriptor = makeParametricObject(makeSphereDescriptor(radius, slices, stacks));
    auto centerNode = makePointNode(center);
    auto* primitiveFeature = descriptor.features.empty() ? nullptr : &descriptor.features.front();
    if (primitiveFeature != nullptr && primitiveFeature->kind == FeatureKind::primitive) {
        primitiveFeature->primitive.sphere.constructionMode = SphereSpec::ConstructionMode::center_radius;
        primitiveFeature->primitive.sphere.center = {centerNode.id};
    }
    descriptor.nodes.push_back(centerNode);
    return descriptor;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricSphereFromCenterSurfacePoint(
    const scene_contract::Vec3f& center,
    const scene_contract::Vec3f& surfacePoint,
    std::uint32_t slices,
    std::uint32_t stacks)
{
    const float radius = lengthVec3(subtractVec3(surfacePoint, center));
    auto descriptor = makeParametricObject(makeSphereDescriptor(radius, slices, stacks));
    auto centerNode = makePointNode(center);
    auto surfaceNode = makePointNode(surfacePoint);
    auto* primitiveFeature = descriptor.features.empty() ? nullptr : &descriptor.features.front();
    if (primitiveFeature != nullptr && primitiveFeature->kind == FeatureKind::primitive) {
        primitiveFeature->primitive.sphere.constructionMode = SphereSpec::ConstructionMode::center_surface_point;
        primitiveFeature->primitive.sphere.center = {centerNode.id};
        primitiveFeature->primitive.sphere.surfacePoint = {surfaceNode.id};
    }
    descriptor.nodes.push_back(centerNode);
    descriptor.nodes.push_back(surfaceNode);
    return descriptor;
}

ParametricObjectDescriptor PrimitiveFactory::makeParametricSphereFromDiameterPoints(
    const scene_contract::Vec3f& diameterStart,
    const scene_contract::Vec3f& diameterEnd,
    std::uint32_t slices,
    std::uint32_t stacks)
{
    const float radius = lengthVec3(subtractVec3(diameterEnd, diameterStart)) * 0.5F;
    auto descriptor = makeParametricObject(makeSphereDescriptor(radius, slices, stacks));
    auto startNode = makePointNode(diameterStart);
    auto endNode = makePointNode(diameterEnd);
    auto* primitiveFeature = descriptor.features.empty() ? nullptr : &descriptor.features.front();
    if (primitiveFeature != nullptr && primitiveFeature->kind == FeatureKind::primitive) {
        primitiveFeature->primitive.sphere.constructionMode = SphereSpec::ConstructionMode::diameter_points;
        primitiveFeature->primitive.sphere.diameterStart = {startNode.id};
        primitiveFeature->primitive.sphere.diameterEnd = {endNode.id};
    }
    descriptor.nodes.push_back(startNode);
    descriptor.nodes.push_back(endNode);
    return descriptor;
}

EvaluationResult PrimitiveFactory::evaluate(const PrimitiveDescriptor& descriptor) {
    return ParametricEvaluator::evaluate(descriptor);
}

EvaluationResult PrimitiveFactory::evaluate(const ParametricObjectDescriptor& descriptor) {
    return ParametricEvaluator::evaluate(descriptor);
}

scene_contract::MeshData PrimitiveFactory::build(const PrimitiveDescriptor& descriptor) {
    return evaluate(descriptor).mesh;
}

scene_contract::MeshData PrimitiveFactory::build(const ParametricObjectDescriptor& descriptor) {
    return evaluate(descriptor).mesh;
}

scene_contract::MeshData PrimitiveFactory::makeBox(
    float width,
    float height,
    float depth)
{
    return build(makeBoxDescriptor(width, height, depth));
}

scene_contract::MeshData PrimitiveFactory::makeCylinder(
    float radius,
    float height,
    std::uint32_t segments)
{
    return build(makeCylinderDescriptor(radius, height, segments));
}

scene_contract::MeshData PrimitiveFactory::makeSphere(
    float radius,
    std::uint32_t slices,
    std::uint32_t stacks)
{
    return build(makeSphereDescriptor(radius, slices, stacks));
}

}  // namespace renderer::parametric_model
