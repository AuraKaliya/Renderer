#include "viewport_zoom_state.h"

namespace viewport_zoom {

ZoomOperationKind chooseZoomOperationKind(
    bool orthographicProjection,
    bool lensZoomMode)
{
    if (orthographicProjection) {
        return ZoomOperationKind::lens;
    }

    return lensZoomMode ? ZoomOperationKind::lens : ZoomOperationKind::dolly;
}

void begin(
    State& state,
    const renderer::scene_contract::Vec2f& viewportPositionNormalized,
    const renderer::scene_contract::Vec3f& anchorWorldPoint)
{
    state.active = true;
    state.anchor.viewportPositionNormalized = viewportPositionNormalized;
    state.anchor.worldPoint = anchorWorldPoint;
    state.anchor.valid = true;
}

void reset(State& state) {
    const auto anchorMode = state.anchorMode;
    const auto zoomOperationKind = state.zoomOperationKind;
    state = {};
    state.anchorMode = anchorMode;
    state.zoomOperationKind = zoomOperationKind;
}

}  // namespace viewport_zoom
