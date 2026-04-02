#include "model_change_view_state.h"

namespace model_change_view {

void setViewStrategy(State& state, ViewStrategy viewStrategy) {
    state.viewStrategy = viewStrategy;
}

void markModelChanged(State& state, ChangeKind changeKind) {
    state.pendingModelChange = true;
    state.lastChangeKind = changeKind;
}

PendingActions pendingActions(const State& state) {
    PendingActions actions;
    actions.updateBoundsAndClip = state.pendingModelChange;
    actions.autoFrame =
        state.pendingModelChange &&
        state.viewStrategy == ViewStrategy::auto_frame;
    return actions;
}

void clearPending(State& state) {
    state.pendingModelChange = false;
}

}  // namespace model_change_view
