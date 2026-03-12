#pragma once

#include "renderer/scene_contract/types.h"

namespace camera_focus {

enum class ProjectionMode {
    perspective,
    orthographic
};

struct FocusSettings {
    renderer::scene_contract::Vec3f orbitCenter {};
    float distance = 1.5F;
    float orthographicHeight = 8.0F;
};

[[nodiscard]] FocusSettings makeFocusSettingsForBounds(
    const renderer::scene_contract::Aabb& bounds,
    ProjectionMode projectionMode,
    int viewportWidth,
    int viewportHeight,
    float verticalFovDegrees,
    float currentDistance,
    float currentOrthographicHeight);

[[nodiscard]] FocusSettings makeFocusSettingsForPoint(
    const renderer::scene_contract::Vec3f& point,
    ProjectionMode projectionMode,
    float currentDistance,
    float currentOrthographicHeight);

// Compatibility wrapper for the current perspective path.
[[nodiscard]] FocusSettings makeFocusSettingsForBoundsPerspective(
    const renderer::scene_contract::Aabb& bounds,
    int viewportWidth,
    int viewportHeight,
    float verticalFovDegrees);

// Compatibility wrapper for the current perspective path.
[[nodiscard]] FocusSettings makeFocusSettingsForPointPerspective(
    const renderer::scene_contract::Vec3f& point,
    float currentDistance);

// Legacy wrappers kept for incremental migration.
[[nodiscard]] FocusSettings makeFocusSettingsForBounds(
    const renderer::scene_contract::Aabb& bounds,
    int viewportWidth,
    int viewportHeight,
    float verticalFovDegrees);

// Legacy wrappers kept for incremental migration.
[[nodiscard]] FocusSettings makeFocusSettingsForPoint(
    const renderer::scene_contract::Vec3f& point,
    float currentDistance);

}  // namespace camera_focus
