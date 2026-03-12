#include "camera_focus_bounds_utils.h"

#include <algorithm>

#include "renderer/scene_contract/math_utils.h"

namespace {

renderer::scene_contract::Vec3f aabbCenter(const renderer::scene_contract::Aabb& bounds) {
    return {
        (bounds.min.x + bounds.max.x) * 0.5F,
        (bounds.min.y + bounds.max.y) * 0.5F,
        (bounds.min.z + bounds.max.z) * 0.5F
    };
}

renderer::scene_contract::Vec3f aabbHalfExtent(const renderer::scene_contract::Aabb& bounds) {
    return {
        (bounds.max.x - bounds.min.x) * 0.5F,
        (bounds.max.y - bounds.min.y) * 0.5F,
        (bounds.max.z - bounds.min.z) * 0.5F
    };
}

renderer::scene_contract::Vec3f transformPoint(
    const renderer::scene_contract::Mat4f& matrix,
    const renderer::scene_contract::Vec3f& point)
{
    return {
        matrix.elements[0] * point.x + matrix.elements[4] * point.y + matrix.elements[8] * point.z + matrix.elements[12],
        matrix.elements[1] * point.x + matrix.elements[5] * point.y + matrix.elements[9] * point.z + matrix.elements[13],
        matrix.elements[2] * point.x + matrix.elements[6] * point.y + matrix.elements[10] * point.z + matrix.elements[14]
    };
}

float axisScaleLength(const renderer::scene_contract::Mat4f& matrix, int columnOffset) {
    const renderer::scene_contract::Vec3f axis = {
        matrix.elements[columnOffset + 0],
        matrix.elements[columnOffset + 1],
        matrix.elements[columnOffset + 2]
    };
    return renderer::scene_contract::math::length(axis);
}

float maxAxisScale(const renderer::scene_contract::Mat4f& matrix) {
    return std::max({
        axisScaleLength(matrix, 0),
        axisScaleLength(matrix, 4),
        axisScaleLength(matrix, 8)
    });
}

}  // namespace

renderer::scene_contract::Aabb camera_focus_bounds::mergeFocusBounds(
    const renderer::scene_contract::Aabb& left,
    const renderer::scene_contract::Aabb& right)
{
    if (!left.valid) {
        return right;
    }
    if (!right.valid) {
        return left;
    }

    renderer::scene_contract::Aabb merged;
    merged.min = {
        std::min(left.min.x, right.min.x),
        std::min(left.min.y, right.min.y),
        std::min(left.min.z, right.min.z)
    };
    merged.max = {
        std::max(left.max.x, right.max.x),
        std::max(left.max.y, right.max.y),
        std::max(left.max.z, right.max.z)
    };
    merged.valid = true;
    return merged;
}

renderer::scene_contract::Aabb camera_focus_bounds::makeStableFocusBounds(
    const renderer::scene_contract::Aabb& localBounds,
    const renderer::scene_contract::TransformData& transform)
{
    if (!localBounds.valid) {
        return {};
    }

    const auto localCenter = aabbCenter(localBounds);
    const auto worldCenter = transformPoint(transform.world, localCenter);
    const auto halfExtent = aabbHalfExtent(localBounds);
    const float radius =
        renderer::scene_contract::math::length(halfExtent) * std::max(maxAxisScale(transform.world), 1.0F);

    renderer::scene_contract::Aabb bounds;
    bounds.min = {worldCenter.x - radius, worldCenter.y - radius, worldCenter.z - radius};
    bounds.max = {worldCenter.x + radius, worldCenter.y + radius, worldCenter.z + radius};
    bounds.valid = true;
    return bounds;
}
