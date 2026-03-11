#include "viewer_control_panel.h"

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString formatBoundsText(const QString& name, const renderer::scene_contract::Aabb& bounds) {
    if (!bounds.valid) {
        return QStringLiteral("%1: invalid").arg(name);
    }

    return QStringLiteral(
               "%1\nmin(%2, %3, %4)\nmax(%5, %6, %7)")
        .arg(name)
        .arg(bounds.min.x, 0, 'f', 2)
        .arg(bounds.min.y, 0, 'f', 2)
        .arg(bounds.min.z, 0, 'f', 2)
        .arg(bounds.max.x, 0, 'f', 2)
        .arg(bounds.max.y, 0, 'f', 2)
        .arg(bounds.max.z, 0, 'f', 2);
}

}  // namespace

ViewerControlPanel::ViewerControlPanel(QWidget* parent)
    : QWidget(parent)
{
    static const std::array<const char*, kSceneObjectCount> kObjectNames = {
        "Box",
        "Cylinder",
        "Sphere"
    };

    setFixedWidth(320);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    for (int index = 0; index < kSceneObjectCount; ++index) {
        auto* objectWidget = new SceneObjectControlWidget(QString::fromLatin1(kObjectNames[index]), this);
        connect(objectWidget, &SceneObjectControlWidget::visibleChanged, this, [this, index](bool visible) {
            emit objectVisibleChanged(index, visible);
        });
        connect(objectWidget, &SceneObjectControlWidget::rotationSpeedChanged, this, [this, index](float speed) {
            emit objectRotationSpeedChanged(index, speed);
        });
        connect(objectWidget, &SceneObjectControlWidget::colorChanged, this, [this, index](float red, float green, float blue) {
            emit objectColorChanged(index, red, green, blue);
        });
        objectWidgets_[index] = objectWidget;
        rootLayout->addWidget(objectWidget);
    }

    lightingWidget_ = new LightingControlWidget(this);
    connect(lightingWidget_, &LightingControlWidget::ambientStrengthChanged, this, [this](float strength) {
        emit ambientStrengthChanged(strength);
    });
    connect(lightingWidget_, &LightingControlWidget::lightDirectionChanged, this, [this](float x, float y, float z) {
        emit lightDirectionChanged(x, y, z);
    });

    cameraWidget_ = new CameraControlWidget(this);
    connect(cameraWidget_, &CameraControlWidget::cameraDistanceChanged, this, [this](float distance) {
        emit cameraDistanceChanged(distance);
    });
    connect(cameraWidget_, &CameraControlWidget::verticalFovDegreesChanged, this, [this](float degrees) {
        emit verticalFovDegreesChanged(degrees);
    });

    auto* actionGroup = new QGroupBox("Actions", this);
    auto* actionLayout = new QVBoxLayout(actionGroup);

    auto* boundsGroup = new QGroupBox("Bounds", this);
    auto* boundsLayout = new QVBoxLayout(boundsGroup);
    for (int index = 0; index < kSceneObjectCount; ++index) {
        auto* label = new QLabel(boundsGroup);
        label->setWordWrap(true);
        label->setTextFormat(Qt::PlainText);
        objectBoundsLabels_[index] = label;
        boundsLayout->addWidget(label);
    }

    auto* resetButton = new QPushButton("Reset Defaults", actionGroup);
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        emit resetDefaultsRequested();
    });

    auto* sphereFocusButton = new QPushButton("Focus Sphere", actionGroup);
    connect(sphereFocusButton, &QPushButton::clicked, this, [this]() {
        emit focusSphereRequested();
    });

    actionLayout->addWidget(resetButton);
    actionLayout->addWidget(sphereFocusButton);

    rootLayout->addWidget(lightingWidget_);
    rootLayout->addWidget(cameraWidget_);
    rootLayout->addWidget(boundsGroup);
    rootLayout->addWidget(actionGroup);
    rootLayout->addStretch(1);
}

void ViewerControlPanel::setObjectState(
    int index,
    bool visible,
    float rotationSpeed,
    const renderer::scene_contract::ColorRgba& color)
{
    if (index < 0 || index >= kSceneObjectCount) {
        return;
    }
    objectWidgets_[index]->setObjectState(visible, rotationSpeed, color);
}

void ViewerControlPanel::setObjectBounds(int index, const renderer::scene_contract::Aabb& bounds) {
    if (index < 0 || index >= kSceneObjectCount) {
        return;
    }

    static const std::array<const char*, kSceneObjectCount> kObjectNames = {
        "Box",
        "Cylinder",
        "Sphere"
    };

    objectBoundsLabels_[index]->setText(formatBoundsText(QString::fromLatin1(kObjectNames[index]), bounds));
}

void ViewerControlPanel::setLightingState(float ambientStrength, const renderer::scene_contract::Vec3f& lightDirection) {
    lightingWidget_->setLightingState(ambientStrength, lightDirection);
}

void ViewerControlPanel::setCameraState(float distance, float verticalFovDegrees) {
    cameraWidget_->setCameraState(distance, verticalFovDegrees);
}
