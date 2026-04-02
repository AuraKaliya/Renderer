#pragma once

#include <QWidget>

#include "renderer/parametric_model/primitive_factory.h"
#include "renderer/scene_contract/types.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;

class SceneObjectControlWidget final : public QWidget {
    Q_OBJECT

public:
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

    explicit SceneObjectControlWidget(const QString& title, QWidget* parent = nullptr);

    void setObjectState(bool visible, float rotationSpeed, const renderer::scene_contract::ColorRgba& color);
    void setOperatorState(const MirrorState& mirror, const LinearArrayState& linearArray);

signals:
    void visibleChanged(bool visible);
    void rotationSpeedChanged(float speed);
    void colorChanged(float red, float green, float blue);
    void mirrorEnabledChanged(bool enabled);
    void mirrorAxisChanged(int axis);
    void mirrorPlaneOffsetChanged(float planeOffset);
    void linearArrayEnabledChanged(bool enabled);
    void linearArrayCountChanged(int count);
    void linearArrayOffsetChanged(float x, float y, float z);

private:
    QCheckBox* visibleCheckBox_ = nullptr;
    QDoubleSpinBox* speedSpinBox_ = nullptr;
    QDoubleSpinBox* redSpinBox_ = nullptr;
    QDoubleSpinBox* greenSpinBox_ = nullptr;
    QDoubleSpinBox* blueSpinBox_ = nullptr;
    QCheckBox* mirrorEnabledCheckBox_ = nullptr;
    QComboBox* mirrorAxisComboBox_ = nullptr;
    QDoubleSpinBox* mirrorPlaneOffsetSpinBox_ = nullptr;
    QCheckBox* linearArrayEnabledCheckBox_ = nullptr;
    QSpinBox* linearArrayCountSpinBox_ = nullptr;
    QDoubleSpinBox* linearArrayOffsetXSpinBox_ = nullptr;
    QDoubleSpinBox* linearArrayOffsetYSpinBox_ = nullptr;
    QDoubleSpinBox* linearArrayOffsetZSpinBox_ = nullptr;
};
