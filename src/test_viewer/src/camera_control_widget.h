#pragma once

#include <QWidget>
#include "renderer/scene_contract/types.h"

class QDoubleSpinBox;
class QLabel;

class CameraControlWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CameraControlWidget(QWidget* parent = nullptr);

    void setCameraState(
        float distance,
        float verticalFovDegrees,
        const renderer::scene_contract::Vec3f& orbitCenter);

signals:
    void cameraDistanceChanged(float distance);
    void verticalFovDegreesChanged(float degrees);

private:
    QDoubleSpinBox* distanceSpinBox_ = nullptr;
    QDoubleSpinBox* fovSpinBox_ = nullptr;
    QLabel* orbitCenterLabel_ = nullptr;
};
