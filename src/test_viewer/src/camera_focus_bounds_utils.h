#pragma once

#include "renderer/scene_contract/types.h"

namespace camera_focus_bounds {

[[nodiscard]] renderer::scene_contract::Aabb mergeFocusBounds(
    const renderer::scene_contract::Aabb& left,
    const renderer::scene_contract::Aabb& right);

[[nodiscard]] renderer::scene_contract::Aabb makeStableFocusBounds(
    const renderer::scene_contract::Aabb& localBounds,
    const renderer::scene_contract::TransformData& transform);

}  // namespace camera_focus_bounds
