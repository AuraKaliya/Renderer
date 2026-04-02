#pragma once

#include "renderer/scene_contract/types.h"

namespace viewport_zoom {

enum class AnchorMode {
    orbit_center_plane
};

enum class ZoomOperationKind {
    dolly,
    lens
};

struct AnchorSample {
    renderer::scene_contract::Vec2f viewportPositionNormalized {};
    renderer::scene_contract::Vec3f worldPoint {};
    bool valid = false;
};

struct State {
    AnchorMode anchorMode = AnchorMode::orbit_center_plane;
    ZoomOperationKind zoomOperationKind = ZoomOperationKind::dolly;
    AnchorSample anchor {};
    bool active = false;
};

[[nodiscard]] ZoomOperationKind chooseZoomOperationKind(
    bool orthographicProjection,
    bool lensZoomMode);

void begin(
    State& state,
    const renderer::scene_contract::Vec2f& viewportPositionNormalized,
    const renderer::scene_contract::Vec3f& anchorWorldPoint);

void reset(State& state);

}  // namespace viewport_zoom
