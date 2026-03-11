#include "camera_control_widget.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QVBoxLayout>

CameraControlWidget::CameraControlWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* group = new QGroupBox("Camera", this);
    auto* formLayout = new QFormLayout(group);

    distanceSpinBox_ = new QDoubleSpinBox(group);
    distanceSpinBox_->setRange(1.5, 20.0);
    distanceSpinBox_->setSingleStep(0.1);
    distanceSpinBox_->setDecimals(2);
    connect(distanceSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit cameraDistanceChanged(static_cast<float>(value));
    });

    fovSpinBox_ = new QDoubleSpinBox(group);
    fovSpinBox_->setRange(20.0, 90.0);
    fovSpinBox_->setSingleStep(1.0);
    fovSpinBox_->setDecimals(1);
    connect(fovSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit verticalFovDegreesChanged(static_cast<float>(value));
    });

    formLayout->addRow("Distance", distanceSpinBox_);
    formLayout->addRow("Vertical FOV", fovSpinBox_);

    rootLayout->addWidget(group);
}

void CameraControlWidget::setCameraState(float distance, float verticalFovDegrees) {
    const QSignalBlocker distanceBlocker(distanceSpinBox_);
    const QSignalBlocker fovBlocker(fovSpinBox_);

    distanceSpinBox_->setValue(distance);
    fovSpinBox_->setValue(verticalFovDegrees);
}
