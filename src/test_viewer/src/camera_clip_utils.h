#pragma once

#include "renderer/scene_contract/types.h"

namespace camera_clip {

struct ClipRange {
    float nearPlane = 0.01F;
    float farPlane = 10.0F;
};

[[nodiscard]] ClipRange makeClipRangeForBounds(
    const renderer::scene_contract::Aabb& bounds,
    float distanceToCenter);

[[nodiscard]] ClipRange makeClipRangeForPoint(float distanceToPoint);

}  // namespace camera_clip
