#include "camera_mode_ui_policy.h"

namespace camera_mode_ui_policy {

CameraModeUiRuleSet makeRuleSet(int projectionMode, int zoomMode) {
    const bool perspective = projectionMode == 0;
    const bool lens = zoomMode == 1;

    CameraModeUiRuleSet rules;
    rules.zoomModeEditable = perspective;

    if (perspective && !lens) {
        rules.distanceEditable = true;
        rules.fovEditable = true;
        rules.distanceRole = ParameterRole::primary;
        rules.fovRole = ParameterRole::secondary;
        return rules;
    }

    if (perspective && lens) {
        rules.distanceEditable = true;
        rules.fovEditable = true;
        rules.distanceRole = ParameterRole::secondary;
        rules.fovRole = ParameterRole::primary;
        return rules;
    }

    rules.orthographicHeightEditable = true;
    rules.distanceRole = ParameterRole::secondary;
    rules.orthographicHeightRole = ParameterRole::primary;
    return rules;
}

}  // namespace camera_mode_ui_policy