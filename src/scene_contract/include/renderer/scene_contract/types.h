#pragma once

#include <cstdint>
#include <vector>

namespace renderer::scene_contract {

using MeshHandle = std::uint32_t;
constexpr MeshHandle kInvalidMeshHandle = 0U;
using MaterialHandle = std::uint32_t;
constexpr MaterialHandle kInvalidMaterialHandle = 0U;

struct Vec2f {
    float x = 0.0F;
    float y = 0.0F;
};

struct Vec3f {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

struct Vec4f {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;
};

struct Mat4f {
    float elements[16] = {
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };
};

struct ColorRgba {
    float r = 1.0F;
    float g = 1.0F;
    float b = 1.0F;
    float a = 1.0F;
};

struct VertexPNT {
    Vec3f position {};
    Vec3f normal {};
    Vec2f texcoord {};
};

struct MeshData {
    std::vector<VertexPNT> vertices;
    std::vector<std::uint32_t> indices;
};

struct MaterialData {
    ColorRgba baseColor {};
};

struct TransformData {
    Mat4f world {};
};

struct CameraData {
    Mat4f view {};
    Mat4f projection {};
    Vec3f position {};
};

struct RenderableItem {
    MeshHandle meshHandle = kInvalidMeshHandle;
    MaterialHandle materialHandle = kInvalidMaterialHandle;
    TransformData transform;
};

struct FrameScene {
    CameraData camera;
    std::vector<RenderableItem> items;
};

struct RenderTargetDesc {
    std::int32_t width = 0;
    std::int32_t height = 0;
};

}  // namespace renderer::scene_contract
