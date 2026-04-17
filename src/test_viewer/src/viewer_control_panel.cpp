#include "viewer_control_panel.h"

#include "renderer/parametric_model/construction_schema.h"

#include <algorithm>
#include <string_view>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
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

QString formatEvaluationSummaryText(
    const ViewerControlPanel::EvaluationSummaryPanelState& summary)
{
    return QStringLiteral(
               "Status: %1\nVertices: %2\nIndices: %3\nDiagnostics: %4  warnings:%5  errors:%6")
        .arg(summary.succeeded ? QStringLiteral("succeeded") : QStringLiteral("failed"))
        .arg(summary.vertexCount)
        .arg(summary.indexCount)
        .arg(summary.diagnosticCount)
        .arg(summary.warningCount)
        .arg(summary.errorCount);
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

QString unitKindText(renderer::parametric_model::ParametricUnitKind kind) {
    switch (kind) {
    case renderer::parametric_model::ParametricUnitKind::primitive_generator:
        return QStringLiteral("Primitive Generator");
    case renderer::parametric_model::ParametricUnitKind::mirror_operator:
        return QStringLiteral("Mirror Operator");
    case renderer::parametric_model::ParametricUnitKind::linear_array_operator:
        return QStringLiteral("Linear Array Operator");
    }

    return QStringLiteral("Unknown");
}

QString unitEvaluationStatusText(renderer::parametric_model::ParametricUnitEvaluationStatus status) {
    switch (status) {
    case renderer::parametric_model::ParametricUnitEvaluationStatus::valid:
        return QStringLiteral("valid");
    case renderer::parametric_model::ParametricUnitEvaluationStatus::warning:
        return QStringLiteral("warning");
    case renderer::parametric_model::ParametricUnitEvaluationStatus::error:
        return QStringLiteral("error");
    case renderer::parametric_model::ParametricUnitEvaluationStatus::skipped:
        return QStringLiteral("skipped");
    }

    return QStringLiteral("unknown");
}

QString schemaText(std::string_view text) {
    return QString::fromUtf8(text.data(), static_cast<int>(text.size()));
}

QString constructionKindText(renderer::parametric_model::ParametricConstructionKind kind) {
    return schemaText(renderer::parametric_model::ParametricConstructionSchema::constructionLabel(kind));
}

QString inputKindText(renderer::parametric_model::ParametricInputKind kind) {
    switch (kind) {
    case renderer::parametric_model::ParametricInputKind::node:
        return QStringLiteral("node");
    case renderer::parametric_model::ParametricInputKind::float_value:
        return QStringLiteral("float");
    case renderer::parametric_model::ParametricInputKind::integer_value:
        return QStringLiteral("integer");
    case renderer::parametric_model::ParametricInputKind::vector3:
        return QStringLiteral("vec3");
    case renderer::parametric_model::ParametricInputKind::enum_value:
        return QStringLiteral("enum");
    }

    return QStringLiteral("unknown");
}

QString inputSemanticText(renderer::parametric_model::ParametricInputSemantic semantic) {
    return schemaText(renderer::parametric_model::ParametricConstructionSchema::inputSemanticLabel(semantic));
}

QString nodeKindText(renderer::parametric_model::ParametricNodeKind kind) {
    switch (kind) {
    case renderer::parametric_model::ParametricNodeKind::point:
        return QStringLiteral("Point");
    case renderer::parametric_model::ParametricNodeKind::direction:
        return QStringLiteral("Direction");
    case renderer::parametric_model::ParametricNodeKind::axis:
        return QStringLiteral("Axis");
    case renderer::parametric_model::ParametricNodeKind::plane:
        return QStringLiteral("Plane");
    case renderer::parametric_model::ParametricNodeKind::scalar:
        return QStringLiteral("Scalar");
    }

    return QStringLiteral("Unknown");
}

QString nodeValueText(const renderer::parametric_model::ParametricNodeDescriptor& node) {
    switch (node.kind) {
    case renderer::parametric_model::ParametricNodeKind::point:
        return QStringLiteral("(%1, %2, %3)")
            .arg(node.point.position.x, 0, 'f', 2)
            .arg(node.point.position.y, 0, 'f', 2)
            .arg(node.point.position.z, 0, 'f', 2);
    case renderer::parametric_model::ParametricNodeKind::direction:
        return QStringLiteral("dir(%1, %2, %3)")
            .arg(node.direction.direction.x, 0, 'f', 2)
            .arg(node.direction.direction.y, 0, 'f', 2)
            .arg(node.direction.direction.z, 0, 'f', 2);
    case renderer::parametric_model::ParametricNodeKind::axis:
        return QStringLiteral("origin(%1, %2, %3) dir(%4, %5, %6)")
            .arg(node.axis.origin.x, 0, 'f', 2)
            .arg(node.axis.origin.y, 0, 'f', 2)
            .arg(node.axis.origin.z, 0, 'f', 2)
            .arg(node.axis.direction.x, 0, 'f', 2)
            .arg(node.axis.direction.y, 0, 'f', 2)
            .arg(node.axis.direction.z, 0, 'f', 2);
    case renderer::parametric_model::ParametricNodeKind::plane:
        return QStringLiteral("origin(%1, %2, %3) normal(%4, %5, %6)")
            .arg(node.plane.origin.x, 0, 'f', 2)
            .arg(node.plane.origin.y, 0, 'f', 2)
            .arg(node.plane.origin.z, 0, 'f', 2)
            .arg(node.plane.normal.x, 0, 'f', 2)
            .arg(node.plane.normal.y, 0, 'f', 2)
            .arg(node.plane.normal.z, 0, 'f', 2);
    case renderer::parametric_model::ParametricNodeKind::scalar:
        return QStringLiteral("%1").arg(node.scalar.value, 0, 'f', 4);
    }

    return QStringLiteral("unknown");
}

QString diagnosticSeverityText(renderer::parametric_model::EvaluationDiagnosticSeverity severity) {
    switch (severity) {
    case renderer::parametric_model::EvaluationDiagnosticSeverity::info:
        return QStringLiteral("info");
    case renderer::parametric_model::EvaluationDiagnosticSeverity::warning:
        return QStringLiteral("warning");
    case renderer::parametric_model::EvaluationDiagnosticSeverity::error:
        return QStringLiteral("error");
    }

    return QStringLiteral("unknown");
}

QString diagnosticCodeText(renderer::parametric_model::EvaluationDiagnosticCode code) {
    switch (code) {
    case renderer::parametric_model::EvaluationDiagnosticCode::value_clamped:
        return QStringLiteral("value_clamped");
    case renderer::parametric_model::EvaluationDiagnosticCode::missing_node:
        return QStringLiteral("missing_node");
    case renderer::parametric_model::EvaluationDiagnosticCode::invalid_feature_order:
        return QStringLiteral("invalid_feature_order");
    case renderer::parametric_model::EvaluationDiagnosticCode::empty_model:
        return QStringLiteral("empty_model");
    }

    return QStringLiteral("unknown");
}

QDoubleSpinBox* makeNodeCoordinateSpinBox(QWidget* parent) {
    auto* spinBox = new QDoubleSpinBox(parent);
    spinBox->setRange(-1000.0, 1000.0);
    spinBox->setSingleStep(0.05);
    spinBox->setDecimals(3);
    return spinBox;
}

QString inspectorTitleForObject(const ViewerControlPanel::SceneObjectPanelState& objectState) {
    return QStringLiteral("%1 Object [id:%2]")
        .arg(primitiveKindText(objectState.primitiveKind))
        .arg(objectState.id);
}

QString nodeUsageSummary(
    const ViewerControlPanel::SceneObjectPanelState& objectState,
    renderer::parametric_model::ParametricNodeId nodeId)
{
    QString summary;
    for (const auto& usage : objectState.nodeUsages) {
        if (usage.nodeId != nodeId) {
            continue;
        }

        if (!summary.isEmpty()) {
            summary += QStringLiteral(", ");
        }
        summary += inputSemanticText(usage.semantic);
    }

    return summary;
}

}  // namespace

ViewerControlPanel::ViewerControlPanel(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(320);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* tabWidget = new QTabWidget(this);
    rootLayout->addWidget(tabWidget);

    const auto sceneTab = makeScrollableTabPage(tabWidget);
    const auto cameraTab = makeScrollableTabPage(tabWidget);
    const auto parametricTab = makeScrollableTabPage(tabWidget);
    const auto debugTab = makeScrollableTabPage(tabWidget);

    tabWidget->addTab(sceneTab.page, "Scene");
    tabWidget->addTab(cameraTab.page, "Camera");
    tabWidget->addTab(parametricTab.page, "Parametric");
    tabWidget->addTab(debugTab.page, "Debug");

    auto* explorerGroup = new QGroupBox("Object Explorer", sceneTab.content);
    auto* explorerLayout = new QVBoxLayout(explorerGroup);

    auto* addRow = new QHBoxLayout();
    addBoxObjectButton_ = new QPushButton("Add Box", explorerGroup);
    addCylinderObjectButton_ = new QPushButton("Add Cylinder", explorerGroup);
    addSphereObjectButton_ = new QPushButton("Add Sphere", explorerGroup);
    connect(addBoxObjectButton_, &QPushButton::clicked, this, [this]() {
        emit objectAddRequested(static_cast<int>(renderer::parametric_model::PrimitiveKind::box));
    });
    connect(addCylinderObjectButton_, &QPushButton::clicked, this, [this]() {
        emit objectAddRequested(static_cast<int>(renderer::parametric_model::PrimitiveKind::cylinder));
    });
    connect(addSphereObjectButton_, &QPushButton::clicked, this, [this]() {
        emit objectAddRequested(static_cast<int>(renderer::parametric_model::PrimitiveKind::sphere));
    });
    addRow->addWidget(addBoxObjectButton_);
    addRow->addWidget(addCylinderObjectButton_);
    addRow->addWidget(addSphereObjectButton_);
    explorerLayout->addLayout(addRow);

    explorerLayout->addWidget(new QLabel("Objects", explorerGroup));

    objectListWidget_ = new QListWidget(explorerGroup);
    objectListWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(objectListWidget_, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0 || row >= static_cast<int>(panelState_.objects.size())) {
            inspectedObjectIndex_ = -1;
            inspectedFeatureId_ = 0U;
            inspectedNodeId_ = 0U;
            refreshFeatureExplorer();
            refreshNodeExplorer();
            refreshNodeInspector();
            refreshObjectInspector();
            updateFeatureActionState();
            return;
        }

        inspectedObjectIndex_ = row;
        inspectedFeatureId_ = 0U;
        inspectedNodeId_ = 0U;
        refreshFeatureExplorer();
        refreshNodeExplorer();
        refreshNodeInspector();
        refreshObjectInspector();
        updateFeatureActionState();
        emit objectSelectionChanged(static_cast<int>(panelState_.objects[inspectedObjectIndex_].id));
    });
    explorerLayout->addWidget(objectListWidget_);

    deleteSelectedObjectButton_ = new QPushButton("Delete Selected Object", explorerGroup);
    connect(deleteSelectedObjectButton_, &QPushButton::clicked, this, [this]() {
        emit deleteSelectedObjectRequested();
    });
    explorerLayout->addWidget(deleteSelectedObjectButton_);

    objectSelectionLabel_ = new QLabel(explorerGroup);
    objectSelectionLabel_->setWordWrap(true);
    objectSelectionLabel_->setTextFormat(Qt::PlainText);
    explorerLayout->addWidget(objectSelectionLabel_);

    activateObjectButton_ = new QPushButton("Activate Selected Object", explorerGroup);
    connect(activateObjectButton_, &QPushButton::clicked, this, [this]() {
        const auto* objectState = currentObjectState();
        if (objectState == nullptr) {
            return;
        }
        emit objectActivationRequested(static_cast<int>(objectState->id));
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
        const auto* objectState = currentObjectState();
        if (row < 0 || objectState == nullptr) {
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
            static_cast<int>(objectState->id),
            static_cast<int>(inspectedFeatureId_));
    });
    explorerLayout->addWidget(featureListWidget_);

    featureSelectionLabel_ = new QLabel(explorerGroup);
    featureSelectionLabel_->setWordWrap(true);
    featureSelectionLabel_->setTextFormat(Qt::PlainText);
    explorerLayout->addWidget(featureSelectionLabel_);

    activateFeatureButton_ = new QPushButton("Activate Selected Feature", explorerGroup);
    connect(activateFeatureButton_, &QPushButton::clicked, this, [this]() {
        const auto* objectState = currentObjectState();
        if (objectState == nullptr || inspectedFeatureId_ == 0U) {
            return;
        }

        emit featureActivationRequested(
            static_cast<int>(objectState->id),
            static_cast<int>(inspectedFeatureId_));
    });
    explorerLayout->addWidget(activateFeatureButton_);

    featureEnabledCheckBox_ = new QCheckBox("Selected Feature Enabled", explorerGroup);
    connect(featureEnabledCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        const int objectIndex = currentObjectIndex();
        const auto* objectState = currentObjectState();
        if (objectIndex < 0 || objectState == nullptr) {
            return;
        }

        for (const auto& feature : objectState->features) {
            if (feature.id != inspectedFeatureId_) {
                continue;
            }

            if (feature.kind == renderer::parametric_model::FeatureKind::primitive) {
                const QSignalBlocker blocker(featureEnabledCheckBox_);
                featureEnabledCheckBox_->setChecked(true);
                return;
            }

            emit objectFeatureEnabledChanged(
                objectIndex,
                static_cast<int>(feature.id),
                checked);
            return;
        }
    });
    explorerLayout->addWidget(featureEnabledCheckBox_);

    addMirrorFeatureButton_ = new QPushButton("Add Mirror Feature", explorerGroup);
    connect(addMirrorFeatureButton_, &QPushButton::clicked, this, [this]() {
        const int objectIndex = currentObjectIndex();
        if (objectIndex < 0) {
            return;
        }
        emit objectFeatureAddRequested(
            objectIndex,
            static_cast<int>(renderer::parametric_model::FeatureKind::mirror));
    });
    explorerLayout->addWidget(addMirrorFeatureButton_);

    addLinearArrayFeatureButton_ = new QPushButton("Add Linear Array Feature", explorerGroup);
    connect(addLinearArrayFeatureButton_, &QPushButton::clicked, this, [this]() {
        const int objectIndex = currentObjectIndex();
        if (objectIndex < 0) {
            return;
        }
        emit objectFeatureAddRequested(
            objectIndex,
            static_cast<int>(renderer::parametric_model::FeatureKind::linear_array));
    });
    explorerLayout->addWidget(addLinearArrayFeatureButton_);

    removeFeatureButton_ = new QPushButton("Remove Selected Feature", explorerGroup);
    connect(removeFeatureButton_, &QPushButton::clicked, this, [this]() {
        const int objectIndex = currentObjectIndex();
        if (objectIndex < 0 || inspectedFeatureId_ == 0U) {
            return;
        }
        emit objectFeatureRemoveRequested(
            objectIndex,
            static_cast<int>(inspectedFeatureId_));
    });
    explorerLayout->addWidget(removeFeatureButton_);

    explorerLayout->addWidget(new QLabel("Nodes", explorerGroup));

    nodeListWidget_ = new QListWidget(explorerGroup);
    nodeListWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(nodeListWidget_, &QListWidget::currentRowChanged, this, [this](int row) {
        const auto* objectState = currentObjectState();
        if (row < 0 || objectState == nullptr) {
            inspectedNodeId_ = 0U;
            refreshNodeInspector();
            return;
        }

        const auto* item = nodeListWidget_->item(row);
        if (item == nullptr) {
            inspectedNodeId_ = 0U;
            refreshNodeInspector();
            return;
        }

        inspectedNodeId_ = static_cast<renderer::parametric_model::ParametricNodeId>(
            item->data(Qt::UserRole).toUInt());
        refreshNodeInspector();
    });
    explorerLayout->addWidget(nodeListWidget_);

    auto* nodePositionGroup = new QGroupBox("Selected Node Position", explorerGroup);
    auto* nodePositionLayout = new QFormLayout(nodePositionGroup);
    nodeXSpinBox_ = makeNodeCoordinateSpinBox(nodePositionGroup);
    nodeYSpinBox_ = makeNodeCoordinateSpinBox(nodePositionGroup);
    nodeZSpinBox_ = makeNodeCoordinateSpinBox(nodePositionGroup);
    connect(nodeXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        emitInspectedNodePosition();
    });
    connect(nodeYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        emitInspectedNodePosition();
    });
    connect(nodeZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        emitInspectedNodePosition();
    });
    nodePositionLayout->addRow("X", nodeXSpinBox_);
    nodePositionLayout->addRow("Y", nodeYSpinBox_);
    nodePositionLayout->addRow("Z", nodeZSpinBox_);
    explorerLayout->addWidget(nodePositionGroup);

    sceneTab.contentLayout->addWidget(explorerGroup);

    objectWidgets_[0] = new SceneObjectControlWidget(
        "Box Object",
        SceneObjectControlWidget::PrimitivePanelKind::box,
        sceneTab.content);
    objectWidgets_[1] = new SceneObjectControlWidget(
        "Cylinder Object",
        SceneObjectControlWidget::PrimitivePanelKind::cylinder,
        sceneTab.content);
    objectWidgets_[2] = new SceneObjectControlWidget(
        "Sphere Object",
        SceneObjectControlWidget::PrimitivePanelKind::sphere,
        sceneTab.content);

    for (auto* objectWidget : objectWidgets_) {
        objectWidget->hide();

        connect(objectWidget, &SceneObjectControlWidget::visibleChanged, this, [this](bool visible) {
            const int objectIndex = currentObjectIndex();
            if (objectIndex < 0) {
                return;
            }
            emit objectVisibleChanged(objectIndex, visible);
        });
        connect(objectWidget, &SceneObjectControlWidget::rotationSpeedChanged, this, [this](float speed) {
            const int objectIndex = currentObjectIndex();
            if (objectIndex < 0) {
                return;
            }
            emit objectRotationSpeedChanged(objectIndex, speed);
        });
        connect(objectWidget, &SceneObjectControlWidget::colorChanged, this, [this](float red, float green, float blue) {
            const int objectIndex = currentObjectIndex();
            if (objectIndex < 0) {
                return;
            }
            emit objectColorChanged(objectIndex, red, green, blue);
        });
        connect(objectWidget, &SceneObjectControlWidget::mirrorEnabledChanged, this, [this](bool enabled) {
            const int objectIndex = currentObjectIndex();
            if (objectIndex < 0) {
                return;
            }
            emit objectMirrorEnabledChanged(objectIndex, enabled);
        });
        connect(objectWidget, &SceneObjectControlWidget::mirrorAxisChanged, this, [this](int axis) {
            const int objectIndex = currentObjectIndex();
            if (objectIndex < 0) {
                return;
            }
            emit objectMirrorAxisChanged(objectIndex, axis);
        });
        connect(objectWidget, &SceneObjectControlWidget::mirrorPlaneOffsetChanged, this, [this](float planeOffset) {
            const int objectIndex = currentObjectIndex();
            if (objectIndex < 0) {
                return;
            }
            emit objectMirrorPlaneOffsetChanged(objectIndex, planeOffset);
        });
        connect(objectWidget, &SceneObjectControlWidget::linearArrayEnabledChanged, this, [this](bool enabled) {
            const int objectIndex = currentObjectIndex();
            if (objectIndex < 0) {
                return;
            }
            emit objectLinearArrayEnabledChanged(objectIndex, enabled);
        });
        connect(objectWidget, &SceneObjectControlWidget::linearArrayCountChanged, this, [this](int count) {
            const int objectIndex = currentObjectIndex();
            if (objectIndex < 0) {
                return;
            }
            emit objectLinearArrayCountChanged(objectIndex, count);
        });
        connect(objectWidget, &SceneObjectControlWidget::linearArrayOffsetChanged, this, [this](float x, float y, float z) {
            const int objectIndex = currentObjectIndex();
            if (objectIndex < 0) {
                return;
            }
            emit objectLinearArrayOffsetChanged(objectIndex, x, y, z);
        });
        sceneTab.contentLayout->addWidget(objectWidget);
    }

    connect(objectWidgets_[0], &SceneObjectControlWidget::boxConstructionModeChanged, this, [this](int mode) {
        emit boxConstructionModeChanged(mode);
    });
    connect(objectWidgets_[0], &SceneObjectControlWidget::boxWidthChanged, this, [this](float width) {
        emit boxWidthChanged(width);
    });
    connect(objectWidgets_[0], &SceneObjectControlWidget::boxHeightChanged, this, [this](float height) {
        emit boxHeightChanged(height);
    });
    connect(objectWidgets_[0], &SceneObjectControlWidget::boxDepthChanged, this, [this](float depth) {
        emit boxDepthChanged(depth);
    });
    connect(objectWidgets_[0], &SceneObjectControlWidget::boxCenterChanged, this, [this](float x, float y, float z) {
        emit boxCenterChanged(x, y, z);
    });
    connect(objectWidgets_[0], &SceneObjectControlWidget::boxCornerPointChanged, this, [this](float x, float y, float z) {
        emit boxCornerPointChanged(x, y, z);
    });
    connect(objectWidgets_[0], &SceneObjectControlWidget::boxCornerStartChanged, this, [this](float x, float y, float z) {
        emit boxCornerStartChanged(x, y, z);
    });
    connect(objectWidgets_[0], &SceneObjectControlWidget::boxCornerEndChanged, this, [this](float x, float y, float z) {
        emit boxCornerEndChanged(x, y, z);
    });
    connect(objectWidgets_[1], &SceneObjectControlWidget::cylinderConstructionModeChanged, this, [this](int mode) {
        emit cylinderConstructionModeChanged(mode);
    });
    connect(objectWidgets_[1], &SceneObjectControlWidget::cylinderRadiusChanged, this, [this](float radius) {
        emit cylinderRadiusChanged(radius);
    });
    connect(objectWidgets_[1], &SceneObjectControlWidget::cylinderHeightChanged, this, [this](float height) {
        emit cylinderHeightChanged(height);
    });
    connect(objectWidgets_[1], &SceneObjectControlWidget::cylinderSegmentsChanged, this, [this](int segments) {
        emit cylinderSegmentsChanged(segments);
    });
    connect(objectWidgets_[1], &SceneObjectControlWidget::cylinderCenterChanged, this, [this](float x, float y, float z) {
        emit cylinderCenterChanged(x, y, z);
    });
    connect(objectWidgets_[1], &SceneObjectControlWidget::cylinderRadiusPointChanged, this, [this](float x, float y, float z) {
        emit cylinderRadiusPointChanged(x, y, z);
    });
    connect(objectWidgets_[1], &SceneObjectControlWidget::cylinderAxisStartChanged, this, [this](float x, float y, float z) {
        emit cylinderAxisStartChanged(x, y, z);
    });
    connect(objectWidgets_[1], &SceneObjectControlWidget::cylinderAxisEndChanged, this, [this](float x, float y, float z) {
        emit cylinderAxisEndChanged(x, y, z);
    });
    connect(objectWidgets_[2], &SceneObjectControlWidget::sphereRadiusChanged, this, [this](float radius) {
        emit sphereRadiusChanged(radius);
    });
    connect(objectWidgets_[2], &SceneObjectControlWidget::sphereSlicesChanged, this, [this](int slices) {
        emit sphereSlicesChanged(slices);
    });
    connect(objectWidgets_[2], &SceneObjectControlWidget::sphereStacksChanged, this, [this](int stacks) {
        emit sphereStacksChanged(stacks);
    });
    connect(objectWidgets_[2], &SceneObjectControlWidget::sphereConstructionModeChanged, this, [this](int mode) {
        emit sphereConstructionModeChanged(mode);
    });
    connect(objectWidgets_[2], &SceneObjectControlWidget::sphereCenterChanged, this, [this](float x, float y, float z) {
        emit sphereCenterChanged(x, y, z);
    });
    connect(objectWidgets_[2], &SceneObjectControlWidget::sphereSurfacePointChanged, this, [this](float x, float y, float z) {
        emit sphereSurfacePointChanged(x, y, z);
    });
    connect(objectWidgets_[2], &SceneObjectControlWidget::sphereDiameterStartChanged, this, [this](float x, float y, float z) {
        emit sphereDiameterStartChanged(x, y, z);
    });
    connect(objectWidgets_[2], &SceneObjectControlWidget::sphereDiameterEndChanged, this, [this](float x, float y, float z) {
        emit sphereDiameterEndChanged(x, y, z);
    });

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

    auto* boundsGroup = new QGroupBox("Bounds", debugTab.content);
    auto* boundsLayout = new QVBoxLayout(boundsGroup);
    boundsListWidget_ = new QListWidget(boundsGroup);
    boundsListWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    boundsLayout->addWidget(boundsListWidget_);

    auto* parametricBoundsGroup = new QGroupBox("Model Bounds", parametricTab.content);
    auto* parametricBoundsLayout = new QVBoxLayout(parametricBoundsGroup);
    parametricBoundsListWidget_ = new QListWidget(parametricBoundsGroup);
    parametricBoundsListWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    parametricBoundsLayout->addWidget(parametricBoundsListWidget_);

    auto* evaluationSummaryGroup = new QGroupBox("Evaluation Summary", parametricTab.content);
    auto* evaluationSummaryLayout = new QVBoxLayout(evaluationSummaryGroup);
    evaluationSummaryLabel_ = new QLabel("No active object", evaluationSummaryGroup);
    evaluationSummaryLabel_->setWordWrap(true);
    evaluationSummaryLayout->addWidget(evaluationSummaryLabel_);

    auto* overlayGroup = new QGroupBox("Viewport Display", parametricTab.content);
    auto* overlayLayout = new QVBoxLayout(overlayGroup);
    showParametricBoundsCheckBox_ = new QCheckBox("Model Bounds", overlayGroup);
    showParametricNodesCheckBox_ = new QCheckBox("Node Points", overlayGroup);
    showParametricLinksCheckBox_ = new QCheckBox("Construction Links", overlayGroup);
    showParametricBoundsCheckBox_->setChecked(true);
    showParametricNodesCheckBox_->setChecked(true);
    showParametricLinksCheckBox_->setChecked(true);
    connect(showParametricBoundsCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit parametricOverlayModelBoundsChanged(checked);
    });
    connect(showParametricNodesCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit parametricOverlayNodePointsChanged(checked);
    });
    connect(showParametricLinksCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit parametricOverlayConstructionLinksChanged(checked);
    });
    overlayLayout->addWidget(showParametricBoundsCheckBox_);
    overlayLayout->addWidget(showParametricNodesCheckBox_);
    overlayLayout->addWidget(showParametricLinksCheckBox_);
    auto* overlayHintLabel = new QLabel(
        "Debug overlay only. These marks do not affect picking or model geometry.",
        overlayGroup);
    overlayHintLabel->setWordWrap(true);
    overlayLayout->addWidget(overlayHintLabel);

    auto* unitGroup = new QGroupBox("Units", parametricTab.content);
    auto* unitLayout = new QVBoxLayout(unitGroup);
    unitListWidget_ = new QListWidget(unitGroup);
    unitListWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    unitLayout->addWidget(unitListWidget_);

    auto* unitEvaluationGroup = new QGroupBox("Unit Evaluation", parametricTab.content);
    auto* unitEvaluationLayout = new QVBoxLayout(unitEvaluationGroup);
    unitEvaluationListWidget_ = new QListWidget(unitEvaluationGroup);
    unitEvaluationListWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    unitEvaluationLayout->addWidget(unitEvaluationListWidget_);

    auto* unitInputGroup = new QGroupBox("Unit Inputs", parametricTab.content);
    auto* unitInputLayout = new QVBoxLayout(unitInputGroup);
    unitInputListWidget_ = new QListWidget(unitInputGroup);
    unitInputListWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    unitInputLayout->addWidget(unitInputListWidget_);

    auto* nodeUsageGroup = new QGroupBox("Node Usage", parametricTab.content);
    auto* nodeUsageLayout = new QVBoxLayout(nodeUsageGroup);
    nodeUsageListWidget_ = new QListWidget(nodeUsageGroup);
    nodeUsageListWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    nodeUsageLayout->addWidget(nodeUsageListWidget_);

    auto* constructionLinkGroup = new QGroupBox("Construction Links", parametricTab.content);
    auto* constructionLinkLayout = new QVBoxLayout(constructionLinkGroup);
    constructionLinkListWidget_ = new QListWidget(constructionLinkGroup);
    constructionLinkListWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    constructionLinkLayout->addWidget(constructionLinkListWidget_);

    auto* derivedParameterGroup = new QGroupBox("Derived Parameters", parametricTab.content);
    auto* derivedParameterLayout = new QVBoxLayout(derivedParameterGroup);
    derivedParameterListWidget_ = new QListWidget(derivedParameterGroup);
    derivedParameterListWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    derivedParameterLayout->addWidget(derivedParameterListWidget_);

    auto* evaluationDiagnosticGroup = new QGroupBox("Evaluation Diagnostics", parametricTab.content);
    auto* evaluationDiagnosticLayout = new QVBoxLayout(evaluationDiagnosticGroup);
    evaluationDiagnosticListWidget_ = new QListWidget(evaluationDiagnosticGroup);
    evaluationDiagnosticListWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    evaluationDiagnosticLayout->addWidget(evaluationDiagnosticListWidget_);

    auto* debugActionGroup = new QGroupBox("Debug Actions", debugTab.content);
    auto* debugActionLayout = new QVBoxLayout(debugActionGroup);

    auto* resetButton = new QPushButton("Reset Defaults", debugActionGroup);
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        emit resetDefaultsRequested();
    });
    debugActionLayout->addWidget(resetButton);

    sceneTab.contentLayout->addWidget(lightingWidget_);
    sceneTab.contentLayout->addStretch(1);

    cameraTab.contentLayout->addWidget(cameraWidget_);
    cameraTab.contentLayout->addWidget(cameraActionGroup);
    cameraTab.contentLayout->addStretch(1);

    parametricTab.contentLayout->addWidget(evaluationSummaryGroup);
    parametricTab.contentLayout->addWidget(parametricBoundsGroup);
    parametricTab.contentLayout->addWidget(overlayGroup);
    parametricTab.contentLayout->addWidget(unitGroup);
    parametricTab.contentLayout->addWidget(unitEvaluationGroup);
    parametricTab.contentLayout->addWidget(unitInputGroup);
    parametricTab.contentLayout->addWidget(nodeUsageGroup);
    parametricTab.contentLayout->addWidget(constructionLinkGroup);
    parametricTab.contentLayout->addWidget(derivedParameterGroup);
    parametricTab.contentLayout->addWidget(evaluationDiagnosticGroup);
    parametricTab.contentLayout->addStretch(1);

    debugTab.contentLayout->addWidget(boundsGroup);
    debugTab.contentLayout->addWidget(debugActionGroup);
    debugTab.contentLayout->addStretch(1);

    refreshObjectExplorer();
    refreshFeatureExplorer();
    refreshNodeExplorer();
    refreshNodeInspector();
    refreshObjectInspector();
    refreshBoundsList();
    refreshParametricDebugPage();
    updateFeatureActionState();
}

void ViewerControlPanel::setPanelState(const PanelState& state) {
    panelState_ = state;
    const int selectedObjectIndex = findObjectIndexById(panelState_.selection.selectedObjectId);
    if (selectedObjectIndex >= 0) {
        inspectedObjectIndex_ = selectedObjectIndex;
    } else if (panelState_.objects.empty()) {
        inspectedObjectIndex_ = -1;
    } else if (inspectedObjectIndex_ < 0 || inspectedObjectIndex_ >= static_cast<int>(panelState_.objects.size())) {
        inspectedObjectIndex_ = 0;
    }
    inspectedFeatureId_ = panelState_.selection.selectedFeatureId;

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
    refreshNodeExplorer();
    refreshNodeInspector();
    refreshObjectInspector();
    refreshBoundsList();
    refreshParametricDebugPage();
    updateFeatureActionState();
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

    for (int index = 0; index < static_cast<int>(panelState_.objects.size()); ++index) {
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

    if (panelState_.objects.empty()) {
        objectListWidget_->setCurrentRow(-1);
        inspectedObjectIndex_ = -1;
        return;
    }

    if (inspectedObjectIndex_ < 0 || inspectedObjectIndex_ >= static_cast<int>(panelState_.objects.size())) {
        inspectedObjectIndex_ = 0;
    }
    objectListWidget_->setCurrentRow(inspectedObjectIndex_);
}

void ViewerControlPanel::refreshFeatureExplorer() {
    if (featureListWidget_ == nullptr) {
        return;
    }

    const auto* objectState = currentObjectState();
    const QSignalBlocker blocker(featureListWidget_);
    featureListWidget_->clear();

    if (objectState == nullptr) {
        inspectedFeatureId_ = 0U;
        updateFeatureActionState();
        return;
    }

    bool hasSelectedFeature = false;
    for (const auto& feature : objectState->features) {
        if (feature.id == inspectedFeatureId_) {
            hasSelectedFeature = true;
            break;
        }
    }
    if (!hasSelectedFeature) {
        inspectedFeatureId_ = objectState->features.empty() ? 0U : objectState->features.front().id;
    }

    int selectedRow = -1;
    for (int row = 0; row < static_cast<int>(objectState->features.size()); ++row) {
        const auto& feature = objectState->features[static_cast<std::size_t>(row)];
        const bool isSelected = feature.id == panelState_.selection.selectedFeatureId;
        const bool isActive = feature.id == panelState_.selection.activeFeatureId;
        auto* item = new QListWidgetItem(
            QStringLiteral("%1  [feature:%2]  [unit:%3]  [%4]%5%6")
                .arg(featureKindText(feature.kind))
                .arg(feature.id)
                .arg(feature.unitId)
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

void ViewerControlPanel::refreshNodeExplorer() {
    if (nodeListWidget_ == nullptr) {
        return;
    }

    const auto* objectState = currentObjectState();
    const QSignalBlocker blocker(nodeListWidget_);
    nodeListWidget_->clear();

    if (objectState == nullptr || objectState->nodes.empty()) {
        inspectedNodeId_ = 0U;
        nodeListWidget_->setCurrentRow(-1);
        return;
    }

    bool hasInspectedNode = false;
    for (const auto& node : objectState->nodes) {
        if (node.id == inspectedNodeId_) {
            hasInspectedNode = true;
            break;
        }
    }
    if (!hasInspectedNode) {
        inspectedNodeId_ = objectState->nodes.front().id;
    }

    int selectedRow = -1;
    for (int row = 0; row < static_cast<int>(objectState->nodes.size()); ++row) {
        const auto& node = objectState->nodes[static_cast<std::size_t>(row)];
        const auto usageSummary = nodeUsageSummary(*objectState, node.id);
        auto* item = new QListWidgetItem(
            QStringLiteral("%1 [id:%2]%3  %4")
                .arg(nodeKindText(node.kind))
                .arg(node.id)
                .arg(!usageSummary.isEmpty() ? QStringLiteral("  [%1]").arg(usageSummary) : QString())
                .arg(nodeValueText(node)),
            nodeListWidget_);
        item->setData(Qt::UserRole, static_cast<uint>(node.id));
        if (node.id == inspectedNodeId_) {
            selectedRow = row;
        }
    }

    nodeListWidget_->setCurrentRow(selectedRow);
}

void ViewerControlPanel::refreshNodeInspector() {
    if (nodeXSpinBox_ == nullptr || nodeYSpinBox_ == nullptr || nodeZSpinBox_ == nullptr) {
        return;
    }

    const auto* objectState = currentObjectState();
    const renderer::parametric_model::ParametricNodeDescriptor* selectedNode = nullptr;
    if (objectState != nullptr) {
        for (const auto& node : objectState->nodes) {
            if (node.id == inspectedNodeId_) {
                selectedNode = &node;
                break;
            }
        }
    }

    const QSignalBlocker xBlocker(nodeXSpinBox_);
    const QSignalBlocker yBlocker(nodeYSpinBox_);
    const QSignalBlocker zBlocker(nodeZSpinBox_);
    const bool hasEditablePointNode =
        selectedNode != nullptr
        && selectedNode->kind == renderer::parametric_model::ParametricNodeKind::point;
    nodeXSpinBox_->setEnabled(hasEditablePointNode);
    nodeYSpinBox_->setEnabled(hasEditablePointNode);
    nodeZSpinBox_->setEnabled(hasEditablePointNode);
    nodeXSpinBox_->setValue(hasEditablePointNode ? selectedNode->point.position.x : 0.0);
    nodeYSpinBox_->setValue(hasEditablePointNode ? selectedNode->point.position.y : 0.0);
    nodeZSpinBox_->setValue(hasEditablePointNode ? selectedNode->point.position.z : 0.0);
}

void ViewerControlPanel::refreshObjectInspector() {
    for (auto* objectWidget : objectWidgets_) {
        if (objectWidget != nullptr) {
            objectWidget->hide();
        }
    }

    const auto* objectState = currentObjectState();
    auto* objectWidget = currentInspectorWidget();
    if (objectState == nullptr || objectWidget == nullptr) {
        return;
    }

    objectWidget->setTitle(inspectorTitleForObject(*objectState));
    objectWidget->setObjectState(objectState->visible, objectState->rotationSpeed, objectState->color);
    objectWidget->setOperatorState(objectState->mirror, objectState->linearArray);

    switch (objectState->primitive.kind) {
    case renderer::parametric_model::PrimitiveKind::box:
        objectWidget->setBoxSpec(objectState->primitive.box, objectState->nodes);
        break;
    case renderer::parametric_model::PrimitiveKind::cylinder:
        objectWidget->setCylinderSpec(objectState->primitive.cylinder, objectState->nodes);
        break;
    case renderer::parametric_model::PrimitiveKind::sphere:
        objectWidget->setSphereSpec(objectState->primitive.sphere, objectState->nodes);
        break;
    }

    objectWidget->show();
}

void ViewerControlPanel::refreshBoundsList() {
    if (boundsListWidget_ == nullptr) {
        return;
    }

    const QSignalBlocker blocker(boundsListWidget_);
    boundsListWidget_->clear();

    for (int index = 0; index < static_cast<int>(panelState_.objects.size()); ++index) {
        const auto& object = panelState_.objects[index];
        boundsListWidget_->addItem(formatBoundsText(
            QStringLiteral("%1. %2 [id:%3]")
                .arg(index + 1)
                .arg(primitiveKindText(object.primitiveKind))
                .arg(object.id),
            object.bounds));
    }
}

void ViewerControlPanel::refreshParametricDebugPage() {
    if (evaluationSummaryLabel_ == nullptr
        || parametricBoundsListWidget_ == nullptr
        || unitListWidget_ == nullptr
        || unitEvaluationListWidget_ == nullptr
        || unitInputListWidget_ == nullptr
        || nodeUsageListWidget_ == nullptr
        || constructionLinkListWidget_ == nullptr
        || derivedParameterListWidget_ == nullptr
        || evaluationDiagnosticListWidget_ == nullptr) {
        return;
    }

    const auto* objectState = currentObjectState();

    evaluationSummaryLabel_->setText(
        objectState != nullptr
            ? formatEvaluationSummaryText(objectState->evaluationSummary)
            : QStringLiteral("No active object"));

    const QSignalBlocker boundsBlocker(parametricBoundsListWidget_);
    parametricBoundsListWidget_->clear();
    if (objectState != nullptr) {
        parametricBoundsListWidget_->addItem(formatBoundsText("Active model bounds", objectState->bounds));
    }

    const QSignalBlocker unitBlocker(unitListWidget_);
    unitListWidget_->clear();
    if (objectState != nullptr) {
        if (objectState->units.empty()) {
            unitListWidget_->addItem(QStringLiteral("No units"));
        }
        for (const auto& unit : objectState->units) {
            unitListWidget_->addItem(
                QStringLiteral("unit:%1  feature:%2\n%3\n%4")
                    .arg(unit.id)
                    .arg(unit.featureId)
                    .arg(unitKindText(unit.kind))
                    .arg(constructionKindText(unit.constructionKind)));
        }
    }

    const QSignalBlocker unitEvaluationBlocker(unitEvaluationListWidget_);
    unitEvaluationListWidget_->clear();
    if (objectState != nullptr) {
        if (objectState->unitEvaluations.empty()) {
            unitEvaluationListWidget_->addItem(QStringLiteral("No unit evaluations"));
        }
        for (const auto& evaluation : objectState->unitEvaluations) {
            unitEvaluationListWidget_->addItem(
                QStringLiteral("unit:%1  feature:%2  [%3]\ndiagnostics:%4  warnings:%5  errors:%6")
                    .arg(evaluation.unitId)
                    .arg(evaluation.featureId)
                    .arg(unitEvaluationStatusText(evaluation.status))
                    .arg(evaluation.diagnosticCount)
                    .arg(evaluation.warningCount)
                    .arg(evaluation.errorCount));
        }
    }

    const QSignalBlocker inputBlocker(unitInputListWidget_);
    unitInputListWidget_->clear();
    if (objectState != nullptr) {
        if (objectState->unitInputs.empty()) {
            unitInputListWidget_->addItem(QStringLiteral("No unit inputs"));
        }
        for (const auto& input : objectState->unitInputs) {
            unitInputListWidget_->addItem(
                QStringLiteral("unit:%1  feature:%2  %3 : %4%5")
                    .arg(input.unitId)
                    .arg(input.featureId)
                    .arg(inputSemanticText(input.semantic))
                    .arg(inputKindText(input.kind))
                    .arg(input.nodeId != 0U ? QStringLiteral("  node:%1").arg(input.nodeId) : QString()));
        }
    }

    const QSignalBlocker usageBlocker(nodeUsageListWidget_);
    nodeUsageListWidget_->clear();
    if (objectState != nullptr) {
        if (objectState->nodeUsages.empty()) {
            nodeUsageListWidget_->addItem(QStringLiteral("No node usages"));
        }
        for (const auto& usage : objectState->nodeUsages) {
            nodeUsageListWidget_->addItem(
                QStringLiteral("node:%1 -> unit:%2  feature:%3  as %4")
                    .arg(usage.nodeId)
                    .arg(usage.unitId)
                    .arg(usage.featureId)
                    .arg(inputSemanticText(usage.semantic)));
        }
    }

    const QSignalBlocker parameterBlocker(derivedParameterListWidget_);
    derivedParameterListWidget_->clear();
    if (objectState != nullptr) {
        if (objectState->derivedParameters.empty()) {
            derivedParameterListWidget_->addItem(QStringLiteral("No derived parameters"));
        }
        for (const auto& parameter : objectState->derivedParameters) {
            derivedParameterListWidget_->addItem(
                QStringLiteral("unit:%1  feature:%2\n%3 = %4\nnode:%5 -> node:%6")
                    .arg(parameter.unitId)
                    .arg(parameter.featureId)
                    .arg(inputSemanticText(parameter.semantic))
                    .arg(parameter.value, 0, 'f', 4)
                    .arg(parameter.referenceNodeId)
                    .arg(parameter.sourceNodeId));
        }
    }

    const QSignalBlocker diagnosticBlocker(evaluationDiagnosticListWidget_);
    evaluationDiagnosticListWidget_->clear();
    if (objectState != nullptr) {
        if (objectState->evaluationDiagnostics.empty()) {
            evaluationDiagnosticListWidget_->addItem(QStringLiteral("No diagnostics"));
        }
        for (const auto& diagnostic : objectState->evaluationDiagnostics) {
            evaluationDiagnosticListWidget_->addItem(
                QStringLiteral("%1  %2\nfeature:%3  node:%4\n%5")
                    .arg(diagnosticSeverityText(diagnostic.severity))
                    .arg(diagnosticCodeText(diagnostic.code))
                    .arg(diagnostic.featureId)
                    .arg(diagnostic.nodeId)
                    .arg(QString::fromStdString(diagnostic.message)));
        }
    }

    const QSignalBlocker linkBlocker(constructionLinkListWidget_);
    constructionLinkListWidget_->clear();
    if (objectState != nullptr) {
        if (objectState->constructionLinks.empty()) {
            constructionLinkListWidget_->addItem(QStringLiteral("No construction links"));
        }
        for (const auto& link : objectState->constructionLinks) {
            constructionLinkListWidget_->addItem(
                QStringLiteral("unit:%1  feature:%2\n%3\nnode:%4 (%5) -> node:%6 (%7)")
                    .arg(link.unitId)
                    .arg(link.featureId)
                    .arg(constructionKindText(link.constructionKind))
                    .arg(link.startNodeId)
                    .arg(inputSemanticText(link.startSemantic))
                    .arg(link.endNodeId)
                    .arg(inputSemanticText(link.endSemantic)));
        }
    }
}

void ViewerControlPanel::updateFeatureActionState() {
    if (objectSelectionLabel_ == nullptr
        || featureSelectionLabel_ == nullptr
        || featureEnabledCheckBox_ == nullptr
        || activateObjectButton_ == nullptr
        || deleteSelectedObjectButton_ == nullptr
        || activateFeatureButton_ == nullptr
        || focusSelectedObjectButton_ == nullptr
        || addMirrorFeatureButton_ == nullptr
        || addLinearArrayFeatureButton_ == nullptr
        || removeFeatureButton_ == nullptr) {
        return;
    }

    const auto* objectState = currentObjectState();
    const bool hasObject = objectState != nullptr;
    activateObjectButton_->setEnabled(
        hasObject
        && panelState_.selection.selectedObjectId != 0U
        && panelState_.selection.selectedObjectId != panelState_.selection.activeObjectId);
    deleteSelectedObjectButton_->setEnabled(hasObject);
    focusSelectedObjectButton_->setEnabled(panelState_.selection.selectedObjectId != 0U);
    objectSelectionLabel_->setText(
        QStringLiteral("Selected Object: %1\nActive Object: %2")
            .arg(panelState_.selection.selectedObjectId != 0U
                     ? QString::number(panelState_.selection.selectedObjectId)
                     : QStringLiteral("none"))
            .arg(panelState_.selection.activeObjectId != 0U
                     ? QString::number(panelState_.selection.activeObjectId)
                     : QStringLiteral("none")));

    if (!hasObject) {
        const QSignalBlocker blocker(featureEnabledCheckBox_);
        featureEnabledCheckBox_->setChecked(false);
        featureEnabledCheckBox_->setEnabled(false);
        activateFeatureButton_->setEnabled(false);
        addMirrorFeatureButton_->setEnabled(false);
        addLinearArrayFeatureButton_->setEnabled(false);
        removeFeatureButton_->setEnabled(false);
        featureSelectionLabel_->setText(QStringLiteral("Selected Feature: none\nActive Feature: none"));
        return;
    }

    bool hasMirrorFeature = false;
    bool hasLinearArrayFeature = false;
    const FeaturePanelState* selectedFeature = nullptr;

    for (const auto& feature : objectState->features) {
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

    const QSignalBlocker blocker(featureEnabledCheckBox_);
    if (selectedFeature == nullptr) {
        featureEnabledCheckBox_->setChecked(false);
        featureEnabledCheckBox_->setEnabled(false);
        activateFeatureButton_->setEnabled(false);
        removeFeatureButton_->setEnabled(false);
        featureSelectionLabel_->setText(
            QStringLiteral("Selected Feature: none\nActive Feature: %1")
                .arg(panelState_.selection.activeFeatureId != 0U
                         ? QString::number(panelState_.selection.activeFeatureId)
                         : QStringLiteral("none")));
        return;
    }

    featureSelectionLabel_->setText(
        QStringLiteral("Selected Feature: %1\nActive Feature: %2")
            .arg(panelState_.selection.selectedFeatureId != 0U
                     ? QString::number(panelState_.selection.selectedFeatureId)
                     : QStringLiteral("none"))
            .arg(panelState_.selection.activeFeatureId != 0U
                     ? QString::number(panelState_.selection.activeFeatureId)
                     : QStringLiteral("none")));
    featureEnabledCheckBox_->setChecked(selectedFeature->enabled);
    const bool removable = selectedFeature->kind != renderer::parametric_model::FeatureKind::primitive;
    featureEnabledCheckBox_->setEnabled(removable);
    activateFeatureButton_->setEnabled(
        panelState_.selection.selectedFeatureId != 0U
        && panelState_.selection.selectedFeatureId != panelState_.selection.activeFeatureId);
    removeFeatureButton_->setEnabled(removable);
}

void ViewerControlPanel::emitInspectedNodePosition() {
    const int objectIndex = currentObjectIndex();
    if (objectIndex < 0
        || inspectedNodeId_ == 0U
        || nodeXSpinBox_ == nullptr
        || nodeYSpinBox_ == nullptr
        || nodeZSpinBox_ == nullptr) {
        return;
    }

    emit objectNodePositionChanged(
        objectIndex,
        static_cast<int>(inspectedNodeId_),
        static_cast<float>(nodeXSpinBox_->value()),
        static_cast<float>(nodeYSpinBox_->value()),
        static_cast<float>(nodeZSpinBox_->value()));
}

int ViewerControlPanel::findObjectIndexById(renderer::parametric_model::ParametricObjectId objectId) const {
    for (int index = 0; index < static_cast<int>(panelState_.objects.size()); ++index) {
        if (panelState_.objects[index].id == objectId) {
            return index;
        }
    }
    return -1;
}

int ViewerControlPanel::currentObjectIndex() const {
    if (inspectedObjectIndex_ < 0 || inspectedObjectIndex_ >= static_cast<int>(panelState_.objects.size())) {
        return -1;
    }
    return inspectedObjectIndex_;
}

const ViewerControlPanel::SceneObjectPanelState* ViewerControlPanel::currentObjectState() const {
    const int objectIndex = currentObjectIndex();
    if (objectIndex < 0) {
        return nullptr;
    }
    return &panelState_.objects[objectIndex];
}

SceneObjectControlWidget* ViewerControlPanel::currentInspectorWidget() const {
    const auto* objectState = currentObjectState();
    if (objectState == nullptr) {
        return nullptr;
    }

    switch (objectState->primitive.kind) {
    case renderer::parametric_model::PrimitiveKind::box:
        return objectWidgets_[0];
    case renderer::parametric_model::PrimitiveKind::cylinder:
        return objectWidgets_[1];
    case renderer::parametric_model::PrimitiveKind::sphere:
        return objectWidgets_[2];
    }

    return nullptr;
}
