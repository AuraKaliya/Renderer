#pragma once

#include <array>

#include <QWidget>

class QLabel;

#include "camera_control_widget.h"
#include "lighting_control_widget.h"
#include "scene_object_control_widget.h"
#include "renderer/scene_contract/types.h"

class ViewerControlPanel final : public QWidget {
    Q_OBJECT

public:
    static constexpr int kSceneObjectCount = 3;

    struct CameraPanelState {
        int projectionMode = 0;
        int zoomMode = 0;
        float distance = 0.0F;
        float verticalFovDegrees = 0.0F;
        float orthographicHeight = 0.0F;
        float nearPlane = 0.0F;
        float farPlane = 0.0F;
        renderer::scene_contract::Vec3f orbitCenter {};
    };

    struct SceneObjectPanelState {
        bool visible = true;
        float rotationSpeed = 0.0F;
        renderer::scene_contract::ColorRgba color {};
        renderer::scene_contract::Aabb bounds {};
    };

    struct LightingPanelState {
        float ambientStrength = 0.0F;
        renderer::scene_contract::Vec3f lightDirection {};
    };

    struct PanelState {
        std::array<SceneObjectPanelState, kSceneObjectCount> objects {};
        LightingPanelState lighting {};
        CameraPanelState camera {};
        int modelChangeViewStrategy = 0;
    };

    explicit ViewerControlPanel(QWidget* parent = nullptr);

    void setPanelState(const PanelState& state);

signals:
    void objectVisibleChanged(int index, bool visible);
    void objectRotationSpeedChanged(int index, float speed);
    void objectColorChanged(int index, float red, float green, float blue);
    void ambientStrengthChanged(float strength);
    void lightDirectionChanged(float x, float y, float z);
    void projectionModeChanged(int mode);
    void zoomModeChanged(int mode);
    void cameraDistanceChanged(float distance);
    void verticalFovDegreesChanged(float degrees);
    void orthographicHeightChanged(float height);
    void focusPointRequested(float x, float y, float z);
    void focusAllRequested();
    void resetDefaultsRequested();
    void focusSphereRequested();
    void modelChangeViewStrategyChanged(int strategy);

private:
    void setObjectState(int index, bool visible, float rotationSpeed, const renderer::scene_contract::ColorRgba& color);
    void setObjectBounds(int index, const renderer::scene_contract::Aabb& bounds);
    void setLightingState(float ambientStrength, const renderer::scene_contract::Vec3f& lightDirection);
    void setCameraState(const CameraPanelState& state);

    std::array<SceneObjectControlWidget*, kSceneObjectCount> objectWidgets_ {};
    std::array<QLabel*, kSceneObjectCount> objectBoundsLabels_ {};
    LightingControlWidget* lightingWidget_ = nullptr;
    CameraControlWidget* cameraWidget_ = nullptr;
    class QComboBox* modelChangeViewStrategyComboBox_ = nullptr;
};
