#include "renderer/parametric_model/parametric_evaluator.h"

#include <cmath>
#include <utility>

namespace renderer::parametric_model {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kMinimumDimension = 0.0001F;
constexpr std::uint32_t kMinimumCylinderSegments = 3U;
constexpr std::uint32_t kMinimumSphereSlices = 3U;
constexpr std::uint32_t kMinimumSphereStacks = 2U;

void appendDiagnostic(
    std::vector<EvaluationDiagnostic>& diagnostics,
    EvaluationDiagnosticSeverity severity,
    EvaluationDiagnosticCode code,
    std::string message,
    ParametricFeatureId featureId = 0U,
    ParametricNodeId nodeId = 0U)
{
    EvaluationDiagnostic diagnostic;
    diagnostic.severity = severity;
    diagnostic.code = code;
    diagnostic.featureId = featureId;
    diagnostic.nodeId = nodeId;
    diagnostic.message = std::move(message);
    diagnostics.push_back(std::move(diagnostic));
}

renderer::scene_contract::VertexPNT makeVertex(
    float px,
    float py,
    float pz,
    float nx,
    float ny,
    float nz,
    float u,
    float v)
{
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
    const scene_contract::VertexPNT& c)
{
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

void translateMesh(scene_contract::MeshData& mesh, const scene_contract::Vec3f& offset) {
    if (offset.x == 0.0F && offset.y == 0.0F && offset.z == 0.0F) {
        return;
    }

    for (auto& vertex : mesh.vertices) {
        vertex.position = addVec3(vertex.position, offset);
    }
    mesh.localBounds = computeBoundsFromVertices(mesh.vertices);
}

const ParametricNodeDescriptor* findNodeById(
    const std::vector<ParametricNodeDescriptor>& nodes,
    ParametricNodeId id)
{
    for (const auto& node : nodes) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

scene_contract::Vec3f resolvePointNode(
    const std::vector<ParametricNodeDescriptor>& nodes,
    const NodeReference& reference,
    const scene_contract::Vec3f& fallback,
    std::vector<EvaluationDiagnostic>* diagnostics = nullptr,
    ParametricFeatureId featureId = 0U)
{
    if (reference.id == 0U) {
        return fallback;
    }

    const auto* node = findNodeById(nodes, reference.id);
    if (node == nullptr) {
        if (diagnostics != nullptr) {
            appendDiagnostic(
                *diagnostics,
                EvaluationDiagnosticSeverity::warning,
                EvaluationDiagnosticCode::missing_node,
                "Point node reference was not found; fallback position was used.",
                featureId,
                reference.id);
        }
        return fallback;
    }

    if (node->kind != ParametricNodeKind::point) {
        if (diagnostics != nullptr) {
            appendDiagnostic(
                *diagnostics,
                EvaluationDiagnosticSeverity::warning,
                EvaluationDiagnosticCode::missing_node,
                "Node reference did not target a point node; fallback position was used.",
                featureId,
                reference.id);
        }
        return fallback;
    }
    return node->point.position;
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

BoxSpec normalizeBoxSpec(
    BoxSpec spec,
    std::vector<EvaluationDiagnostic>& diagnostics,
    ParametricFeatureId featureId)
{
    if (spec.width < kMinimumDimension) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Box width was clamped to the minimum dimension.",
            featureId);
    }
    if (spec.height < kMinimumDimension) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Box height was clamped to the minimum dimension.",
            featureId);
    }
    if (spec.depth < kMinimumDimension) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Box depth was clamped to the minimum dimension.",
            featureId);
    }
    spec.width = clampDimension(spec.width);
    spec.height = clampDimension(spec.height);
    spec.depth = clampDimension(spec.depth);
    return spec;
}

CylinderSpec normalizeCylinderSpec(
    CylinderSpec spec,
    std::vector<EvaluationDiagnostic>& diagnostics,
    ParametricFeatureId featureId)
{
    if (spec.radius < kMinimumDimension) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Cylinder radius was clamped to the minimum dimension.",
            featureId);
    }
    if (spec.height < kMinimumDimension) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Cylinder height was clamped to the minimum dimension.",
            featureId);
    }
    if (spec.segments < kMinimumCylinderSegments) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Cylinder segments were clamped to the minimum segment count.",
            featureId);
    }
    spec.radius = clampDimension(spec.radius);
    spec.height = clampDimension(spec.height);
    if (spec.segments < kMinimumCylinderSegments) {
        spec.segments = kMinimumCylinderSegments;
    }
    return spec;
}

SphereSpec normalizeSphereSpec(
    SphereSpec spec,
    std::vector<EvaluationDiagnostic>& diagnostics,
    ParametricFeatureId featureId)
{
    if (spec.radius < kMinimumDimension) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Sphere radius was clamped to the minimum dimension.",
            featureId);
    }
    if (spec.slices < kMinimumSphereSlices) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Sphere slices were clamped to the minimum slice count.",
            featureId);
    }
    if (spec.stacks < kMinimumSphereStacks) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Sphere stacks were clamped to the minimum stack count.",
            featureId);
    }
    spec.radius = clampDimension(spec.radius);
    if (spec.slices < kMinimumSphereSlices) {
        spec.slices = kMinimumSphereSlices;
    }
    if (spec.stacks < kMinimumSphereStacks) {
        spec.stacks = kMinimumSphereStacks;
    }
    return spec;
}

LinearArrayOperatorSpec normalizeLinearArraySpec(
    LinearArrayOperatorSpec spec,
    std::vector<EvaluationDiagnostic>& diagnostics,
    ParametricFeatureId featureId)
{
    if (spec.count < 1U) {
        appendDiagnostic(
            diagnostics,
            EvaluationDiagnosticSeverity::warning,
            EvaluationDiagnosticCode::value_clamped,
            "Linear array count was clamped to one.",
            featureId);
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

scene_contract::MeshData buildBoxMesh(
    BoxSpec spec,
    const std::vector<ParametricNodeDescriptor>& nodes,
    std::vector<EvaluationDiagnostic>& diagnostics,
    ParametricFeatureId featureId)
{
    const auto center = resolvePointNode(nodes, spec.center, {}, &diagnostics, featureId);
    auto mesh = buildBoxMesh(normalizeBoxSpec(spec, diagnostics, featureId));
    translateMesh(mesh, center);
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

scene_contract::MeshData buildCylinderMesh(
    CylinderSpec spec,
    const std::vector<ParametricNodeDescriptor>& nodes,
    std::vector<EvaluationDiagnostic>& diagnostics,
    ParametricFeatureId featureId)
{
    const auto center = resolvePointNode(nodes, spec.center, {}, &diagnostics, featureId);
    auto mesh = buildCylinderMesh(normalizeCylinderSpec(spec, diagnostics, featureId));
    translateMesh(mesh, center);
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

scene_contract::MeshData buildSphereMesh(
    SphereSpec spec,
    const std::vector<ParametricNodeDescriptor>& nodes,
    std::vector<EvaluationDiagnostic>& diagnostics,
    ParametricFeatureId featureId)
{
    scene_contract::Vec3f center = resolvePointNode(nodes, spec.center, {}, &diagnostics, featureId);
    if (spec.constructionMode == SphereSpec::ConstructionMode::center_surface_point) {
        const scene_contract::Vec3f defaultSurfacePoint = {
            center.x + spec.radius,
            center.y,
            center.z
        };
        const auto surfacePoint = resolvePointNode(
            nodes,
            spec.surfacePoint,
            defaultSurfacePoint,
            &diagnostics,
            featureId);
        spec.radius = lengthVec3(subtractVec3(surfacePoint, center));
    }

    auto mesh = buildSphereMesh(normalizeSphereSpec(spec, diagnostics, featureId));
    translateMesh(mesh, center);
    return mesh;
}

scene_contract::MeshData buildPrimitiveMesh(
    const PrimitiveDescriptor& descriptor,
    const std::vector<ParametricNodeDescriptor>& nodes,
    std::vector<EvaluationDiagnostic>& diagnostics,
    ParametricFeatureId featureId = 0U)
{
    switch (descriptor.kind) {
    case PrimitiveKind::box:
        return buildBoxMesh(descriptor.box, nodes, diagnostics, featureId);
    case PrimitiveKind::cylinder:
        return buildCylinderMesh(descriptor.cylinder, nodes, diagnostics, featureId);
    case PrimitiveKind::sphere:
        return buildSphereMesh(descriptor.sphere, nodes, diagnostics, featureId);
    }

    return {};
}

}  // namespace

EvaluationResult ParametricEvaluator::evaluate(const PrimitiveDescriptor& descriptor) {
    EvaluationResult result;
    result.mesh = buildPrimitiveMesh(descriptor, {}, result.diagnostics);

    if (result.mesh.vertices.empty()) {
        appendDiagnostic(
            result.diagnostics,
            EvaluationDiagnosticSeverity::error,
            EvaluationDiagnosticCode::empty_model,
            "Primitive evaluation did not produce renderable mesh data.");
        result.succeeded = false;
    }

    return result;
}

EvaluationResult ParametricEvaluator::evaluate(const ParametricObjectDescriptor& descriptor) {
    EvaluationResult result;
    bool hasBasePrimitive = false;

    for (const auto& feature : descriptor.features) {
        if (!feature.enabled) {
            continue;
        }

        switch (feature.kind) {
        case FeatureKind::primitive:
            result.mesh = buildPrimitiveMesh(
                feature.primitive,
                descriptor.nodes,
                result.diagnostics,
                feature.id);
            hasBasePrimitive = true;
            break;
        case FeatureKind::mirror:
            if (!hasBasePrimitive) {
                appendDiagnostic(
                    result.diagnostics,
                    EvaluationDiagnosticSeverity::warning,
                    EvaluationDiagnosticCode::invalid_feature_order,
                    "Mirror feature was skipped because no base primitive had been evaluated.",
                    feature.id);
                continue;
            }
            result.mesh = applyMirror(result.mesh, feature.mirror);
            break;
        case FeatureKind::linear_array:
            if (!hasBasePrimitive) {
                appendDiagnostic(
                    result.diagnostics,
                    EvaluationDiagnosticSeverity::warning,
                    EvaluationDiagnosticCode::invalid_feature_order,
                    "Linear array feature was skipped because no base primitive had been evaluated.",
                    feature.id);
                continue;
            }
            result.mesh = applyLinearArray(
                result.mesh,
                normalizeLinearArraySpec(feature.linearArray, result.diagnostics, feature.id));
            break;
        }
    }

    if (!result.mesh.vertices.empty()) {
        result.mesh.localBounds = computeBoundsFromVertices(result.mesh.vertices);
    } else {
        appendDiagnostic(
            result.diagnostics,
            EvaluationDiagnosticSeverity::error,
            EvaluationDiagnosticCode::empty_model,
            "Parametric object evaluation did not produce renderable mesh data.");
        result.succeeded = false;
    }

    return result;
}

}  // namespace renderer::parametric_model
