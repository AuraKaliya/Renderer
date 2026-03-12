#pragma once

#include <QWidget>
#include "renderer/scene_contract/types.h"

class QDoubleSpinBox;
class QLabel;
class QPushButton;

class CameraControlWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CameraControlWidget(QWidget* parent = nullptr);

    void setCameraState(
        float distance,
        float verticalFovDegrees,
        float nearPlane,
        float farPlane,
        const renderer::scene_contract::Vec3f& orbitCenter);

signals:
    void cameraDistanceChanged(float distance);
    void verticalFovDegreesChanged(float degrees);
    void focusPointRequested(float x, float y, float z);

private:
    QDoubleSpinBox* distanceSpinBox_ = nullptr;
    QDoubleSpinBox* fovSpinBox_ = nullptr;
    QDoubleSpinBox* focusPointXSpinBox_ = nullptr;
    QDoubleSpinBox* focusPointYSpinBox_ = nullptr;
    QDoubleSpinBox* focusPointZSpinBox_ = nullptr;
    QLabel* orbitCenterLabel_ = nullptr;
    QLabel* nearFarLabel_ = nullptr;
    QPushButton* focusPointButton_ = nullptr;
};
