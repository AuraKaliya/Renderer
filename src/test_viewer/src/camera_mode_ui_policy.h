#pragma once

namespace camera_mode_ui_policy {

enum class ParameterRole {
    primary,
    secondary,
    inactive
};

struct CameraModeUiRuleSet {
    bool zoomModeEditable = false;
    bool distanceEditable = false;
    bool fovEditable = false;
    bool orthographicHeightEditable = false;
    ParameterRole distanceRole = ParameterRole::inactive;
    ParameterRole fovRole = ParameterRole::inactive;
    ParameterRole orthographicHeightRole = ParameterRole::inactive;
};

[[nodiscard]] CameraModeUiRuleSet makeRuleSet(int projectionMode, int zoomMode);

}  // namespace camera_mode_ui_policy