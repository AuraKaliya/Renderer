#include "renderer/parametric_model/primitive_factory.h"

#include <cmath>

namespace renderer::parametric_model {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kMinimumDimension = 0.0001F;
constexpr std::uint32_t kMinimumCylinderSegments = 3U;
constexpr std::uint32_t kMinimumSphereSlices = 3U;
constexpr std::uint32_t kMinimumSphereStacks = 2U;

ParametricObjectId nextParametricObjectId() {
    static ParametricObjectId nextId = 1U;
    return nextId++;
}

ParametricFeatureId nextParametricFeatureId() {
    static ParametricFeatureId nextId = 1U;
    return nextId++;
}

renderer::scene_contract::VertexPNT makeVertex(
    float px,
    float py,
    float pz,
    float nx,
    float ny,
    float nz,
    float u,
    float v) {
    renderer::scene_contract::VertexPNT vertex;
    vertex.position = {px, py, pz};
    vertex.normal = {nx, ny, nz};
    vertex.texcoord = {u, v};
    return vertex;
}

void appendTriangle(
    scene_contract::MeshData& mesh,
    const scene_contract::VertexPNT& a,
    const scene_contract::VertexPNT& b,
    const scene_contract::VertexPNT& c) {
    const auto baseIndex = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(a);
    mesh.vertices.push_back(b);
    mesh.vertices.push_back(c);
    mesh.indices.push_back(baseIndex);
    mesh.indices.push_back(baseIndex + 1U);
    mesh.indices.push_back(baseIndex + 2U);
}

scene_contract::Aabb makeAabb(
    float minX,
    float minY,
    float minZ,
    float maxX,
    float maxY,
    float maxZ)
{
    scene_contract::Aabb bounds;
    bounds.min = {minX, minY, minZ};
    bounds.max = {maxX, maxY, maxZ};
    bounds.valid = true;
    return bounds;
}

scene_contract::Aabb computeBoundsFromVertices(const std::vector<scene_contract::VertexPNT>& vertices) {
    scene_contract::Aabb bounds;
    if (vertices.empty()) {
        return bounds;
    }

    bounds.min = vertices.front().position;
    bounds.max = vertices.front().position;
    bounds.valid = true;

    for (const auto& vertex : vertices) {
        const auto& position = vertex.position;
        if (position.x < bounds.min.x) {
            bounds.min.x = position.x;
        }
        if (position.y < bounds.min.y) {
            bounds.min.y = position.y;
        }
        if (position.z < bounds.min.z) {
            bounds.min.z = position.z;
        }
        if (position.x > bounds.max.x) {
            bounds.max.x = position.x;
        }
        if (position.y > bounds.max.y) {
            bounds.max.y = position.y;
        }
        if (position.z > bounds.max.z) {
            bounds.max.z = position.z;
        }
    }

    return bounds;
}

scene_contract::Vec3f addVec3(
    const scene_contract::Vec3f& left,
    const scene_contract::Vec3f& right)
{
    return {
        left.x + right.x,
        left.y + right.y,
        left.z + right.z
    };
}

scene_contract::Vec3f mirrorPosition(
    const scene_contract::Vec3f& value,
    Axis axis,
    float planeOffset)
{
    auto mirrored = value;
    switch (axis) {
    case Axis::x:
        mirrored.x = 2.0F * planeOffset - mirrored.x;
        break;
    case Axis::y:
        mirrored.y = 2.0F * planeOffset - mirrored.y;
        break;
    case Axis::z:
        mirrored.z = 2.0F * planeOffset - mirrored.z;
        break;
    }
    return mirrored;
}

scene_contract::Vec3f mirrorNormal(
    const scene_contract::Vec3f& value,
    Axis axis)
{
    auto mirrored = value;
    switch (axis) {
    case Axis::x:
        mirrored.x = -mirrored.x;
        break;
    case Axis::y:
        mirrored.y = -mirrored.y;
        break;
    case Axis::z:
        mirrored.z = -mirrored.z;
        break;
    }
    return mirrored;
}

scene_contract::MeshData applyMirror(
    const scene_contract::MeshData& source,
    const MirrorOperatorSpec& spec)
{
    scene_contract::MeshData mesh = source;
    const std::uint32_t baseVertexCount = static_cast<std::uint32_t>(source.vertices.size());
    const std::uint32_t baseIndexCount = static_cast<std::uint32_t>(source.indices.size());

    mesh.vertices.reserve(source.vertices.size() * 2U);
    mesh.indices.reserve(source.indices.size() * 2U);

    for (const auto& vertex : source.vertices) {
        auto mirrored = vertex;
        mirrored.position = mirrorPosition(vertex.position, spec.axis, spec.planeOffset);
        mirrored.normal = mirrorNormal(vertex.normal, spec.axis);
        mesh.vertices.push_back(mirrored);
    }

    for (std::uint32_t index = 0; index + 2U < baseIndexCount; index += 3U) {
        mesh.indices.push_back(baseVertexCount + source.indices[index]);
        mesh.indices.push_back(baseVertexCount + source.indices[index + 2U]);
        mesh.indices.push_back(baseVertexCount + source.indices[index + 1U]);
    }

    mesh.localBounds = computeBoundsFromVertices(mesh.vertices);
    return mesh;
}

scene_contract::MeshData applyLinearArray(
    const scene_contract::MeshData& source,
    const LinearArrayOperatorSpec& spec)
{
    scene_contract::MeshData mesh;
    const std::uint32_t count = spec.count > 0U ? spec.count : 1U;
    mesh.vertices.reserve(source.vertices.size() * count);
    mesh.indices.reserve(source.indices.size() * count);

    for (std::uint32_t instance = 0; instance < count; ++instance) {
        const scene_contract::Vec3f instanceOffset = {
            spec.offset.x * static_cast<float>(instance),
            spec.offset.y * static_cast<float>(instance),
            spec.offset.z * static_cast<float>(instance)
        };
        const std::uint32_t baseVertex = static_cast<std::uint32_t>(mesh.vertices.size());

        for (const auto& vertex : source.vertices) {
            auto translated = vertex;
            translated.position = addVec3(vertex.position, instanceOffset);
            mesh.vertices.push_back(translated);
        }

        for (const auto index : source.indices) {
            mesh.indices.push_back(baseVertex + index);
        }
    }

    mesh.localBounds = computeBoundsFromVertices(mesh.vertices);
    return mesh;
}

float clampDimension(float value) {
    return value > kMinimumDimension ? value : kMinimumDimension;
}

BoxSpec normalizeBoxSpec(BoxSpec spec) {
    spec.width = clampDimension(spec.width);
    spec.height = clampDimension(spec.height);
    spec.depth = clampDimension(spec.depth);
    return spec;
}

CylinderSpec normalizeCylinderSpec(CylinderSpec spec) {
    spec.radius = clampDimension(spec.radius);
    spec.height = clampDimension(spec.height);
    if (spec.segments < kMinimumCylinderSegments) {
        spec.segments = kMinimumCylinderSegments;
    }
    return spec;
}

SphereSpec normalizeSphereSpec(SphereSpec spec) {
    spec.radius = clampDimension(spec.radius);
    if (spec.slices < kMinimumSphereSlices) {
        spec.slices = kMinimumSphereSlices;
    }
    if (spec.stacks < kMinimumSphereStacks) {
        spec.stacks = kMinimumSphereStacks;
    }
    return spec;
}

LinearArrayOperatorSpec normalizeLinearArraySpec(LinearArrayOperatorSpec spec) {
    if (spec.count < 1U) {
        spec.count = 1U;
    }
    return spec;
}

scene_contract::MeshData buildBoxMesh(const BoxSpec& spec) {
    const float hx = spec.width * 0.5F;
    const float hy = spec.height * 0.5F;
    const float hz = spec.depth * 0.5F;

    scene_contract::MeshData mesh;
    mesh.vertices = {
        makeVertex(-hx, -hy, hz, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F),
        makeVertex(hx, -hy, hz, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F),
        makeVertex(hx, hy, hz, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F),
        makeVertex(-hx, hy, hz, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F),

        makeVertex(hx, -hy, -hz, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F),
        makeVertex(-hx, -hy, -hz, 0.0F, 0.0F, -1.0F, 1.0F, 0.0F),
        makeVertex(-hx, hy, -hz, 0.0F, 0.0F, -1.0F, 1.0F, 1.0F),
        makeVertex(hx, hy, -hz, 0.0F, 0.0F, -1.0F, 0.0F, 1.0F),

        makeVertex(-hx, -hy, -hz, -1.0F, 0.0F, 0.0F, 0.0F, 0.0F),
        makeVertex(-hx, -hy, hz, -1.0F, 0.0F, 0.0F, 1.0F, 0.0F),
        makeVertex(-hx, hy, hz, -1.0F, 0.0F, 0.0F, 1.0F, 1.0F),
        makeVertex(-hx, hy, -hz, -1.0F, 0.0F, 0.0F, 0.0F, 1.0F),

        makeVertex(hx, -hy, hz, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F),
        makeVertex(hx, -hy, -hz, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F),
        makeVertex(hx, hy, -hz, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F),
        makeVertex(hx, hy, hz, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F),

        makeVertex(-hx, hy, hz, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F),
        makeVertex(hx, hy, hz, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F),
        makeVertex(hx, hy, -hz, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F),
        makeVertex(-hx, hy, -hz, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F),

        makeVertex(-hx, -hy, -hz, 0.0F, -1.0F, 0.0F, 0.0F, 0.0F),
        makeVertex(hx, -hy, -hz, 0.0F, -1.0F, 0.0F, 1.0F, 0.0F),
        makeVertex(hx, -hy, hz, 0.0F, -1.0F, 0.0F, 1.0F, 1.0F),
        makeVertex(-hx, -hy, hz, 0.0F, -1.0F, 0.0F, 0.0F, 1.0F)
    };

    mesh.indices = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };
    mesh.localBounds = makeAabb(-hx, -hy, -hz, hx, hy, hz);

    return mesh;
}

scene_contract::MeshData buildCylinderMesh(const CylinderSpec& spec) {
    scene_contract::MeshData mesh;
    const float halfHeight = spec.height * 0.5F;

    for (std::uint32_t segment = 0; segment < spec.segments; ++segment) {
        const float currentAngle = (2.0F * kPi * static_cast<float>(segment)) / static_cast<float>(spec.segments);
        const float nextAngle = (2.0F * kPi * static_cast<float>(segment + 1U)) / static_cast<float>(spec.segments);

        const float currentX = spec.radius * std::cos(currentAngle);
        const float currentZ = spec.radius * std::sin(currentAngle);
        const float nextX = spec.radius * std::cos(nextAngle);
        const float nextZ = spec.radius * std::sin(nextAngle);

        const renderer::scene_contract::Vec3f currentNormal = {
            std::cos(currentAngle),
            0.0F,
            std::sin(currentAngle)
        };
        const renderer::scene_contract::Vec3f nextNormal = {
            std::cos(nextAngle),
            0.0F,
            std::sin(nextAngle)
        };

        appendTriangle(
            mesh,
            makeVertex(currentX, -halfHeight, currentZ, currentNormal.x, currentNormal.y, currentNormal.z, 0.0F, 0.0F),
            makeVertex(nextX, -halfHeight, nextZ, nextNormal.x, nextNormal.y, nextNormal.z, 1.0F, 0.0F),
            makeVertex(nextX, halfHeight, nextZ, nextNormal.x, nextNormal.y, nextNormal.z, 1.0F, 1.0F));

        appendTriangle(
            mesh,
            makeVertex(currentX, -halfHeight, currentZ, currentNormal.x, currentNormal.y, currentNormal.z, 0.0F, 0.0F),
            makeVertex(nextX, halfHeight, nextZ, nextNormal.x, nextNormal.y, nextNormal.z, 1.0F, 1.0F),
            makeVertex(currentX, halfHeight, currentZ, currentNormal.x, currentNormal.y, currentNormal.z, 0.0F, 1.0F));

        appendTriangle(
            mesh,
            makeVertex(0.0F, halfHeight, 0.0F, 0.0F, 1.0F, 0.0F, 0.5F, 0.5F),
            makeVertex(nextX, halfHeight, nextZ, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F),
            makeVertex(currentX, halfHeight, currentZ, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F));

        appendTriangle(
            mesh,
            makeVertex(0.0F, -halfHeight, 0.0F, 0.0F, -1.0F, 0.0F, 0.5F, 0.5F),
            makeVertex(currentX, -halfHeight, currentZ, 0.0F, -1.0F, 0.0F, 0.0F, 0.0F),
            makeVertex(nextX, -halfHeight, nextZ, 0.0F, -1.0F, 0.0F, 1.0F, 0.0F));
    }

    mesh.localBounds = makeAabb(-spec.radius, -halfHeight, -spec.radius, spec.radius, halfHeight, spec.radius);
    return mesh;
}

scene_contract::MeshData buildSphereMesh(const SphereSpec& spec) {
    scene_contract::MeshData mesh;
    for (std::uint32_t stack = 0; stack <= spec.stacks; ++stack) {
        const float v = static_cast<float>(stack) / static_cast<float>(spec.stacks);
        const float phi = kPi * v;
        const float y = std::cos(phi);
        const float ringRadius = std::sin(phi);

        for (std::uint32_t slice = 0; slice <= spec.slices; ++slice) {
            const float u = static_cast<float>(slice) / static_cast<float>(spec.slices);
            const float theta = 2.0F * kPi * u;
            const float x = ringRadius * std::cos(theta);
            const float z = ringRadius * std::sin(theta);

            mesh.vertices.push_back(makeVertex(
                spec.radius * x,
                spec.radius * y,
                spec.radius * z,
                x,
                y,
                z,
                u,
                v));
        }
    }

    const std::uint32_t stride = spec.slices + 1U;
    for (std::uint32_t stack = 0; stack < spec.stacks; ++stack) {
        for (std::uint32_t slice = 0; slice < spec.slices; ++slice) {
            const std::uint32_t current = stack * stride + slice;
            const std::uint32_t next = current + stride;

            mesh.indices.push_back(current);
            mesh.indices.push_back(next);
            mesh.indices.push_back(current + 1U);

            mesh.indices.push_back(current + 1U);
            mesh.indices.push_back(next);
            mesh.indices.push_back(next + 1U);
        }
    }

    mesh.localBounds = computeBoundsFromVertices(mesh.vertices);
    return mesh;
}

}  // namespace

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
    feature.kind = FeatureKind::primitive;
    feature.enabled = true;
    feature.primitive = primitive;
    return feature;
}

FeatureDescriptor PrimitiveFactory::makeMirrorFeature() {
    FeatureDescriptor feature;
    feature.id = nextParametricFeatureId();
    feature.kind = FeatureKind::mirror;
    feature.enabled = false;
    feature.mirror.axis = Axis::x;
    return feature;
}

FeatureDescriptor PrimitiveFactory::makeLinearArrayFeature() {
    FeatureDescriptor feature;
    feature.id = nextParametricFeatureId();
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

scene_contract::MeshData PrimitiveFactory::build(const PrimitiveDescriptor& descriptor) {
    switch (descriptor.kind) {
    case PrimitiveKind::box:
        return buildBoxMesh(normalizeBoxSpec(descriptor.box));
    case PrimitiveKind::cylinder:
        return buildCylinderMesh(normalizeCylinderSpec(descriptor.cylinder));
    case PrimitiveKind::sphere:
        return buildSphereMesh(normalizeSphereSpec(descriptor.sphere));
    }

    return {};
}

scene_contract::MeshData PrimitiveFactory::build(const ParametricObjectDescriptor& descriptor) {
    scene_contract::MeshData mesh;
    bool hasBasePrimitive = false;

    for (const auto& feature : descriptor.features) {
        if (!feature.enabled) {
            continue;
        }

        switch (feature.kind) {
        case FeatureKind::primitive:
            mesh = build(feature.primitive);
            hasBasePrimitive = true;
            break;
        case FeatureKind::mirror:
            if (!hasBasePrimitive) {
                continue;
            }
            mesh = applyMirror(mesh, feature.mirror);
            break;
        case FeatureKind::linear_array:
            if (!hasBasePrimitive) {
                continue;
            }
            mesh = applyLinearArray(mesh, normalizeLinearArraySpec(feature.linearArray));
            break;
        }
    }

    if (!mesh.vertices.empty()) {
        mesh.localBounds = computeBoundsFromVertices(mesh.vertices);
    }
    return mesh;
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
    std::uint32_t segments) {
    return build(makeCylinderDescriptor(radius, height, segments));
}

scene_contract::MeshData PrimitiveFactory::makeSphere(
    float radius,
    std::uint32_t slices,
    std::uint32_t stacks) {
    return build(makeSphereDescriptor(radius, slices, stacks));
}

}  // namespace renderer::parametric_model
