#pragma once

#include <QWidget>
#include "renderer/scene_contract/types.h"

class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QComboBox;
class QGroupBox;

class CameraControlWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CameraControlWidget(QWidget* parent = nullptr);

    void setCameraState(
        int projectionMode,
        int zoomMode,
        float distance,
        float verticalFovDegrees,
        float orthographicHeight,
        float nearPlane,
        float farPlane,
        const renderer::scene_contract::Vec3f& orbitCenter);

signals:
    void projectionModeChanged(int mode);
    void zoomModeChanged(int mode);
    void cameraDistanceChanged(float distance);
    void verticalFovDegreesChanged(float degrees);
    void orthographicHeightChanged(float height);
    void focusPointRequested(float x, float y, float z);

private:
    QGroupBox* modeGroup_ = nullptr;
    QGroupBox* parameterGroup_ = nullptr;
    QGroupBox* focusGroup_ = nullptr;
    QComboBox* projectionModeComboBox_ = nullptr;
    QComboBox* zoomModeComboBox_ = nullptr;
    QDoubleSpinBox* distanceSpinBox_ = nullptr;
    QDoubleSpinBox* fovSpinBox_ = nullptr;
    QDoubleSpinBox* orthographicHeightSpinBox_ = nullptr;
    QLabel* distanceRoleLabel_ = nullptr;
    QLabel* fovRoleLabel_ = nullptr;
    QLabel* orthographicHeightRoleLabel_ = nullptr;
    QDoubleSpinBox* focusPointXSpinBox_ = nullptr;
    QDoubleSpinBox* focusPointYSpinBox_ = nullptr;
    QDoubleSpinBox* focusPointZSpinBox_ = nullptr;
    QLabel* orbitCenterLabel_ = nullptr;
    QLabel* nearFarLabel_ = nullptr;
    QPushButton* focusPointButton_ = nullptr;
};
