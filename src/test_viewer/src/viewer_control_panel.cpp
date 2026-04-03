#include "viewer_control_panel.h"

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

struct ScrollableTabPage {
    QWidget* page = nullptr;
    QWidget* content = nullptr;
    QVBoxLayout* contentLayout = nullptr;
};

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

ScrollableTabPage makeScrollableTabPage(QWidget* parent) {
    ScrollableTabPage tabPage;
    tabPage.page = new QWidget(parent);

    auto* pageLayout = new QVBoxLayout(tabPage.page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    auto* scrollArea = new QScrollArea(tabPage.page);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    tabPage.content = new QWidget(scrollArea);
    tabPage.contentLayout = new QVBoxLayout(tabPage.content);
    tabPage.contentLayout->setContentsMargins(12, 12, 12, 12);
    tabPage.contentLayout->setSpacing(10);

    scrollArea->setWidget(tabPage.content);
    pageLayout->addWidget(scrollArea);
    return tabPage;
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

    auto* tabWidget = new QTabWidget(this);
    rootLayout->addWidget(tabWidget);

    const auto sceneTab = makeScrollableTabPage(tabWidget);
    const auto cameraTab = makeScrollableTabPage(tabWidget);
    const auto debugTab = makeScrollableTabPage(tabWidget);

    tabWidget->addTab(sceneTab.page, "Scene");
    tabWidget->addTab(cameraTab.page, "Camera");
    tabWidget->addTab(debugTab.page, "Debug");

    for (int index = 0; index < kSceneObjectCount; ++index) {
        const auto primitiveKind =
            index == 0
                ? SceneObjectControlWidget::PrimitivePanelKind::box
                : (index == 1
                       ? SceneObjectControlWidget::PrimitivePanelKind::cylinder
                       : SceneObjectControlWidget::PrimitivePanelKind::sphere);

        auto* objectWidget = new SceneObjectControlWidget(
            QString::fromLatin1(kObjectNames[index]),
            primitiveKind,
            sceneTab.content);
        connect(objectWidget, &SceneObjectControlWidget::visibleChanged, this, [this, index](bool visible) {
            emit objectVisibleChanged(index, visible);
        });
        connect(objectWidget, &SceneObjectControlWidget::rotationSpeedChanged, this, [this, index](float speed) {
            emit objectRotationSpeedChanged(index, speed);
        });
        connect(objectWidget, &SceneObjectControlWidget::colorChanged, this, [this, index](float red, float green, float blue) {
            emit objectColorChanged(index, red, green, blue);
        });
        connect(objectWidget, &SceneObjectControlWidget::boxWidthChanged, this, [this](float width) {
            emit boxWidthChanged(width);
        });
        connect(objectWidget, &SceneObjectControlWidget::boxHeightChanged, this, [this](float height) {
            emit boxHeightChanged(height);
        });
        connect(objectWidget, &SceneObjectControlWidget::boxDepthChanged, this, [this](float depth) {
            emit boxDepthChanged(depth);
        });
        connect(objectWidget, &SceneObjectControlWidget::cylinderRadiusChanged, this, [this](float radius) {
            emit cylinderRadiusChanged(radius);
        });
        connect(objectWidget, &SceneObjectControlWidget::cylinderHeightChanged, this, [this](float height) {
            emit cylinderHeightChanged(height);
        });
        connect(objectWidget, &SceneObjectControlWidget::cylinderSegmentsChanged, this, [this](int segments) {
            emit cylinderSegmentsChanged(segments);
        });
        connect(objectWidget, &SceneObjectControlWidget::sphereRadiusChanged, this, [this](float radius) {
            emit sphereRadiusChanged(radius);
        });
        connect(objectWidget, &SceneObjectControlWidget::sphereSlicesChanged, this, [this](int slices) {
            emit sphereSlicesChanged(slices);
        });
        connect(objectWidget, &SceneObjectControlWidget::sphereStacksChanged, this, [this](int stacks) {
            emit sphereStacksChanged(stacks);
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
        sceneTab.contentLayout->addWidget(objectWidget);
    }

    lightingWidget_ = new LightingControlWidget(sceneTab.content);
    connect(lightingWidget_, &LightingControlWidget::ambientStrengthChanged, this, [this](float strength) {
        emit ambientStrengthChanged(strength);
    });
    connect(lightingWidget_, &LightingControlWidget::lightDirectionChanged, this, [this](float x, float y, float z) {
        emit lightDirectionChanged(x, y, z);
    });

    cameraWidget_ = new CameraControlWidget(cameraTab.content);
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

    auto* cameraActionGroup = new QGroupBox("View Actions", cameraTab.content);
    auto* cameraActionLayout = new QVBoxLayout(cameraActionGroup);

    modelChangeViewStrategyComboBox_ = new QComboBox(cameraActionGroup);
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

    auto* boundsGroup = new QGroupBox("Bounds", debugTab.content);
    auto* boundsLayout = new QVBoxLayout(boundsGroup);
    for (int index = 0; index < kSceneObjectCount; ++index) {
        auto* label = new QLabel(boundsGroup);
        label->setWordWrap(true);
        label->setTextFormat(Qt::PlainText);
        objectBoundsLabels_[index] = label;
        boundsLayout->addWidget(label);
    }

    auto* debugActionGroup = new QGroupBox("Debug Actions", debugTab.content);
    auto* debugActionLayout = new QVBoxLayout(debugActionGroup);

    auto* resetButton = new QPushButton("Reset Defaults", debugActionGroup);
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        emit resetDefaultsRequested();
    });

    auto* sphereFocusButton = new QPushButton("Focus Sphere", cameraActionGroup);
    connect(sphereFocusButton, &QPushButton::clicked, this, [this]() {
        emit focusSphereRequested();
    });

    auto* focusAllButton = new QPushButton("Focus All", cameraActionGroup);
    connect(focusAllButton, &QPushButton::clicked, this, [this]() {
        emit focusAllRequested();
    });

    cameraActionLayout->addWidget(new QLabel("Model Change View", cameraActionGroup));
    cameraActionLayout->addWidget(modelChangeViewStrategyComboBox_);
    cameraActionLayout->addWidget(sphereFocusButton);
    cameraActionLayout->addWidget(focusAllButton);

    debugActionLayout->addWidget(resetButton);

    sceneTab.contentLayout->addWidget(lightingWidget_);
    sceneTab.contentLayout->addStretch(1);

    cameraTab.contentLayout->addWidget(cameraWidget_);
    cameraTab.contentLayout->addWidget(cameraActionGroup);
    cameraTab.contentLayout->addStretch(1);

    debugTab.contentLayout->addWidget(boundsGroup);
    debugTab.contentLayout->addWidget(debugActionGroup);
    debugTab.contentLayout->addStretch(1);
}

void ViewerControlPanel::setPanelState(const PanelState& state) {
    for (int index = 0; index < kSceneObjectCount; ++index) {
        const auto& object = state.objects[index];
        setObjectState(index, object.visible, object.rotationSpeed, object.color);
        setObjectPrimitiveState(index, state);
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

void ViewerControlPanel::setObjectPrimitiveState(int index, const PanelState& state) {
    if (index < 0 || index >= kSceneObjectCount) {
        return;
    }

    if (index == 0) {
        objectWidgets_[index]->setBoxSpec(state.box);
        return;
    }
    if (index == 1) {
        objectWidgets_[index]->setCylinderSpec(state.cylinder);
        return;
    }

    objectWidgets_[index]->setSphereSpec(state.sphere);
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
