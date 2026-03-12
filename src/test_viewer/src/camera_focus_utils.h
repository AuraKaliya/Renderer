#pragma once

#include "renderer/scene_contract/types.h"

namespace camera_focus {

struct FocusSettings {
    renderer::scene_contract::Vec3f orbitCenter {};
    float distance = 1.5F;
    float nearPlane = 0.01F;
    float farPlane = 10.0F;
};

[[nodiscard]] FocusSettings makeFocusSettingsForBounds(
    const renderer::scene_contract::Aabb& bounds,
    int viewportWidth,
    int viewportHeight,
    float verticalFovDegrees);

[[nodiscard]] FocusSettings makeFocusSettingsForPoint(
    const renderer::scene_contract::Vec3f& point,
    float currentDistance);

}  // namespace camera_focus
