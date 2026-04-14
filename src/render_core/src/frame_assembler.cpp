#include "renderer/render_core/frame_assembler.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include "renderer/scene_contract/math_utils.h"

namespace renderer::render_core {

namespace {

struct ClipPoint {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;
};

scene_contract::Vec3f aabbCenter(const scene_contract::Aabb& bounds) {
    return {
        (bounds.min.x + bounds.max.x) * 0.5F,
        (bounds.min.y + bounds.max.y) * 0.5F,
        (bounds.min.z + bounds.max.z) * 0.5F
    };
}

scene_contract::Vec3f itemCenter(const scene_contract::RenderableItem& item) {
    if (item.worldBounds.valid) {
        return aabbCenter(item.worldBounds);
    }

    return {
        item.transform.world.elements[12],
        item.transform.world.elements[13],
        item.transform.world.elements[14]
    };
}

float distanceSquared(
    const scene_contract::Vec3f& left,
    const scene_contract::Vec3f& right)
{
    const auto delta = scene_contract::math::subtract(left, right);
    return scene_contract::math::dot(delta, delta);
}

ClipPoint transformPoint(
    const scene_contract::Mat4f& matrix,
    const scene_contract::Vec3f& point)
{
    return {
        matrix.elements[0] * point.x + matrix.elements[4] * point.y + matrix.elements[8] * point.z + matrix.elements[12],
        matrix.elements[1] * point.x + matrix.elements[5] * point.y + matrix.elements[9] * point.z + matrix.elements[13],
        matrix.elements[2] * point.x + matrix.elements[6] * point.y + matrix.elements[10] * point.z + matrix.elements[14],
        matrix.elements[3] * point.x + matrix.elements[7] * point.y + matrix.elements[11] * point.z + matrix.elements[15]
    };
}

std::array<scene_contract::Vec3f, 8> aabbCorners(const scene_contract::Aabb& bounds) {
    return {{
        {bounds.min.x, bounds.min.y, bounds.min.z},
        {bounds.max.x, bounds.min.y, bounds.min.z},
        {bounds.min.x, bounds.max.y, bounds.min.z},
        {bounds.max.x, bounds.max.y, bounds.min.z},
        {bounds.min.x, bounds.min.y, bounds.max.z},
        {bounds.max.x, bounds.min.y, bounds.max.z},
        {bounds.min.x, bounds.max.y, bounds.max.z},
        {bounds.max.x, bounds.max.y, bounds.max.z}
    }};
}

bool allOutsideLeft(const std::array<ClipPoint, 8>& points) {
    return std::all_of(points.begin(), points.end(), [](const ClipPoint& point) {
        return point.x < -point.w;
    });
}

bool allOutsideRight(const std::array<ClipPoint, 8>& points) {
    return std::all_of(points.begin(), points.end(), [](const ClipPoint& point) {
        return point.x > point.w;
    });
}

bool allOutsideBottom(const std::array<ClipPoint, 8>& points) {
    return std::all_of(points.begin(), points.end(), [](const ClipPoint& point) {
        return point.y < -point.w;
    });
}

bool allOutsideTop(const std::array<ClipPoint, 8>& points) {
    return std::all_of(points.begin(), points.end(), [](const ClipPoint& point) {
        return point.y > point.w;
    });
}

bool allOutsideNear(const std::array<ClipPoint, 8>& points) {
    return std::all_of(points.begin(), points.end(), [](const ClipPoint& point) {
        return point.z < -point.w;
    });
}

bool allOutsideFar(const std::array<ClipPoint, 8>& points) {
    return std::all_of(points.begin(), points.end(), [](const ClipPoint& point) {
        return point.z > point.w;
    });
}

bool visibleInFrustum(
    const scene_contract::Aabb& bounds,
    const scene_contract::Mat4f& viewProjection)
{
    if (!bounds.valid) {
        return true;
    }

    const auto corners = aabbCorners(bounds);
    std::array<ClipPoint, 8> clipPoints {};
    for (std::size_t index = 0; index < corners.size(); ++index) {
        clipPoints[index] = transformPoint(viewProjection, corners[index]);
    }

    return !allOutsideLeft(clipPoints) &&
        !allOutsideRight(clipPoints) &&
        !allOutsideBottom(clipPoints) &&
        !allOutsideTop(clipPoints) &&
        !allOutsideNear(clipPoints) &&
        !allOutsideFar(clipPoints);
}

}  // namespace

FramePacket FrameAssembler::build(
    const scene_contract::FrameScene& scene,
    const scene_contract::RenderTargetDesc& target,
    FrameBuildOptions options) const {
    FramePacket packet;
    packet.camera = scene.camera;
    packet.light = scene.light;
    packet.target = target;

    const auto viewProjection =
        scene_contract::math::multiply(scene.camera.projection, scene.camera.view);

    packet.opaqueItems.reserve(scene.items.size());
    for (const auto& item : scene.items) {
        if (!item.visible) {
            continue;
        }
        if (item.meshHandle == scene_contract::kInvalidMeshHandle ||
            item.materialHandle == scene_contract::kInvalidMaterialHandle) {
            continue;
        }
        if (options.enableFrustumCulling &&
            !visibleInFrustum(item.worldBounds, viewProjection)) {
            continue;
        }

        RenderQueueItem queueItem;
        queueItem.item = item;
        queueItem.cameraDistanceSquared =
            distanceSquared(scene.camera.position, itemCenter(item));
        packet.opaqueItems.push_back(queueItem);
    }

    if (options.sortOpaqueFrontToBack) {
        std::sort(
            packet.opaqueItems.begin(),
            packet.opaqueItems.end(),
            [](const RenderQueueItem& left, const RenderQueueItem& right) {
                if (left.cameraDistanceSquared != right.cameraDistanceSquared) {
                    return left.cameraDistanceSquared < right.cameraDistanceSquared;
                }
                if (left.item.materialHandle != right.item.materialHandle) {
                    return left.item.materialHandle < right.item.materialHandle;
                }
                return left.item.meshHandle < right.item.meshHandle;
            });
    }

    return packet;
}

}  // namespace renderer::render_core
