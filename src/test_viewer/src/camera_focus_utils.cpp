#include "camera_focus_utils.h"

#include <algorithm>
#include <cmath>

#include "renderer/scene_contract/math_utils.h"

namespace {

constexpr float kMinDistance = 1.5F;
constexpr float kMinNearPlane = 0.01F;
constexpr float kBoundsPadding = 1.15F;
constexpr float kBoundsClipPadding = 1.2F;
constexpr float kMinRadius = 0.25F;

float degreesToRadians(float degrees) {
    return degrees * renderer::scene_contract::math::kPi / 180.0F;
}

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

float fittingHalfAngleRadians(int viewportWidth, int viewportHeight, float verticalFovDegrees) {
    const float safeVerticalFovDegrees = std::clamp(verticalFovDegrees, 20.0F, 90.0F);
    const float verticalHalfAngle = degreesToRadians(safeVerticalFovDegrees) * 0.5F;
    const float aspectRatio =
        static_cast<float>(std::max(viewportWidth, 1)) / static_cast<float>(std::max(viewportHeight, 1));
    const float horizontalHalfAngle = std::atan(std::tan(verticalHalfAngle) * aspectRatio);
    return std::min(verticalHalfAngle, horizontalHalfAngle);
}

float boundsRadius(const renderer::scene_contract::Aabb& bounds) {
    const auto halfExtent = aabbHalfExtent(bounds);
    return std::max(kMinRadius, renderer::scene_contract::math::length(halfExtent));
}

}  // namespace

camera_focus::FocusSettings camera_focus::makeFocusSettingsForBounds(
    const renderer::scene_contract::Aabb& bounds,
    int viewportWidth,
    int viewportHeight,
    float verticalFovDegrees)
{
    FocusSettings settings;
    settings.orbitCenter = aabbCenter(bounds);

    const float radius = boundsRadius(bounds);
    const float halfAngle = fittingHalfAngleRadians(viewportWidth, viewportHeight, verticalFovDegrees);
    const float distance = (radius * kBoundsPadding) / std::max(std::sin(halfAngle), 0.1F);

    settings.distance = std::max(kMinDistance, distance);
    settings.nearPlane = std::max(kMinNearPlane, settings.distance - radius * kBoundsClipPadding);
    settings.farPlane = std::max(settings.nearPlane + 1.0F, settings.distance + radius * kBoundsClipPadding);
    return settings;
}

camera_focus::FocusSettings camera_focus::makeFocusSettingsForPoint(
    const renderer::scene_contract::Vec3f& point,
    float currentDistance)
{
    FocusSettings settings;
    settings.orbitCenter = point;
    settings.distance = std::max(kMinDistance, currentDistance);
    settings.nearPlane = std::max(kMinNearPlane, settings.distance * 0.05F);
    settings.farPlane = std::max(settings.nearPlane + 1.0F, settings.distance * 4.0F);
    return settings;
}
