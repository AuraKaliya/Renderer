#include "viewer_control_panel.h"

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QSignalBlocker>
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
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* contentWidget = new QWidget(scrollArea);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(10);
    scrollArea->setWidget(contentWidget);

    rootLayout->addWidget(scrollArea);

    for (int index = 0; index < kSceneObjectCount; ++index) {
        auto* objectWidget = new SceneObjectControlWidget(QString::fromLatin1(kObjectNames[index]), contentWidget);
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
        contentLayout->addWidget(objectWidget);
    }

    lightingWidget_ = new LightingControlWidget(contentWidget);
    connect(lightingWidget_, &LightingControlWidget::ambientStrengthChanged, this, [this](float strength) {
        emit ambientStrengthChanged(strength);
    });
    connect(lightingWidget_, &LightingControlWidget::lightDirectionChanged, this, [this](float x, float y, float z) {
        emit lightDirectionChanged(x, y, z);
    });

    cameraWidget_ = new CameraControlWidget(contentWidget);
    connect(cameraWidget_, &CameraControlWidget::projectionModeChanged, this, [this](int mode) {
        emit projectionModeChanged(mode);
    });
    connect(cameraWidget_, &CameraControlWidget::zoomModeChanged, this, [this](int mode) {
        emit zoomModeChanged(mode);
    });
    connect(cameraWidget_, &CameraControlWidget::cameraDistanceChanged, this, [this](float distance) {
        emit cameraDistanceChanged(distance);
    });
    connect(cameraWidget_, &CameraControlWidget::verticalFovDegreesChanged, this, [this](float degrees) {
        emit verticalFovDegreesChanged(degrees);
    });
    connect(cameraWidget_, &CameraControlWidget::orthographicHeightChanged, this, [this](float height) {
        emit orthographicHeightChanged(height);
    });
    connect(cameraWidget_, &CameraControlWidget::focusPointRequested, this, [this](float x, float y, float z) {
        emit focusPointRequested(x, y, z);
    });

    auto* actionGroup = new QGroupBox("Actions", contentWidget);
    auto* actionLayout = new QVBoxLayout(actionGroup);

    modelChangeViewStrategyComboBox_ = new QComboBox(actionGroup);
    modelChangeViewStrategyComboBox_->addItem("Keep View", 0);
    modelChangeViewStrategyComboBox_->addItem("Auto Frame", 1);
    connect(
        modelChangeViewStrategyComboBox_,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        [this](int index) {
            emit modelChangeViewStrategyChanged(
                modelChangeViewStrategyComboBox_->itemData(index).toInt());
        });

    auto* boundsGroup = new QGroupBox("Bounds", contentWidget);
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

    auto* focusAllButton = new QPushButton("Focus All", actionGroup);
    connect(focusAllButton, &QPushButton::clicked, this, [this]() {
        emit focusAllRequested();
    });

    actionLayout->addWidget(new QLabel("Model Change View", actionGroup));
    actionLayout->addWidget(modelChangeViewStrategyComboBox_);
    actionLayout->addWidget(resetButton);
    actionLayout->addWidget(sphereFocusButton);
    actionLayout->addWidget(focusAllButton);

    contentLayout->addWidget(lightingWidget_);
    contentLayout->addWidget(cameraWidget_);
    contentLayout->addWidget(boundsGroup);
    contentLayout->addWidget(actionGroup);
    contentLayout->addStretch(1);
}

void ViewerControlPanel::setPanelState(const PanelState& state) {
    for (int index = 0; index < kSceneObjectCount; ++index) {
        const auto& object = state.objects[index];
        setObjectState(index, object.visible, object.rotationSpeed, object.color);
        setObjectBounds(index, object.bounds);
    }

    setLightingState(state.lighting.ambientStrength, state.lighting.lightDirection);
    setCameraState(state.camera);
    if (modelChangeViewStrategyComboBox_ != nullptr) {
        const QSignalBlocker blocker(modelChangeViewStrategyComboBox_);
        const int comboIndex = modelChangeViewStrategyComboBox_->findData(state.modelChangeViewStrategy);
        if (comboIndex >= 0) {
            modelChangeViewStrategyComboBox_->setCurrentIndex(comboIndex);
        }
    }
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

void ViewerControlPanel::setCameraState(const CameraPanelState& state) {
    cameraWidget_->setCameraState(
        state.projectionMode,
        state.zoomMode,
        state.distance,
        state.verticalFovDegrees,
        state.orthographicHeight,
        state.nearPlane,
        state.farPlane,
        state.orbitCenter);
}
