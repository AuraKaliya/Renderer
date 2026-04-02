#include "viewer_control_panel.h"

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
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
        connect(objectWidget, &SceneObjectControlWidget::mirrorEnabledChanged, this, [this, index](bool enabled) {
            emit objectMirrorEnabledChanged(index, enabled);
        });
        connect(objectWidget, &SceneObjectControlWidget::mirrorAxisChanged, this, [this, index](int axis) {
            emit objectMirrorAxisChanged(index, axis);
        });
        connect(objectWidget, &SceneObjectControlWidget::mirrorPlaneOffsetChanged, this, [this, index](float planeOffset) {
            emit objectMirrorPlaneOffsetChanged(index, planeOffset);
        });
        connect(objectWidget, &SceneObjectControlWidget::linearArrayEnabledChanged, this, [this, index](bool enabled) {
            emit objectLinearArrayEnabledChanged(index, enabled);
        });
        connect(objectWidget, &SceneObjectControlWidget::linearArrayCountChanged, this, [this, index](int count) {
            emit objectLinearArrayCountChanged(index, count);
        });
        connect(objectWidget, &SceneObjectControlWidget::linearArrayOffsetChanged, this, [this, index](float x, float y, float z) {
            emit objectLinearArrayOffsetChanged(index, x, y, z);
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

    boxWidthSpinBox_ = new QDoubleSpinBox(actionGroup);
    boxWidthSpinBox_->setRange(0.05, 10.0);
    boxWidthSpinBox_->setSingleStep(0.05);
    boxWidthSpinBox_->setDecimals(2);
    connect(boxWidthSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit boxWidthChanged(static_cast<float>(value));
    });

    boxHeightSpinBox_ = new QDoubleSpinBox(actionGroup);
    boxHeightSpinBox_->setRange(0.05, 10.0);
    boxHeightSpinBox_->setSingleStep(0.05);
    boxHeightSpinBox_->setDecimals(2);
    connect(boxHeightSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit boxHeightChanged(static_cast<float>(value));
    });

    boxDepthSpinBox_ = new QDoubleSpinBox(actionGroup);
    boxDepthSpinBox_->setRange(0.05, 10.0);
    boxDepthSpinBox_->setSingleStep(0.05);
    boxDepthSpinBox_->setDecimals(2);
    connect(boxDepthSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit boxDepthChanged(static_cast<float>(value));
    });

    cylinderRadiusSpinBox_ = new QDoubleSpinBox(actionGroup);
    cylinderRadiusSpinBox_->setRange(0.05, 10.0);
    cylinderRadiusSpinBox_->setSingleStep(0.05);
    cylinderRadiusSpinBox_->setDecimals(2);
    connect(cylinderRadiusSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit cylinderRadiusChanged(static_cast<float>(value));
    });

    cylinderHeightSpinBox_ = new QDoubleSpinBox(actionGroup);
    cylinderHeightSpinBox_->setRange(0.05, 10.0);
    cylinderHeightSpinBox_->setSingleStep(0.05);
    cylinderHeightSpinBox_->setDecimals(2);
    connect(cylinderHeightSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit cylinderHeightChanged(static_cast<float>(value));
    });

    cylinderSegmentsSpinBox_ = new QSpinBox(actionGroup);
    cylinderSegmentsSpinBox_->setRange(3, 128);
    cylinderSegmentsSpinBox_->setSingleStep(1);
    connect(cylinderSegmentsSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        emit cylinderSegmentsChanged(value);
    });

    sphereRadiusSpinBox_ = new QDoubleSpinBox(actionGroup);
    sphereRadiusSpinBox_->setRange(0.05, 10.0);
    sphereRadiusSpinBox_->setSingleStep(0.05);
    sphereRadiusSpinBox_->setDecimals(2);
    connect(sphereRadiusSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit sphereRadiusChanged(static_cast<float>(value));
    });

    sphereSlicesSpinBox_ = new QSpinBox(actionGroup);
    sphereSlicesSpinBox_->setRange(3, 128);
    sphereSlicesSpinBox_->setSingleStep(1);
    connect(sphereSlicesSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        emit sphereSlicesChanged(value);
    });

    sphereStacksSpinBox_ = new QSpinBox(actionGroup);
    sphereStacksSpinBox_->setRange(2, 128);
    sphereStacksSpinBox_->setSingleStep(1);
    connect(sphereStacksSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        emit sphereStacksChanged(value);
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
    actionLayout->addWidget(new QLabel("Box Width", actionGroup));
    actionLayout->addWidget(boxWidthSpinBox_);
    actionLayout->addWidget(new QLabel("Box Height", actionGroup));
    actionLayout->addWidget(boxHeightSpinBox_);
    actionLayout->addWidget(new QLabel("Box Depth", actionGroup));
    actionLayout->addWidget(boxDepthSpinBox_);
    actionLayout->addWidget(new QLabel("Cylinder Radius", actionGroup));
    actionLayout->addWidget(cylinderRadiusSpinBox_);
    actionLayout->addWidget(new QLabel("Cylinder Height", actionGroup));
    actionLayout->addWidget(cylinderHeightSpinBox_);
    actionLayout->addWidget(new QLabel("Cylinder Segments", actionGroup));
    actionLayout->addWidget(cylinderSegmentsSpinBox_);
    actionLayout->addWidget(new QLabel("Sphere Radius", actionGroup));
    actionLayout->addWidget(sphereRadiusSpinBox_);
    actionLayout->addWidget(new QLabel("Sphere Slices", actionGroup));
    actionLayout->addWidget(sphereSlicesSpinBox_);
    actionLayout->addWidget(new QLabel("Sphere Stacks", actionGroup));
    actionLayout->addWidget(sphereStacksSpinBox_);
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
        setObjectOperatorState(index, object.mirror, object.linearArray);
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
    if (boxWidthSpinBox_ != nullptr) {
        const QSignalBlocker blocker(boxWidthSpinBox_);
        boxWidthSpinBox_->setValue(state.box.width);
    }
    if (boxHeightSpinBox_ != nullptr) {
        const QSignalBlocker blocker(boxHeightSpinBox_);
        boxHeightSpinBox_->setValue(state.box.height);
    }
    if (boxDepthSpinBox_ != nullptr) {
        const QSignalBlocker blocker(boxDepthSpinBox_);
        boxDepthSpinBox_->setValue(state.box.depth);
    }
    if (cylinderRadiusSpinBox_ != nullptr) {
        const QSignalBlocker blocker(cylinderRadiusSpinBox_);
        cylinderRadiusSpinBox_->setValue(state.cylinder.radius);
    }
    if (cylinderHeightSpinBox_ != nullptr) {
        const QSignalBlocker blocker(cylinderHeightSpinBox_);
        cylinderHeightSpinBox_->setValue(state.cylinder.height);
    }
    if (cylinderSegmentsSpinBox_ != nullptr) {
        const QSignalBlocker blocker(cylinderSegmentsSpinBox_);
        cylinderSegmentsSpinBox_->setValue(static_cast<int>(state.cylinder.segments));
    }
    if (sphereRadiusSpinBox_ != nullptr) {
        const QSignalBlocker blocker(sphereRadiusSpinBox_);
        sphereRadiusSpinBox_->setValue(state.sphere.radius);
    }
    if (sphereSlicesSpinBox_ != nullptr) {
        const QSignalBlocker blocker(sphereSlicesSpinBox_);
        sphereSlicesSpinBox_->setValue(static_cast<int>(state.sphere.slices));
    }
    if (sphereStacksSpinBox_ != nullptr) {
        const QSignalBlocker blocker(sphereStacksSpinBox_);
        sphereStacksSpinBox_->setValue(static_cast<int>(state.sphere.stacks));
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

void ViewerControlPanel::setObjectOperatorState(
    int index,
    const SceneObjectControlWidget::MirrorState& mirror,
    const SceneObjectControlWidget::LinearArrayState& linearArray)
{
    if (index < 0 || index >= kSceneObjectCount) {
        return;
    }

    objectWidgets_[index]->setOperatorState(mirror, linearArray);
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
