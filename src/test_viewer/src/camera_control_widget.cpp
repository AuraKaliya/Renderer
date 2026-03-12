#include "camera_control_widget.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace {

QString formatOrbitCenterText(const renderer::scene_contract::Vec3f& orbitCenter) {
    return QStringLiteral("(%1, %2, %3)")
        .arg(orbitCenter.x, 0, 'f', 2)
        .arg(orbitCenter.y, 0, 'f', 2)
        .arg(orbitCenter.z, 0, 'f', 2);
}

QString formatNearFarText(float nearPlane, float farPlane) {
    return QStringLiteral("near=%1 far=%2")
        .arg(nearPlane, 0, 'f', 3)
        .arg(farPlane, 0, 'f', 3);
}

QString formatZoomModeText(int zoomMode) {
    return zoomMode == 1 ? QStringLiteral("Lens") : QStringLiteral("Dolly");
}

}  // namespace

CameraControlWidget::CameraControlWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* group = new QGroupBox("Camera", this);
    auto* formLayout = new QFormLayout(group);

    projectionModeComboBox_ = new QComboBox(group);
    projectionModeComboBox_->addItem("Perspective", 0);
    projectionModeComboBox_->addItem("Orthographic", 1);
    connect(projectionModeComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        emit projectionModeChanged(projectionModeComboBox_->itemData(index).toInt());
    });

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

    orthographicHeightSpinBox_ = new QDoubleSpinBox(group);
    orthographicHeightSpinBox_->setRange(0.1, 100.0);
    orthographicHeightSpinBox_->setSingleStep(0.1);
    orthographicHeightSpinBox_->setDecimals(2);
    connect(orthographicHeightSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit orthographicHeightChanged(static_cast<float>(value));
    });

    focusPointXSpinBox_ = new QDoubleSpinBox(group);
    focusPointXSpinBox_->setRange(-1000.0, 1000.0);
    focusPointXSpinBox_->setSingleStep(0.1);
    focusPointXSpinBox_->setDecimals(2);

    focusPointYSpinBox_ = new QDoubleSpinBox(group);
    focusPointYSpinBox_->setRange(-1000.0, 1000.0);
    focusPointYSpinBox_->setSingleStep(0.1);
    focusPointYSpinBox_->setDecimals(2);

    focusPointZSpinBox_ = new QDoubleSpinBox(group);
    focusPointZSpinBox_->setRange(-1000.0, 1000.0);
    focusPointZSpinBox_->setSingleStep(0.1);
    focusPointZSpinBox_->setDecimals(2);

    orbitCenterLabel_ = new QLabel(group);
    orbitCenterLabel_->setTextFormat(Qt::PlainText);

    zoomModeLabel_ = new QLabel(group);
    zoomModeLabel_->setTextFormat(Qt::PlainText);

    nearFarLabel_ = new QLabel(group);
    nearFarLabel_->setTextFormat(Qt::PlainText);

    focusPointButton_ = new QPushButton("Focus Point", group);
    connect(focusPointButton_, &QPushButton::clicked, this, [this]() {
        emit focusPointRequested(
            static_cast<float>(focusPointXSpinBox_->value()),
            static_cast<float>(focusPointYSpinBox_->value()),
            static_cast<float>(focusPointZSpinBox_->value()));
    });

    formLayout->addRow("Projection", projectionModeComboBox_);
    formLayout->addRow("Distance", distanceSpinBox_);
    formLayout->addRow("Vertical FOV", fovSpinBox_);
    formLayout->addRow("Ortho Height", orthographicHeightSpinBox_);
    formLayout->addRow("Zoom Mode", zoomModeLabel_);
    formLayout->addRow("Near / Far", nearFarLabel_);
    formLayout->addRow("Orbit Center", orbitCenterLabel_);
    formLayout->addRow("Point X", focusPointXSpinBox_);
    formLayout->addRow("Point Y", focusPointYSpinBox_);
    formLayout->addRow("Point Z", focusPointZSpinBox_);
    formLayout->addRow("", focusPointButton_);

    rootLayout->addWidget(group);
}

void CameraControlWidget::setCameraState(
    int projectionMode,
    int zoomMode,
    float distance,
    float verticalFovDegrees,
    float orthographicHeight,
    float nearPlane,
    float farPlane,
    const renderer::scene_contract::Vec3f& orbitCenter)
{
    const QSignalBlocker projectionBlocker(projectionModeComboBox_);
    const QSignalBlocker distanceBlocker(distanceSpinBox_);
    const QSignalBlocker fovBlocker(fovSpinBox_);
    const QSignalBlocker orthographicHeightBlocker(orthographicHeightSpinBox_);
    const QSignalBlocker pointXBlocker(focusPointXSpinBox_);
    const QSignalBlocker pointYBlocker(focusPointYSpinBox_);
    const QSignalBlocker pointZBlocker(focusPointZSpinBox_);

    const int comboIndex = projectionModeComboBox_->findData(projectionMode);
    if (comboIndex >= 0) {
        projectionModeComboBox_->setCurrentIndex(comboIndex);
    }
    distanceSpinBox_->setValue(distance);
    fovSpinBox_->setValue(verticalFovDegrees);
    orthographicHeightSpinBox_->setValue(orthographicHeight);
    zoomModeLabel_->setText(formatZoomModeText(zoomMode));
    nearFarLabel_->setText(formatNearFarText(nearPlane, farPlane));
    orbitCenterLabel_->setText(formatOrbitCenterText(orbitCenter));
    focusPointXSpinBox_->setValue(orbitCenter.x);
    focusPointYSpinBox_->setValue(orbitCenter.y);
    focusPointZSpinBox_->setValue(orbitCenter.z);

    const bool perspective = projectionMode == 0;
    fovSpinBox_->setEnabled(perspective);
    distanceSpinBox_->setEnabled(perspective);
    orthographicHeightSpinBox_->setEnabled(!perspective);
}
