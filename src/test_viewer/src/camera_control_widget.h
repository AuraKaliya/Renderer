#pragma once

#include <QWidget>

class QDoubleSpinBox;

class CameraControlWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CameraControlWidget(QWidget* parent = nullptr);

    void setCameraState(float distance, float verticalFovDegrees);

signals:
    void cameraDistanceChanged(float distance);
    void verticalFovDegreesChanged(float degrees);

private:
    QDoubleSpinBox* distanceSpinBox_ = nullptr;
    QDoubleSpinBox* fovSpinBox_ = nullptr;
};
