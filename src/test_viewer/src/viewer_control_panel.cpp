#include "viewer_control_panel.h"

#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
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

QString primitiveKindText(renderer::parametric_model::PrimitiveKind kind) {
    switch (kind) {
    case renderer::parametric_model::PrimitiveKind::box:
        return QStringLiteral("Box");
    case renderer::parametric_model::PrimitiveKind::cylinder:
        return QStringLiteral("Cylinder");
    case renderer::parametric_model::PrimitiveKind::sphere:
        return QStringLiteral("Sphere");
    }

    return QStringLiteral("Unknown");
}

QString featureKindText(renderer::parametric_model::FeatureKind kind) {
    switch (kind) {
    case renderer::parametric_model::FeatureKind::primitive:
        return QStringLiteral("Primitive");
    case renderer::parametric_model::FeatureKind::mirror:
        return QStringLiteral("Mirror");
    case renderer::parametric_model::FeatureKind::linear_array:
        return QStringLiteral("Linear Array");
    }

    return QStringLiteral("Unknown");
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

    auto* explorerGroup = new QGroupBox("Object Explorer", sceneTab.content);
    auto* explorerLayout = new QVBoxLayout(explorerGroup);

    explorerLayout->addWidget(new QLabel("Objects", explorerGroup));

    objectListWidget_ = new QListWidget(explorerGroup);
    objectListWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(objectListWidget_, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0 || row >= kSceneObjectCount) {
            return;
        }

        inspectedObjectIndex_ = row;
        inspectedFeatureId_ = 0U;
        refreshFeatureExplorer();
        emit objectSelectionChanged(static_cast<int>(panelState_.objects[inspectedObjectIndex_].id));
    });
    explorerLayout->addWidget(objectListWidget_);

    objectSelectionLabel_ = new QLabel(explorerGroup);
    objectSelectionLabel_->setWordWrap(true);
    objectSelectionLabel_->setTextFormat(Qt::PlainText);
    explorerLayout->addWidget(objectSelectionLabel_);

    activateObjectButton_ = new QPushButton("Activate Selected Object", explorerGroup);
    connect(activateObjectButton_, &QPushButton::clicked, this, [this]() {
        if (inspectedObjectIndex_ < 0 || inspectedObjectIndex_ >= kSceneObjectCount) {
            return;
        }
        emit objectActivationRequested(static_cast<int>(panelState_.objects[inspectedObjectIndex_].id));
    });
    explorerLayout->addWidget(activateObjectButton_);

    focusSelectedObjectButton_ = new QPushButton("Focus Selected Object", explorerGroup);
    connect(focusSelectedObjectButton_, &QPushButton::clicked, this, [this]() {
        emit focusSelectedObjectRequested();
    });
    explorerLayout->addWidget(focusSelectedObjectButton_);

    explorerLayout->addWidget(new QLabel("Features", explorerGroup));

    featureListWidget_ = new QListWidget(explorerGroup);
    featureListWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(featureListWidget_, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) {
            inspectedFeatureId_ = 0U;
            updateFeatureActionState();
            return;
        }

        const auto* item = featureListWidget_->item(row);
        if (item == nullptr) {
            inspectedFeatureId_ = 0U;
            updateFeatureActionState();
            return;
        }

        inspectedFeatureId_ = static_cast<renderer::parametric_model::ParametricFeatureId>(
            item->data(Qt::UserRole).toUInt());
        updateFeatureActionState();
        emit featureSelectionChanged(
            static_cast<int>(panelState_.objects[inspectedObjectIndex_].id),
            static_cast<int>(inspectedFeatureId_));
    });
    explorerLayout->addWidget(featureListWidget_);

    featureSelectionLabel_ = new QLabel(explorerGroup);
    featureSelectionLabel_->setWordWrap(true);
    featureSelectionLabel_->setTextFormat(Qt::PlainText);
    explorerLayout->addWidget(featureSelectionLabel_);

    activateFeatureButton_ = new QPushButton("Activate Selected Feature", explorerGroup);
    connect(activateFeatureButton_, &QPushButton::clicked, this, [this]() {
        if (inspectedObjectIndex_ < 0 || inspectedObjectIndex_ >= kSceneObjectCount || inspectedFeatureId_ == 0U) {
            return;
        }

        emit featureActivationRequested(
            static_cast<int>(panelState_.objects[inspectedObjectIndex_].id),
            static_cast<int>(inspectedFeatureId_));
    });
    explorerLayout->addWidget(activateFeatureButton_);

    featureEnabledCheckBox_ = new QCheckBox("Selected Feature Enabled", explorerGroup);
    connect(featureEnabledCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        if (inspectedObjectIndex_ < 0 || inspectedObjectIndex_ >= kSceneObjectCount) {
            return;
        }

        const auto& objectState = panelState_.objects[inspectedObjectIndex_];
        for (const auto& feature : objectState.features) {
            if (feature.id != inspectedFeatureId_) {
                continue;
            }

            if (feature.kind == renderer::parametric_model::FeatureKind::primitive) {
                const QSignalBlocker blocker(featureEnabledCheckBox_);
                featureEnabledCheckBox_->setChecked(true);
                return;
            }

            emit objectFeatureEnabledChanged(
                inspectedObjectIndex_,
                static_cast<int>(feature.id),
                checked);
            return;
        }
    });
    explorerLayout->addWidget(featureEnabledCheckBox_);

    addMirrorFeatureButton_ = new QPushButton("Add Mirror Feature", explorerGroup);
    connect(addMirrorFeatureButton_, &QPushButton::clicked, this, [this]() {
        emit objectFeatureAddRequested(
            inspectedObjectIndex_,
            static_cast<int>(renderer::parametric_model::FeatureKind::mirror));
    });
    explorerLayout->addWidget(addMirrorFeatureButton_);

    addLinearArrayFeatureButton_ = new QPushButton("Add Linear Array Feature", explorerGroup);
    connect(addLinearArrayFeatureButton_, &QPushButton::clicked, this, [this]() {
        emit objectFeatureAddRequested(
            inspectedObjectIndex_,
            static_cast<int>(renderer::parametric_model::FeatureKind::linear_array));
    });
    explorerLayout->addWidget(addLinearArrayFeatureButton_);

    removeFeatureButton_ = new QPushButton("Remove Selected Feature", explorerGroup);
    connect(removeFeatureButton_, &QPushButton::clicked, this, [this]() {
        emit objectFeatureRemoveRequested(
            inspectedObjectIndex_,
            static_cast<int>(inspectedFeatureId_));
    });
    explorerLayout->addWidget(removeFeatureButton_);

    sceneTab.contentLayout->addWidget(explorerGroup);

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

    refreshObjectExplorer();
    refreshFeatureExplorer();
}

void ViewerControlPanel::setPanelState(const PanelState& state) {
    panelState_ = state;
    const int selectedObjectIndex = findObjectIndexById(panelState_.selection.selectedObjectId);
    if (selectedObjectIndex >= 0) {
        inspectedObjectIndex_ = selectedObjectIndex;
    } else if (inspectedObjectIndex_ < 0 || inspectedObjectIndex_ >= kSceneObjectCount) {
        inspectedObjectIndex_ = 0;
    }
    inspectedFeatureId_ = panelState_.selection.selectedFeatureId;

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

    refreshObjectExplorer();
    refreshFeatureExplorer();
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

void ViewerControlPanel::refreshObjectExplorer() {
    if (objectListWidget_ == nullptr) {
        return;
    }

    const QSignalBlocker blocker(objectListWidget_);
    objectListWidget_->clear();

    for (int index = 0; index < kSceneObjectCount; ++index) {
        const auto& object = panelState_.objects[index];
        const bool isSelected = object.id == panelState_.selection.selectedObjectId;
        const bool isActive = object.id == panelState_.selection.activeObjectId;
        auto* item = new QListWidgetItem(
            QStringLiteral("%1. %2  [id:%3]  (%4 features)%5%6")
                .arg(index + 1)
                .arg(primitiveKindText(object.primitiveKind))
                .arg(object.id)
                .arg(static_cast<int>(object.features.size()))
                .arg(isSelected ? QStringLiteral("  [Selected]") : QString())
                .arg(isActive ? QStringLiteral("  [Active]") : QString()),
            objectListWidget_);
        item->setData(Qt::UserRole, index);
    }

    objectListWidget_->setCurrentRow(inspectedObjectIndex_);
}

void ViewerControlPanel::refreshFeatureExplorer() {
    if (featureListWidget_ == nullptr || inspectedObjectIndex_ < 0 || inspectedObjectIndex_ >= kSceneObjectCount) {
        return;
    }

    const auto& object = panelState_.objects[inspectedObjectIndex_];
    bool hasSelectedFeature = false;
    for (const auto& feature : object.features) {
        if (feature.id == inspectedFeatureId_) {
            hasSelectedFeature = true;
            break;
        }
    }
    if (!hasSelectedFeature) {
        inspectedFeatureId_ = object.features.empty() ? 0U : object.features.front().id;
    }

    const QSignalBlocker blocker(featureListWidget_);
    featureListWidget_->clear();

    int selectedRow = -1;
    for (int row = 0; row < static_cast<int>(object.features.size()); ++row) {
        const auto& feature = object.features[static_cast<std::size_t>(row)];
        const bool isSelected = feature.id == panelState_.selection.selectedFeatureId;
        const bool isActive = feature.id == panelState_.selection.activeFeatureId;
        auto* item = new QListWidgetItem(
            QStringLiteral("%1  [id:%2]  [%3]%4%5")
                .arg(featureKindText(feature.kind))
                .arg(feature.id)
                .arg(feature.enabled ? QStringLiteral("Enabled") : QStringLiteral("Disabled"))
                .arg(isSelected ? QStringLiteral("  [Selected]") : QString())
                .arg(isActive ? QStringLiteral("  [Active]") : QString()),
            featureListWidget_);
        item->setData(Qt::UserRole, static_cast<uint>(feature.id));
        if (feature.id == inspectedFeatureId_) {
            selectedRow = row;
        }
    }

    featureListWidget_->setCurrentRow(selectedRow);
    updateFeatureActionState();
}

void ViewerControlPanel::updateFeatureActionState() {
    if (featureEnabledCheckBox_ == nullptr
        || objectSelectionLabel_ == nullptr
        || featureSelectionLabel_ == nullptr
        || activateObjectButton_ == nullptr
        || activateFeatureButton_ == nullptr
        || focusSelectedObjectButton_ == nullptr
        || addMirrorFeatureButton_ == nullptr
        || addLinearArrayFeatureButton_ == nullptr
        || removeFeatureButton_ == nullptr
        || inspectedObjectIndex_ < 0
        || inspectedObjectIndex_ >= kSceneObjectCount) {
        return;
    }

    const auto& object = panelState_.objects[inspectedObjectIndex_];
    bool hasMirrorFeature = false;
    bool hasLinearArrayFeature = false;
    const FeaturePanelState* selectedFeature = nullptr;

    for (const auto& feature : object.features) {
        if (feature.kind == renderer::parametric_model::FeatureKind::mirror) {
            hasMirrorFeature = true;
        }
        if (feature.kind == renderer::parametric_model::FeatureKind::linear_array) {
            hasLinearArrayFeature = true;
        }
        if (feature.id == inspectedFeatureId_) {
            selectedFeature = &feature;
        }
    }

    addMirrorFeatureButton_->setEnabled(!hasMirrorFeature);
    addLinearArrayFeatureButton_->setEnabled(!hasLinearArrayFeature);
    activateObjectButton_->setEnabled(
        panelState_.selection.selectedObjectId != 0U
        && panelState_.selection.selectedObjectId != panelState_.selection.activeObjectId);
    focusSelectedObjectButton_->setEnabled(panelState_.selection.selectedObjectId != 0U);

    const QSignalBlocker blocker(featureEnabledCheckBox_);
    objectSelectionLabel_->setText(
        QStringLiteral("Selected Object: %1\nActive Object: %2")
            .arg(panelState_.selection.selectedObjectId)
            .arg(panelState_.selection.activeObjectId));
    if (selectedFeature == nullptr) {
        featureEnabledCheckBox_->setChecked(false);
        featureEnabledCheckBox_->setEnabled(false);
        activateFeatureButton_->setEnabled(false);
        removeFeatureButton_->setEnabled(false);
        featureSelectionLabel_->setText(
            QStringLiteral("Selected Feature: none\nActive Feature: %1")
                .arg(panelState_.selection.activeFeatureId));
        return;
    }

    featureSelectionLabel_->setText(
        QStringLiteral("Selected Feature: %1\nActive Feature: %2")
            .arg(panelState_.selection.selectedFeatureId)
            .arg(panelState_.selection.activeFeatureId));
    featureEnabledCheckBox_->setChecked(selectedFeature->enabled);
    const bool removable = selectedFeature->kind != renderer::parametric_model::FeatureKind::primitive;
    featureEnabledCheckBox_->setEnabled(removable);
    activateFeatureButton_->setEnabled(
        panelState_.selection.selectedFeatureId != 0U
        && panelState_.selection.selectedFeatureId != panelState_.selection.activeFeatureId);
    removeFeatureButton_->setEnabled(removable);
}

int ViewerControlPanel::findObjectIndexById(renderer::parametric_model::ParametricObjectId objectId) const {
    for (int index = 0; index < kSceneObjectCount; ++index) {
        if (panelState_.objects[index].id == objectId) {
            return index;
        }
    }
    return -1;
}
