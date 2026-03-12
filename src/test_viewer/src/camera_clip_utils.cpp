#include "camera_clip_utils.h"

#include <algorithm>

#include "renderer/scene_contract/math_utils.h"

namespace {

constexpr float kMinNearPlane = 0.01F;
constexpr float kBoundsClipPadding = 1.2F;
constexpr float kMinRadius = 0.25F;

renderer::scene_contract::Vec3f aabbHalfExtent(const renderer::scene_contract::Aabb& bounds) {
    return {
        (bounds.max.x - bounds.min.x) * 0.5F,
        (bounds.max.y - bounds.min.y) * 0.5F,
        (bounds.max.z - bounds.min.z) * 0.5F
    };
}

float boundsRadius(const renderer::scene_contract::Aabb& bounds) {
    const auto halfExtent = aabbHalfExtent(bounds);
    return std::max(kMinRadius, renderer::scene_contract::math::length(halfExtent));
}

}  // namespace

camera_clip::ClipRange camera_clip::makeClipRangeForBounds(
    const renderer::scene_contract::Aabb& bounds,
    float distanceToCenter)
{
    ClipRange clipRange;
    const float radius = boundsRadius(bounds);
    clipRange.nearPlane = std::max(kMinNearPlane, distanceToCenter - radius * kBoundsClipPadding);
    clipRange.farPlane = std::max(clipRange.nearPlane + 1.0F, distanceToCenter + radius * kBoundsClipPadding);
    return clipRange;
}

camera_clip::ClipRange camera_clip::makeClipRangeForPoint(float distanceToPoint) {
    ClipRange clipRange;
    clipRange.nearPlane = std::max(kMinNearPlane, distanceToPoint * 0.05F);
    clipRange.farPlane = std::max(clipRange.nearPlane + 1.0F, distanceToPoint * 4.0F);
    return clipRange;
}
