#pragma once

#include "viewer_ui_types.h"

namespace viewer_ui {

class ViewerUiStateAdapter {
public:
    [[nodiscard]] static SceneObjectUiState buildSceneObjectState(
        const SceneObjectAdapterInput& input);
};

}  // namespace viewer_ui
