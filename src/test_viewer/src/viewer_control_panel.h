#pragma once

#include <array>
#include <vector>

#include <QWidget>

class QLabel;
class QListWidget;
class QCheckBox;
class QPushButton;

#include "camera_control_widget.h"
#include "lighting_control_widget.h"
#include "scene_object_control_widget.h"
#include "renderer/parametric_model/primitive_factory.h"
#include "renderer/scene_contract/types.h"

class ViewerControlPanel final : public QWidget {
    Q_OBJECT

public:
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

    struct FeaturePanelState {
        renderer::parametric_model::ParametricFeatureId id = 0U;
        renderer::parametric_model::ParametricUnitId unitId = 0U;
        renderer::parametric_model::FeatureKind kind = renderer::parametric_model::FeatureKind::primitive;
        bool enabled = true;
    };

    struct SceneObjectPanelState {
        renderer::parametric_model::ParametricObjectId id = 0U;
        renderer::parametric_model::PrimitiveKind primitiveKind = renderer::parametric_model::PrimitiveKind::box;
        renderer::parametric_model::PrimitiveDescriptor primitive {};
        std::vector<renderer::parametric_model::ParametricNodeDescriptor> nodes;
        bool visible = true;
        float rotationSpeed = 0.0F;
        renderer::scene_contract::ColorRgba color {};
        renderer::scene_contract::Aabb bounds {};
        SceneObjectControlWidget::MirrorState mirror {};
        SceneObjectControlWidget::LinearArrayState linearArray {};
        std::vector<FeaturePanelState> features;
    };

    struct LightingPanelState {
        float ambientStrength = 0.0F;
        renderer::scene_contract::Vec3f lightDirection {};
    };

    struct SelectionPanelState {
        renderer::parametric_model::ParametricObjectId selectedObjectId = 0U;
        renderer::parametric_model::ParametricObjectId activeObjectId = 0U;
        renderer::parametric_model::ParametricFeatureId selectedFeatureId = 0U;
        renderer::parametric_model::ParametricFeatureId activeFeatureId = 0U;
    };

    struct PanelState {
        std::vector<SceneObjectPanelState> objects;
        LightingPanelState lighting {};
        CameraPanelState camera {};
        SelectionPanelState selection {};
        int modelChangeViewStrategy = 0;
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
    void objectFeatureAddRequested(int index, int featureKind);
    void objectFeatureRemoveRequested(int index, int featureId);
    void objectFeatureEnabledChanged(int index, int featureId, bool enabled);
    void objectAddRequested(int primitiveKind);
    void deleteSelectedObjectRequested();
    void objectSelectionChanged(int objectId);
    void objectActivationRequested(int objectId);
    void featureSelectionChanged(int objectId, int featureId);
    void featureActivationRequested(int objectId, int featureId);
    void focusSelectedObjectRequested();
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
    void sphereConstructionModeChanged(int mode);
    void sphereCenterChanged(float x, float y, float z);
    void sphereSurfacePointChanged(float x, float y, float z);

private:
    void setLightingState(float ambientStrength, const renderer::scene_contract::Vec3f& lightDirection);
    void setCameraState(const CameraPanelState& state);
    void refreshObjectExplorer();
    void refreshFeatureExplorer();
    void refreshObjectInspector();
    void refreshBoundsList();
    void updateFeatureActionState();
    int findObjectIndexById(renderer::parametric_model::ParametricObjectId objectId) const;
    [[nodiscard]] int currentObjectIndex() const;
    [[nodiscard]] const SceneObjectPanelState* currentObjectState() const;
    [[nodiscard]] SceneObjectControlWidget* currentInspectorWidget() const;

    std::array<SceneObjectControlWidget*, 3> objectWidgets_ {};
    LightingControlWidget* lightingWidget_ = nullptr;
    CameraControlWidget* cameraWidget_ = nullptr;
    QListWidget* objectListWidget_ = nullptr;
    QListWidget* featureListWidget_ = nullptr;
    QListWidget* boundsListWidget_ = nullptr;
    QLabel* objectSelectionLabel_ = nullptr;
    QLabel* featureSelectionLabel_ = nullptr;
    QCheckBox* featureEnabledCheckBox_ = nullptr;
    QPushButton* activateObjectButton_ = nullptr;
    QPushButton* deleteSelectedObjectButton_ = nullptr;
    QPushButton* activateFeatureButton_ = nullptr;
    QPushButton* focusSelectedObjectButton_ = nullptr;
    QPushButton* addBoxObjectButton_ = nullptr;
    QPushButton* addCylinderObjectButton_ = nullptr;
    QPushButton* addSphereObjectButton_ = nullptr;
    QPushButton* addMirrorFeatureButton_ = nullptr;
    QPushButton* addLinearArrayFeatureButton_ = nullptr;
    QPushButton* removeFeatureButton_ = nullptr;
    class QComboBox* modelChangeViewStrategyComboBox_ = nullptr;
    PanelState panelState_ {};
    int inspectedObjectIndex_ = -1;
    renderer::parametric_model::ParametricFeatureId inspectedFeatureId_ = 0U;
};
