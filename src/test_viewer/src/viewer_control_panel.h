#pragma once

#include <array>

#include <QWidget>

#include "camera_control_widget.h"
#include "lighting_control_widget.h"
#include "scene_object_control_widget.h"
#include "renderer/scene_contract/types.h"

class ViewerControlPanel final : public QWidget {
    Q_OBJECT

public:
    static constexpr int kSceneObjectCount = 3;

    explicit ViewerControlPanel(QWidget* parent = nullptr);

    void setObjectState(int index, bool visible, float rotationSpeed, const renderer::scene_contract::ColorRgba& color);
    void setLightingState(float ambientStrength, const renderer::scene_contract::Vec3f& lightDirection);
    void setCameraState(float distance, float verticalFovDegrees);

signals:
    void objectVisibleChanged(int index, bool visible);
    void objectRotationSpeedChanged(int index, float speed);
    void objectColorChanged(int index, float red, float green, float blue);
    void ambientStrengthChanged(float strength);
    void lightDirectionChanged(float x, float y, float z);
    void cameraDistanceChanged(float distance);
    void verticalFovDegreesChanged(float degrees);
    void resetDefaultsRequested();
    void focusSphereRequested();

private:
    std::array<SceneObjectControlWidget*, kSceneObjectCount> objectWidgets_ {};
    LightingControlWidget* lightingWidget_ = nullptr;
    CameraControlWidget* cameraWidget_ = nullptr;
};
