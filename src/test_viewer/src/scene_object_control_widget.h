#pragma once

#include <vector>

#include <QWidget>

#include "renderer/parametric_model/primitive_factory.h"
#include "renderer/scene_contract/types.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QSpinBox;

class SceneObjectControlWidget final : public QWidget {
    Q_OBJECT

public:
    enum class PrimitivePanelKind {
        box,
        cylinder,
        sphere
    };

    struct MirrorState {
        bool enabled = false;
        int axis = 0;
        float planeOffset = 0.0F;
    };

    struct LinearArrayState {
        bool enabled = false;
        int count = 1;
        renderer::scene_contract::Vec3f offset {};
    };

    explicit SceneObjectControlWidget(
        const QString& title,
        PrimitivePanelKind primitiveKind,
        QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setObjectState(bool visible, float rotationSpeed, const renderer::scene_contract::ColorRgba& color);
    void setOperatorState(const MirrorState& mirror, const LinearArrayState& linearArray);
    void setBoxSpec(const renderer::parametric_model::BoxSpec& spec);
    void setCylinderSpec(const renderer::parametric_model::CylinderSpec& spec);
    void setSphereSpec(
        const renderer::parametric_model::SphereSpec& spec,
        const std::vector<renderer::parametric_model::ParametricNodeDescriptor>& nodes);

signals:
    void visibleChanged(bool visible);
    void rotationSpeedChanged(float speed);
    void colorChanged(float red, float green, float blue);
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
    void mirrorEnabledChanged(bool enabled);
    void mirrorAxisChanged(int axis);
    void mirrorPlaneOffsetChanged(float planeOffset);
    void linearArrayEnabledChanged(bool enabled);
    void linearArrayCountChanged(int count);
    void linearArrayOffsetChanged(float x, float y, float z);

private:
    PrimitivePanelKind primitiveKind_;
    QGroupBox* groupBox_ = nullptr;
    QCheckBox* visibleCheckBox_ = nullptr;
    QDoubleSpinBox* speedSpinBox_ = nullptr;
    QDoubleSpinBox* redSpinBox_ = nullptr;
    QDoubleSpinBox* greenSpinBox_ = nullptr;
    QDoubleSpinBox* blueSpinBox_ = nullptr;
    QDoubleSpinBox* boxWidthSpinBox_ = nullptr;
    QDoubleSpinBox* boxHeightSpinBox_ = nullptr;
    QDoubleSpinBox* boxDepthSpinBox_ = nullptr;
    QDoubleSpinBox* cylinderRadiusSpinBox_ = nullptr;
    QDoubleSpinBox* cylinderHeightSpinBox_ = nullptr;
    QSpinBox* cylinderSegmentsSpinBox_ = nullptr;
    QDoubleSpinBox* sphereRadiusSpinBox_ = nullptr;
    QSpinBox* sphereSlicesSpinBox_ = nullptr;
    QSpinBox* sphereStacksSpinBox_ = nullptr;
    QComboBox* sphereConstructionModeComboBox_ = nullptr;
    QDoubleSpinBox* sphereCenterXSpinBox_ = nullptr;
    QDoubleSpinBox* sphereCenterYSpinBox_ = nullptr;
    QDoubleSpinBox* sphereCenterZSpinBox_ = nullptr;
    QDoubleSpinBox* sphereSurfacePointXSpinBox_ = nullptr;
    QDoubleSpinBox* sphereSurfacePointYSpinBox_ = nullptr;
    QDoubleSpinBox* sphereSurfacePointZSpinBox_ = nullptr;
    QCheckBox* mirrorEnabledCheckBox_ = nullptr;
    QComboBox* mirrorAxisComboBox_ = nullptr;
    QDoubleSpinBox* mirrorPlaneOffsetSpinBox_ = nullptr;
    QCheckBox* linearArrayEnabledCheckBox_ = nullptr;
    QSpinBox* linearArrayCountSpinBox_ = nullptr;
    QDoubleSpinBox* linearArrayOffsetXSpinBox_ = nullptr;
    QDoubleSpinBox* linearArrayOffsetYSpinBox_ = nullptr;
    QDoubleSpinBox* linearArrayOffsetZSpinBox_ = nullptr;
};
