#include "renderer/parametric_model/primitive_factory.h"

#include <cmath>

namespace renderer::parametric_model {
namespace {

constexpr float kPi = 3.14159265358979323846F;

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

}  // namespace

scene_contract::MeshData PrimitiveFactory::makeBox(
    float width,
    float height,
    float depth) {
    const float hx = width * 0.5F;
    const float hy = height * 0.5F;
    const float hz = depth * 0.5F;

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

    return mesh;
}

scene_contract::MeshData PrimitiveFactory::makeCylinder(
    float radius,
    float height,
    std::uint32_t segments) {
    scene_contract::MeshData mesh;
    if (segments < 3U) {
        return mesh;
    }

    const float halfHeight = height * 0.5F;

    for (std::uint32_t segment = 0; segment < segments; ++segment) {
        const float currentAngle = (2.0F * kPi * static_cast<float>(segment)) / static_cast<float>(segments);
        const float nextAngle = (2.0F * kPi * static_cast<float>(segment + 1U)) / static_cast<float>(segments);

        const float currentX = radius * std::cos(currentAngle);
        const float currentZ = radius * std::sin(currentAngle);
        const float nextX = radius * std::cos(nextAngle);
        const float nextZ = radius * std::sin(nextAngle);

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

    return mesh;
}

scene_contract::MeshData PrimitiveFactory::makeSphere(
    float radius,
    std::uint32_t slices,
    std::uint32_t stacks) {
    scene_contract::MeshData mesh;
    if (slices < 3U || stacks < 2U) {
        return mesh;
    }

    for (std::uint32_t stack = 0; stack <= stacks; ++stack) {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi = kPi * v;
        const float y = std::cos(phi);
        const float ringRadius = std::sin(phi);

        for (std::uint32_t slice = 0; slice <= slices; ++slice) {
            const float u = static_cast<float>(slice) / static_cast<float>(slices);
            const float theta = 2.0F * kPi * u;
            const float x = ringRadius * std::cos(theta);
            const float z = ringRadius * std::sin(theta);

            mesh.vertices.push_back(makeVertex(
                radius * x,
                radius * y,
                radius * z,
                x,
                y,
                z,
                u,
                v));
        }
    }

    const std::uint32_t stride = slices + 1U;
    for (std::uint32_t stack = 0; stack < stacks; ++stack) {
        for (std::uint32_t slice = 0; slice < slices; ++slice) {
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

    return mesh;
}

}  // namespace renderer::parametric_model
