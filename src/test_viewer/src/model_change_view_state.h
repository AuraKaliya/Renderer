#pragma once

namespace model_change_view {

enum class ViewStrategy {
    keep_view,
    auto_frame
};

enum class ChangeKind {
    geometry_or_transform,
    scene_visibility
};

struct PendingActions {
    bool updateBoundsAndClip = false;
    bool autoFrame = false;
};

struct State {
    ViewStrategy viewStrategy = ViewStrategy::keep_view;
    ChangeKind lastChangeKind = ChangeKind::geometry_or_transform;
    bool pendingModelChange = false;
};

void setViewStrategy(State& state, ViewStrategy viewStrategy);

void markModelChanged(State& state, ChangeKind changeKind);

[[nodiscard]] PendingActions pendingActions(const State& state);

void clearPending(State& state);

}  // namespace model_change_view
