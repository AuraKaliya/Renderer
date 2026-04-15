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
        primitiveFeature->primitive.box.center = {centerNode.id};
    }
    descriptor.nodes.push_back(centerNode);
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
        primitiveFeature->primitive.cylinder.center = {centerNode.id};
    }
    descriptor.nodes.push_back(centerNode);
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
