#pragma once

#include <array>

#include <QWidget>

class QLabel;

#include "camera_control_widget.h"
#include "lighting_control_widget.h"
#include "scene_object_control_widget.h"
#include "renderer/parametric_model/primitive_factory.h"
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
        SceneObjectControlWidget::MirrorState mirror {};
        SceneObjectControlWidget::LinearArrayState linearArray {};
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
        renderer::parametric_model::BoxSpec box {};
        renderer::parametric_model::CylinderSpec cylinder {};
        renderer::parametric_model::SphereSpec sphere {};
    };

    explicit ViewerControlPanel(QWidget* parent = nullptr);

    void setPanelState(const PanelState& state);

signals:
    void objectVisibleChanged(int index, bool visible);
    void objectRotationSpeedChanged(int index, float speed);
    void objectColorChanged(int index, float red, float green, float blue);
    void objectMirrorEnabledChanged(int index, bool enabled);
    void objectMirrorAxisChanged(int index, int axis);
    void objectMirrorPlaneOffsetChanged(int index, float planeOffset);
    void objectLinearArrayEnabledChanged(int index, bool enabled);
    void objectLinearArrayCountChanged(int index, int count);
    void objectLinearArrayOffsetChanged(int index, float x, float y, float z);
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
    void boxWidthChanged(float width);
    void boxHeightChanged(float height);
    void boxDepthChanged(float depth);
    void cylinderRadiusChanged(float radius);
    void cylinderHeightChanged(float height);
    void cylinderSegmentsChanged(int segments);
    void sphereRadiusChanged(float radius);
    void sphereSlicesChanged(int slices);
    void sphereStacksChanged(int stacks);

private:
    void setObjectState(int index, bool visible, float rotationSpeed, const renderer::scene_contract::ColorRgba& color);
    void setObjectOperatorState(
        int index,
        const SceneObjectControlWidget::MirrorState& mirror,
        const SceneObjectControlWidget::LinearArrayState& linearArray);
    void setObjectBounds(int index, const renderer::scene_contract::Aabb& bounds);
    void setLightingState(float ambientStrength, const renderer::scene_contract::Vec3f& lightDirection);
    void setCameraState(const CameraPanelState& state);

    std::array<SceneObjectControlWidget*, kSceneObjectCount> objectWidgets_ {};
    std::array<QLabel*, kSceneObjectCount> objectBoundsLabels_ {};
    LightingControlWidget* lightingWidget_ = nullptr;
    CameraControlWidget* cameraWidget_ = nullptr;
    class QComboBox* modelChangeViewStrategyComboBox_ = nullptr;
    class QDoubleSpinBox* boxWidthSpinBox_ = nullptr;
    class QDoubleSpinBox* boxHeightSpinBox_ = nullptr;
    class QDoubleSpinBox* boxDepthSpinBox_ = nullptr;
    class QDoubleSpinBox* cylinderRadiusSpinBox_ = nullptr;
    class QDoubleSpinBox* cylinderHeightSpinBox_ = nullptr;
    class QSpinBox* cylinderSegmentsSpinBox_ = nullptr;
    class QDoubleSpinBox* sphereRadiusSpinBox_ = nullptr;
    class QSpinBox* sphereSlicesSpinBox_ = nullptr;
    class QSpinBox* sphereStacksSpinBox_ = nullptr;
};
