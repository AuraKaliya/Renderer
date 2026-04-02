#include "camera_control_widget.h"

#include "camera_mode_ui_policy.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSignalBlocker>
#include <QHBoxLayout>
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

QString formatRoleText(camera_mode_ui_policy::ParameterRole role) {
    switch (role) {
    case camera_mode_ui_policy::ParameterRole::primary:
        return QStringLiteral("Primary");
    case camera_mode_ui_policy::ParameterRole::secondary:
        return QStringLiteral("Secondary");
    case camera_mode_ui_policy::ParameterRole::inactive:
    default:
        return QStringLiteral("Inactive");
    }
}

void applyRoleLabelStyle(QLabel* label, camera_mode_ui_policy::ParameterRole role) {
    switch (role) {
    case camera_mode_ui_policy::ParameterRole::primary:
        label->setStyleSheet(QStringLiteral("color: #1f5f2c; font-weight: 600;"));
        break;
    case camera_mode_ui_policy::ParameterRole::secondary:
        label->setStyleSheet(QStringLiteral("color: #6a6a6a;"));
        break;
    case camera_mode_ui_policy::ParameterRole::inactive:
    default:
        label->setStyleSheet(QStringLiteral("color: #9a9a9a;"));
        break;
    }
}

QWidget* makeLabeledControlRow(
    const QString& labelText,
    QWidget* editor,
    QLabel*& roleLabel,
    QWidget* parent)
{
    auto* container = new QWidget(parent);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* nameLabel = new QLabel(labelText, container);
    nameLabel->setMinimumWidth(92);

    roleLabel = new QLabel(container);
    roleLabel->setMinimumWidth(60);
    roleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(nameLabel);
    layout->addWidget(editor, 1);
    layout->addWidget(roleLabel);
    return container;
}

}  // namespace

CameraControlWidget::CameraControlWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* group = new QGroupBox("Camera", this);
    auto* groupLayout = new QVBoxLayout(group);

    modeGroup_ = new QGroupBox("Mode", group);
    auto* modeLayout = new QFormLayout(modeGroup_);

    parameterGroup_ = new QGroupBox("Parameters", group);
    auto* parameterLayout = new QVBoxLayout(parameterGroup_);

    focusGroup_ = new QGroupBox("Focus / State", group);
    auto* focusLayout = new QFormLayout(focusGroup_);

    projectionModeComboBox_ = new QComboBox(group);
    projectionModeComboBox_->addItem("Perspective", 0);
    projectionModeComboBox_->addItem("Orthographic", 1);
    connect(projectionModeComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        emit projectionModeChanged(projectionModeComboBox_->itemData(index).toInt());
    });

    zoomModeComboBox_ = new QComboBox(group);
    zoomModeComboBox_->addItem("Dolly", 0);
    zoomModeComboBox_->addItem("Lens", 1);
    connect(zoomModeComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        emit zoomModeChanged(zoomModeComboBox_->itemData(index).toInt());
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

    nearFarLabel_ = new QLabel(group);
    nearFarLabel_->setTextFormat(Qt::PlainText);

    focusPointButton_ = new QPushButton("Focus Point", group);
    connect(focusPointButton_, &QPushButton::clicked, this, [this]() {
        emit focusPointRequested(
            static_cast<float>(focusPointXSpinBox_->value()),
            static_cast<float>(focusPointYSpinBox_->value()),
            static_cast<float>(focusPointZSpinBox_->value()));
    });

    modeLayout->addRow("Projection", projectionModeComboBox_);
    modeLayout->addRow("Zoom Mode", zoomModeComboBox_);

    parameterLayout->addWidget(makeLabeledControlRow("Distance", distanceSpinBox_, distanceRoleLabel_, parameterGroup_));
    parameterLayout->addWidget(makeLabeledControlRow("Vertical FOV", fovSpinBox_, fovRoleLabel_, parameterGroup_));
    parameterLayout->addWidget(makeLabeledControlRow("Ortho Height", orthographicHeightSpinBox_, orthographicHeightRoleLabel_, parameterGroup_));

    focusLayout->addRow("Near / Far", nearFarLabel_);
    focusLayout->addRow("Orbit Center", orbitCenterLabel_);
    focusLayout->addRow("Point X", focusPointXSpinBox_);
    focusLayout->addRow("Point Y", focusPointYSpinBox_);
    focusLayout->addRow("Point Z", focusPointZSpinBox_);
    focusLayout->addRow("", focusPointButton_);

    groupLayout->addWidget(modeGroup_);
    groupLayout->addWidget(parameterGroup_);
    groupLayout->addWidget(focusGroup_);
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
    const QSignalBlocker zoomModeBlocker(zoomModeComboBox_);
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
    const int zoomModeIndex = zoomModeComboBox_->findData(zoomMode);
    if (zoomModeIndex >= 0) {
        zoomModeComboBox_->setCurrentIndex(zoomModeIndex);
    }
    distanceSpinBox_->setValue(distance);
    fovSpinBox_->setValue(verticalFovDegrees);
    orthographicHeightSpinBox_->setValue(orthographicHeight);
    nearFarLabel_->setText(formatNearFarText(nearPlane, farPlane));
    orbitCenterLabel_->setText(formatOrbitCenterText(orbitCenter));
    focusPointXSpinBox_->setValue(orbitCenter.x);
    focusPointYSpinBox_->setValue(orbitCenter.y);
    focusPointZSpinBox_->setValue(orbitCenter.z);

    const auto rules = camera_mode_ui_policy::makeRuleSet(projectionMode, zoomMode);
    zoomModeComboBox_->setEnabled(rules.zoomModeEditable);
    distanceSpinBox_->setEnabled(rules.distanceEditable);
    fovSpinBox_->setEnabled(rules.fovEditable);
    orthographicHeightSpinBox_->setEnabled(rules.orthographicHeightEditable);

    distanceRoleLabel_->setText(formatRoleText(rules.distanceRole));
    fovRoleLabel_->setText(formatRoleText(rules.fovRole));
    orthographicHeightRoleLabel_->setText(formatRoleText(rules.orthographicHeightRole));

    applyRoleLabelStyle(distanceRoleLabel_, rules.distanceRole);
    applyRoleLabelStyle(fovRoleLabel_, rules.fovRole);
    applyRoleLabelStyle(orthographicHeightRoleLabel_, rules.orthographicHeightRole);
}
