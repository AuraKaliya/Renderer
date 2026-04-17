#include "scene_object_control_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

#include <cmath>

namespace {

QDoubleSpinBox* makeFloatSpinBox(
    QWidget* parent,
    double minimum,
    double maximum,
    double step,
    int decimals)
{
    auto* spinBox = new QDoubleSpinBox(parent);
    spinBox->setRange(minimum, maximum);
    spinBox->setSingleStep(step);
    spinBox->setDecimals(decimals);
    return spinBox;
}

const renderer::parametric_model::ParametricNodeDescriptor* findPointNode(
    const std::vector<renderer::parametric_model::ParametricNodeDescriptor>& nodes,
    renderer::parametric_model::NodeReference reference)
{
    for (const auto& node : nodes) {
        if (node.id == reference.id && node.kind == renderer::parametric_model::ParametricNodeKind::point) {
            return &node;
        }
    }
    return nullptr;
}

}  // namespace

SceneObjectControlWidget::SceneObjectControlWidget(
    const QString& title,
    PrimitivePanelKind primitiveKind,
    QWidget* parent)
    : QWidget(parent)
    , primitiveKind_(primitiveKind)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    groupBox_ = new QGroupBox(title, this);
    auto* groupLayout = new QVBoxLayout(groupBox_);

    auto* objectGroup = new QGroupBox("Object", groupBox_);
    auto* objectLayout = new QFormLayout(objectGroup);

    auto* appearanceGroup = new QGroupBox("Appearance", groupBox_);
    auto* appearanceLayout = new QFormLayout(appearanceGroup);

    auto* primitiveGroup = new QGroupBox("Primitive", groupBox_);
    auto* primitiveLayout = new QFormLayout(primitiveGroup);

    auto* operatorGroup = new QGroupBox("Operators", groupBox_);
    auto* operatorLayout = new QFormLayout(operatorGroup);

    visibleCheckBox_ = new QCheckBox(groupBox_);
    connect(visibleCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit visibleChanged(checked);
    });

    speedSpinBox_ = makeFloatSpinBox(groupBox_, -5.0, 5.0, 0.05, 2);
    connect(speedSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit rotationSpeedChanged(static_cast<float>(value));
    });

    redSpinBox_ = makeFloatSpinBox(groupBox_, 0.0, 1.0, 0.05, 2);
    greenSpinBox_ = makeFloatSpinBox(groupBox_, 0.0, 1.0, 0.05, 2);
    blueSpinBox_ = makeFloatSpinBox(groupBox_, 0.0, 1.0, 0.05, 2);

    auto emitColor = [this]() {
        emit colorChanged(
            static_cast<float>(redSpinBox_->value()),
            static_cast<float>(greenSpinBox_->value()),
            static_cast<float>(blueSpinBox_->value()));
    };

    connect(redSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitColor](double) {
        emitColor();
    });
    connect(greenSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitColor](double) {
        emitColor();
    });
    connect(blueSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitColor](double) {
        emitColor();
    });

    objectLayout->addRow("Visible", visibleCheckBox_);
    objectLayout->addRow("Rotation", speedSpinBox_);

    appearanceLayout->addRow("Red", redSpinBox_);
    appearanceLayout->addRow("Green", greenSpinBox_);
    appearanceLayout->addRow("Blue", blueSpinBox_);

    if (primitiveKind_ == PrimitivePanelKind::box) {
        boxConstructionModeComboBox_ = new QComboBox(groupBox_);
        boxConstructionModeComboBox_->addItem(
            "Center + Size",
            static_cast<int>(renderer::parametric_model::BoxSpec::ConstructionMode::center_size));
        boxConstructionModeComboBox_->addItem(
            "Center + Corner Point",
            static_cast<int>(renderer::parametric_model::BoxSpec::ConstructionMode::center_corner_point));
        boxConstructionModeComboBox_->addItem(
            "Two Corner Points",
            static_cast<int>(renderer::parametric_model::BoxSpec::ConstructionMode::corner_points));
        boxWidthSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        boxHeightSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        boxDepthSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        boxCenterXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCenterYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCenterZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCornerPointXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCornerPointYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCornerPointZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCornerStartXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCornerStartYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCornerStartZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCornerEndXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCornerEndYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        boxCornerEndZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);

        connect(boxConstructionModeComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
            emit boxConstructionModeChanged(boxConstructionModeComboBox_->itemData(index).toInt());
        });
        connect(boxWidthSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit boxWidthChanged(static_cast<float>(value));
        });
        connect(boxHeightSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit boxHeightChanged(static_cast<float>(value));
        });
        connect(boxDepthSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit boxDepthChanged(static_cast<float>(value));
        });

        auto emitBoxCenter = [this]() {
            emit boxCenterChanged(
                static_cast<float>(boxCenterXSpinBox_->value()),
                static_cast<float>(boxCenterYSpinBox_->value()),
                static_cast<float>(boxCenterZSpinBox_->value()));
        };
        auto emitBoxCornerPoint = [this]() {
            emit boxCornerPointChanged(
                static_cast<float>(boxCornerPointXSpinBox_->value()),
                static_cast<float>(boxCornerPointYSpinBox_->value()),
                static_cast<float>(boxCornerPointZSpinBox_->value()));
        };
        auto emitBoxCornerStart = [this]() {
            emit boxCornerStartChanged(
                static_cast<float>(boxCornerStartXSpinBox_->value()),
                static_cast<float>(boxCornerStartYSpinBox_->value()),
                static_cast<float>(boxCornerStartZSpinBox_->value()));
        };
        auto emitBoxCornerEnd = [this]() {
            emit boxCornerEndChanged(
                static_cast<float>(boxCornerEndXSpinBox_->value()),
                static_cast<float>(boxCornerEndYSpinBox_->value()),
                static_cast<float>(boxCornerEndZSpinBox_->value()));
        };
        connect(boxCenterXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCenter](double) {
            emitBoxCenter();
        });
        connect(boxCenterYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCenter](double) {
            emitBoxCenter();
        });
        connect(boxCenterZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCenter](double) {
            emitBoxCenter();
        });
        connect(boxCornerPointXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCornerPoint](double) {
            emitBoxCornerPoint();
        });
        connect(boxCornerPointYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCornerPoint](double) {
            emitBoxCornerPoint();
        });
        connect(boxCornerPointZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCornerPoint](double) {
            emitBoxCornerPoint();
        });
        connect(boxCornerStartXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCornerStart](double) {
            emitBoxCornerStart();
        });
        connect(boxCornerStartYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCornerStart](double) {
            emitBoxCornerStart();
        });
        connect(boxCornerStartZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCornerStart](double) {
            emitBoxCornerStart();
        });
        connect(boxCornerEndXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCornerEnd](double) {
            emitBoxCornerEnd();
        });
        connect(boxCornerEndYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCornerEnd](double) {
            emitBoxCornerEnd();
        });
        connect(boxCornerEndZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitBoxCornerEnd](double) {
            emitBoxCornerEnd();
        });

        primitiveLayout->addRow("Mode", boxConstructionModeComboBox_);
        primitiveLayout->addRow("Width", boxWidthSpinBox_);
        primitiveLayout->addRow("Height", boxHeightSpinBox_);
        primitiveLayout->addRow("Depth", boxDepthSpinBox_);
        primitiveLayout->addRow("Center X", boxCenterXSpinBox_);
        primitiveLayout->addRow("Center Y", boxCenterYSpinBox_);
        primitiveLayout->addRow("Center Z", boxCenterZSpinBox_);
        primitiveLayout->addRow("Corner X", boxCornerPointXSpinBox_);
        primitiveLayout->addRow("Corner Y", boxCornerPointYSpinBox_);
        primitiveLayout->addRow("Corner Z", boxCornerPointZSpinBox_);
        primitiveLayout->addRow("Corner A X", boxCornerStartXSpinBox_);
        primitiveLayout->addRow("Corner A Y", boxCornerStartYSpinBox_);
        primitiveLayout->addRow("Corner A Z", boxCornerStartZSpinBox_);
        primitiveLayout->addRow("Corner B X", boxCornerEndXSpinBox_);
        primitiveLayout->addRow("Corner B Y", boxCornerEndYSpinBox_);
        primitiveLayout->addRow("Corner B Z", boxCornerEndZSpinBox_);
    } else if (primitiveKind_ == PrimitivePanelKind::cylinder) {
        cylinderConstructionModeComboBox_ = new QComboBox(groupBox_);
        cylinderConstructionModeComboBox_->addItem(
            "Center + Radius + Height",
            static_cast<int>(renderer::parametric_model::CylinderSpec::ConstructionMode::center_radius_height));
        cylinderConstructionModeComboBox_->addItem(
            "Center + Radius Point + Height",
            static_cast<int>(renderer::parametric_model::CylinderSpec::ConstructionMode::center_radius_point_height));
        cylinderConstructionModeComboBox_->addItem(
            "Axis Endpoints + Radius",
            static_cast<int>(renderer::parametric_model::CylinderSpec::ConstructionMode::axis_endpoints_radius));
        cylinderRadiusSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        cylinderHeightSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        cylinderSegmentsSpinBox_ = new QSpinBox(groupBox_);
        cylinderSegmentsSpinBox_->setRange(3, 128);
        cylinderSegmentsSpinBox_->setSingleStep(1);
        cylinderCenterXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderCenterYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderCenterZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderRadiusPointXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderRadiusPointYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderRadiusPointZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderAxisStartXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderAxisStartYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderAxisStartZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderAxisEndXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderAxisEndYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        cylinderAxisEndZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);

        connect(cylinderConstructionModeComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
            emit cylinderConstructionModeChanged(cylinderConstructionModeComboBox_->itemData(index).toInt());
        });
        connect(cylinderRadiusSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit cylinderRadiusChanged(static_cast<float>(value));
        });
        connect(cylinderHeightSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit cylinderHeightChanged(static_cast<float>(value));
        });
        connect(cylinderSegmentsSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            emit cylinderSegmentsChanged(value);
        });

        auto emitCylinderCenter = [this]() {
            emit cylinderCenterChanged(
                static_cast<float>(cylinderCenterXSpinBox_->value()),
                static_cast<float>(cylinderCenterYSpinBox_->value()),
                static_cast<float>(cylinderCenterZSpinBox_->value()));
        };
        auto emitCylinderRadiusPoint = [this]() {
            emit cylinderRadiusPointChanged(
                static_cast<float>(cylinderRadiusPointXSpinBox_->value()),
                static_cast<float>(cylinderRadiusPointYSpinBox_->value()),
                static_cast<float>(cylinderRadiusPointZSpinBox_->value()));
        };
        auto emitCylinderAxisStart = [this]() {
            emit cylinderAxisStartChanged(
                static_cast<float>(cylinderAxisStartXSpinBox_->value()),
                static_cast<float>(cylinderAxisStartYSpinBox_->value()),
                static_cast<float>(cylinderAxisStartZSpinBox_->value()));
        };
        auto emitCylinderAxisEnd = [this]() {
            emit cylinderAxisEndChanged(
                static_cast<float>(cylinderAxisEndXSpinBox_->value()),
                static_cast<float>(cylinderAxisEndYSpinBox_->value()),
                static_cast<float>(cylinderAxisEndZSpinBox_->value()));
        };
        connect(cylinderCenterXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderCenter](double) {
            emitCylinderCenter();
        });
        connect(cylinderCenterYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderCenter](double) {
            emitCylinderCenter();
        });
        connect(cylinderCenterZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderCenter](double) {
            emitCylinderCenter();
        });
        connect(cylinderRadiusPointXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderRadiusPoint](double) {
            emitCylinderRadiusPoint();
        });
        connect(cylinderRadiusPointYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderRadiusPoint](double) {
            emitCylinderRadiusPoint();
        });
        connect(cylinderRadiusPointZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderRadiusPoint](double) {
            emitCylinderRadiusPoint();
        });
        connect(cylinderAxisStartXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderAxisStart](double) {
            emitCylinderAxisStart();
        });
        connect(cylinderAxisStartYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderAxisStart](double) {
            emitCylinderAxisStart();
        });
        connect(cylinderAxisStartZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderAxisStart](double) {
            emitCylinderAxisStart();
        });
        connect(cylinderAxisEndXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderAxisEnd](double) {
            emitCylinderAxisEnd();
        });
        connect(cylinderAxisEndYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderAxisEnd](double) {
            emitCylinderAxisEnd();
        });
        connect(cylinderAxisEndZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitCylinderAxisEnd](double) {
            emitCylinderAxisEnd();
        });

        primitiveLayout->addRow("Mode", cylinderConstructionModeComboBox_);
        primitiveLayout->addRow("Radius", cylinderRadiusSpinBox_);
        primitiveLayout->addRow("Height", cylinderHeightSpinBox_);
        primitiveLayout->addRow("Segments", cylinderSegmentsSpinBox_);
        primitiveLayout->addRow("Center X", cylinderCenterXSpinBox_);
        primitiveLayout->addRow("Center Y", cylinderCenterYSpinBox_);
        primitiveLayout->addRow("Center Z", cylinderCenterZSpinBox_);
        primitiveLayout->addRow("Radius X", cylinderRadiusPointXSpinBox_);
        primitiveLayout->addRow("Radius Y", cylinderRadiusPointYSpinBox_);
        primitiveLayout->addRow("Radius Z", cylinderRadiusPointZSpinBox_);
        primitiveLayout->addRow("Axis A X", cylinderAxisStartXSpinBox_);
        primitiveLayout->addRow("Axis A Y", cylinderAxisStartYSpinBox_);
        primitiveLayout->addRow("Axis A Z", cylinderAxisStartZSpinBox_);
        primitiveLayout->addRow("Axis B X", cylinderAxisEndXSpinBox_);
        primitiveLayout->addRow("Axis B Y", cylinderAxisEndYSpinBox_);
        primitiveLayout->addRow("Axis B Z", cylinderAxisEndZSpinBox_);
    } else {
        sphereRadiusSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        sphereSlicesSpinBox_ = new QSpinBox(groupBox_);
        sphereSlicesSpinBox_->setRange(3, 128);
        sphereSlicesSpinBox_->setSingleStep(1);
        sphereStacksSpinBox_ = new QSpinBox(groupBox_);
        sphereStacksSpinBox_->setRange(2, 128);
        sphereStacksSpinBox_->setSingleStep(1);
        sphereConstructionModeComboBox_ = new QComboBox(groupBox_);
        sphereConstructionModeComboBox_->addItem(
            "Center + Radius",
            static_cast<int>(renderer::parametric_model::SphereSpec::ConstructionMode::center_radius));
        sphereConstructionModeComboBox_->addItem(
            "Center + Surface Point",
            static_cast<int>(renderer::parametric_model::SphereSpec::ConstructionMode::center_surface_point));
        sphereConstructionModeComboBox_->addItem(
            "Diameter Points",
            static_cast<int>(renderer::parametric_model::SphereSpec::ConstructionMode::diameter_points));
        sphereCenterXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereCenterYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereCenterZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereSurfacePointXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereSurfacePointYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereSurfacePointZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereDiameterStartXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereDiameterStartYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereDiameterStartZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereDiameterEndXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereDiameterEndYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
        sphereDiameterEndZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);

        connect(sphereRadiusSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit sphereRadiusChanged(static_cast<float>(value));
        });
        connect(sphereSlicesSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            emit sphereSlicesChanged(value);
        });
        connect(sphereStacksSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            emit sphereStacksChanged(value);
        });
        connect(sphereConstructionModeComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
            emit sphereConstructionModeChanged(sphereConstructionModeComboBox_->itemData(index).toInt());
        });

        auto emitSphereCenter = [this]() {
            emit sphereCenterChanged(
                static_cast<float>(sphereCenterXSpinBox_->value()),
                static_cast<float>(sphereCenterYSpinBox_->value()),
                static_cast<float>(sphereCenterZSpinBox_->value()));
        };
        auto emitSphereSurfacePoint = [this]() {
            emit sphereSurfacePointChanged(
                static_cast<float>(sphereSurfacePointXSpinBox_->value()),
                static_cast<float>(sphereSurfacePointYSpinBox_->value()),
                static_cast<float>(sphereSurfacePointZSpinBox_->value()));
        };
        auto emitSphereDiameterStart = [this]() {
            emit sphereDiameterStartChanged(
                static_cast<float>(sphereDiameterStartXSpinBox_->value()),
                static_cast<float>(sphereDiameterStartYSpinBox_->value()),
                static_cast<float>(sphereDiameterStartZSpinBox_->value()));
        };
        auto emitSphereDiameterEnd = [this]() {
            emit sphereDiameterEndChanged(
                static_cast<float>(sphereDiameterEndXSpinBox_->value()),
                static_cast<float>(sphereDiameterEndYSpinBox_->value()),
                static_cast<float>(sphereDiameterEndZSpinBox_->value()));
        };
        connect(sphereCenterXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereCenter](double) {
            emitSphereCenter();
        });
        connect(sphereCenterYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereCenter](double) {
            emitSphereCenter();
        });
        connect(sphereCenterZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereCenter](double) {
            emitSphereCenter();
        });
        connect(sphereSurfacePointXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereSurfacePoint](double) {
            emitSphereSurfacePoint();
        });
        connect(sphereSurfacePointYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereSurfacePoint](double) {
            emitSphereSurfacePoint();
        });
        connect(sphereSurfacePointZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereSurfacePoint](double) {
            emitSphereSurfacePoint();
        });
        connect(sphereDiameterStartXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereDiameterStart](double) {
            emitSphereDiameterStart();
        });
        connect(sphereDiameterStartYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereDiameterStart](double) {
            emitSphereDiameterStart();
        });
        connect(sphereDiameterStartZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereDiameterStart](double) {
            emitSphereDiameterStart();
        });
        connect(sphereDiameterEndXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereDiameterEnd](double) {
            emitSphereDiameterEnd();
        });
        connect(sphereDiameterEndYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereDiameterEnd](double) {
            emitSphereDiameterEnd();
        });
        connect(sphereDiameterEndZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitSphereDiameterEnd](double) {
            emitSphereDiameterEnd();
        });

        primitiveLayout->addRow("Mode", sphereConstructionModeComboBox_);
        primitiveLayout->addRow("Radius", sphereRadiusSpinBox_);
        primitiveLayout->addRow("Slices", sphereSlicesSpinBox_);
        primitiveLayout->addRow("Stacks", sphereStacksSpinBox_);
        primitiveLayout->addRow("Center X", sphereCenterXSpinBox_);
        primitiveLayout->addRow("Center Y", sphereCenterYSpinBox_);
        primitiveLayout->addRow("Center Z", sphereCenterZSpinBox_);
        primitiveLayout->addRow("Surface X", sphereSurfacePointXSpinBox_);
        primitiveLayout->addRow("Surface Y", sphereSurfacePointYSpinBox_);
        primitiveLayout->addRow("Surface Z", sphereSurfacePointZSpinBox_);
        primitiveLayout->addRow("Diameter A X", sphereDiameterStartXSpinBox_);
        primitiveLayout->addRow("Diameter A Y", sphereDiameterStartYSpinBox_);
        primitiveLayout->addRow("Diameter A Z", sphereDiameterStartZSpinBox_);
        primitiveLayout->addRow("Diameter B X", sphereDiameterEndXSpinBox_);
        primitiveLayout->addRow("Diameter B Y", sphereDiameterEndYSpinBox_);
        primitiveLayout->addRow("Diameter B Z", sphereDiameterEndZSpinBox_);
    }

    mirrorEnabledCheckBox_ = new QCheckBox(groupBox_);
    connect(mirrorEnabledCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit mirrorEnabledChanged(checked);
    });

    mirrorAxisComboBox_ = new QComboBox(groupBox_);
    mirrorAxisComboBox_->addItem("X", static_cast<int>(renderer::parametric_model::Axis::x));
    mirrorAxisComboBox_->addItem("Y", static_cast<int>(renderer::parametric_model::Axis::y));
    mirrorAxisComboBox_->addItem("Z", static_cast<int>(renderer::parametric_model::Axis::z));
    connect(mirrorAxisComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        emit mirrorAxisChanged(mirrorAxisComboBox_->itemData(index).toInt());
    });

    mirrorPlaneOffsetSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
    connect(mirrorPlaneOffsetSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit mirrorPlaneOffsetChanged(static_cast<float>(value));
    });

    linearArrayEnabledCheckBox_ = new QCheckBox(groupBox_);
    connect(linearArrayEnabledCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit linearArrayEnabledChanged(checked);
    });

    linearArrayCountSpinBox_ = new QSpinBox(groupBox_);
    linearArrayCountSpinBox_->setRange(1, 64);
    connect(linearArrayCountSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        emit linearArrayCountChanged(value);
    });

    linearArrayOffsetXSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
    linearArrayOffsetYSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);
    linearArrayOffsetZSpinBox_ = makeFloatSpinBox(groupBox_, -10.0, 10.0, 0.1, 2);

    auto emitLinearArrayOffset = [this]() {
        emit linearArrayOffsetChanged(
            static_cast<float>(linearArrayOffsetXSpinBox_->value()),
            static_cast<float>(linearArrayOffsetYSpinBox_->value()),
            static_cast<float>(linearArrayOffsetZSpinBox_->value()));
    };

    connect(linearArrayOffsetXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitLinearArrayOffset](double) {
        emitLinearArrayOffset();
    });
    connect(linearArrayOffsetYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitLinearArrayOffset](double) {
        emitLinearArrayOffset();
    });
    connect(linearArrayOffsetZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitLinearArrayOffset](double) {
        emitLinearArrayOffset();
    });

    operatorLayout->addRow("Mirror Enabled", mirrorEnabledCheckBox_);
    operatorLayout->addRow("Mirror Axis", mirrorAxisComboBox_);
    operatorLayout->addRow("Mirror Offset", mirrorPlaneOffsetSpinBox_);
    operatorLayout->addRow("Array Enabled", linearArrayEnabledCheckBox_);
    operatorLayout->addRow("Array Count", linearArrayCountSpinBox_);
    operatorLayout->addRow("Array Offset X", linearArrayOffsetXSpinBox_);
    operatorLayout->addRow("Array Offset Y", linearArrayOffsetYSpinBox_);
    operatorLayout->addRow("Array Offset Z", linearArrayOffsetZSpinBox_);

    groupLayout->addWidget(objectGroup);
    groupLayout->addWidget(appearanceGroup);
    groupLayout->addWidget(primitiveGroup);
    groupLayout->addWidget(operatorGroup);
    rootLayout->addWidget(groupBox_);
}

void SceneObjectControlWidget::setTitle(const QString& title) {
    if (groupBox_ == nullptr) {
        return;
    }

    groupBox_->setTitle(title);
}

void SceneObjectControlWidget::setObjectState(
    bool visible,
    float rotationSpeed,
    const renderer::scene_contract::ColorRgba& color)
{
    const QSignalBlocker visibleBlocker(visibleCheckBox_);
    const QSignalBlocker speedBlocker(speedSpinBox_);
    const QSignalBlocker redBlocker(redSpinBox_);
    const QSignalBlocker greenBlocker(greenSpinBox_);
    const QSignalBlocker blueBlocker(blueSpinBox_);

    visibleCheckBox_->setChecked(visible);
    speedSpinBox_->setValue(rotationSpeed);
    redSpinBox_->setValue(color.r);
    greenSpinBox_->setValue(color.g);
    blueSpinBox_->setValue(color.b);
}

void SceneObjectControlWidget::setOperatorState(
    const MirrorState& mirror,
    const LinearArrayState& linearArray)
{
    const QSignalBlocker mirrorEnabledBlocker(mirrorEnabledCheckBox_);
    const QSignalBlocker mirrorAxisBlocker(mirrorAxisComboBox_);
    const QSignalBlocker mirrorPlaneOffsetBlocker(mirrorPlaneOffsetSpinBox_);
    const QSignalBlocker linearArrayEnabledBlocker(linearArrayEnabledCheckBox_);
    const QSignalBlocker linearArrayCountBlocker(linearArrayCountSpinBox_);
    const QSignalBlocker linearArrayOffsetXBlocker(linearArrayOffsetXSpinBox_);
    const QSignalBlocker linearArrayOffsetYBlocker(linearArrayOffsetYSpinBox_);
    const QSignalBlocker linearArrayOffsetZBlocker(linearArrayOffsetZSpinBox_);

    mirrorEnabledCheckBox_->setChecked(mirror.enabled);
    const int mirrorAxisIndex = mirrorAxisComboBox_->findData(mirror.axis);
    if (mirrorAxisIndex >= 0) {
        mirrorAxisComboBox_->setCurrentIndex(mirrorAxisIndex);
    }
    mirrorPlaneOffsetSpinBox_->setValue(mirror.planeOffset);

    linearArrayEnabledCheckBox_->setChecked(linearArray.enabled);
    linearArrayCountSpinBox_->setValue(linearArray.count);
    linearArrayOffsetXSpinBox_->setValue(linearArray.offset.x);
    linearArrayOffsetYSpinBox_->setValue(linearArray.offset.y);
    linearArrayOffsetZSpinBox_->setValue(linearArray.offset.z);
}

void SceneObjectControlWidget::setBoxSpec(
    const renderer::parametric_model::BoxSpec& spec,
    const std::vector<renderer::parametric_model::ParametricNodeDescriptor>& nodes)
{
    if (boxConstructionModeComboBox_ == nullptr
        || boxWidthSpinBox_ == nullptr
        || boxHeightSpinBox_ == nullptr
        || boxDepthSpinBox_ == nullptr
        || boxCenterXSpinBox_ == nullptr
        || boxCenterYSpinBox_ == nullptr
        || boxCenterZSpinBox_ == nullptr
        || boxCornerPointXSpinBox_ == nullptr
        || boxCornerPointYSpinBox_ == nullptr
        || boxCornerPointZSpinBox_ == nullptr
        || boxCornerStartXSpinBox_ == nullptr
        || boxCornerStartYSpinBox_ == nullptr
        || boxCornerStartZSpinBox_ == nullptr
        || boxCornerEndXSpinBox_ == nullptr
        || boxCornerEndYSpinBox_ == nullptr
        || boxCornerEndZSpinBox_ == nullptr) {
        return;
    }

    const QSignalBlocker modeBlocker(boxConstructionModeComboBox_);
    const QSignalBlocker widthBlocker(boxWidthSpinBox_);
    const QSignalBlocker heightBlocker(boxHeightSpinBox_);
    const QSignalBlocker depthBlocker(boxDepthSpinBox_);
    const QSignalBlocker centerXBlocker(boxCenterXSpinBox_);
    const QSignalBlocker centerYBlocker(boxCenterYSpinBox_);
    const QSignalBlocker centerZBlocker(boxCenterZSpinBox_);
    const QSignalBlocker cornerPointXBlocker(boxCornerPointXSpinBox_);
    const QSignalBlocker cornerPointYBlocker(boxCornerPointYSpinBox_);
    const QSignalBlocker cornerPointZBlocker(boxCornerPointZSpinBox_);
    const QSignalBlocker cornerStartXBlocker(boxCornerStartXSpinBox_);
    const QSignalBlocker cornerStartYBlocker(boxCornerStartYSpinBox_);
    const QSignalBlocker cornerStartZBlocker(boxCornerStartZSpinBox_);
    const QSignalBlocker cornerEndXBlocker(boxCornerEndXSpinBox_);
    const QSignalBlocker cornerEndYBlocker(boxCornerEndYSpinBox_);
    const QSignalBlocker cornerEndZBlocker(boxCornerEndZSpinBox_);

    const int modeIndex = boxConstructionModeComboBox_->findData(static_cast<int>(spec.constructionMode));
    if (modeIndex >= 0) {
        boxConstructionModeComboBox_->setCurrentIndex(modeIndex);
    }
    boxWidthSpinBox_->setValue(spec.width);
    boxHeightSpinBox_->setValue(spec.height);
    boxDepthSpinBox_->setValue(spec.depth);

    renderer::scene_contract::Vec3f center {};
    if (const auto* centerNode = findPointNode(nodes, spec.center); centerNode != nullptr) {
        center = centerNode->point.position;
    }
    renderer::scene_contract::Vec3f cornerPoint {
        center.x + spec.width * 0.5F,
        center.y + spec.height * 0.5F,
        center.z + spec.depth * 0.5F
    };
    if (const auto* cornerNode = findPointNode(nodes, spec.cornerPoint); cornerNode != nullptr) {
        cornerPoint = cornerNode->point.position;
    }
    renderer::scene_contract::Vec3f cornerStart {
        center.x - spec.width * 0.5F,
        center.y - spec.height * 0.5F,
        center.z - spec.depth * 0.5F
    };
    renderer::scene_contract::Vec3f cornerEnd {
        center.x + spec.width * 0.5F,
        center.y + spec.height * 0.5F,
        center.z + spec.depth * 0.5F
    };
    if (const auto* startNode = findPointNode(nodes, spec.cornerStart); startNode != nullptr) {
        cornerStart = startNode->point.position;
    }
    if (const auto* endNode = findPointNode(nodes, spec.cornerEnd); endNode != nullptr) {
        cornerEnd = endNode->point.position;
    }
    if (spec.constructionMode == renderer::parametric_model::BoxSpec::ConstructionMode::corner_points) {
        center = {
            (cornerStart.x + cornerEnd.x) * 0.5F,
            (cornerStart.y + cornerEnd.y) * 0.5F,
            (cornerStart.z + cornerEnd.z) * 0.5F
        };
        cornerPoint = cornerEnd;
    }

    boxCenterXSpinBox_->setValue(center.x);
    boxCenterYSpinBox_->setValue(center.y);
    boxCenterZSpinBox_->setValue(center.z);
    boxCornerPointXSpinBox_->setValue(cornerPoint.x);
    boxCornerPointYSpinBox_->setValue(cornerPoint.y);
    boxCornerPointZSpinBox_->setValue(cornerPoint.z);
    boxCornerStartXSpinBox_->setValue(cornerStart.x);
    boxCornerStartYSpinBox_->setValue(cornerStart.y);
    boxCornerStartZSpinBox_->setValue(cornerStart.z);
    boxCornerEndXSpinBox_->setValue(cornerEnd.x);
    boxCornerEndYSpinBox_->setValue(cornerEnd.y);
    boxCornerEndZSpinBox_->setValue(cornerEnd.z);

    const bool usesCornerPoint =
        spec.constructionMode == renderer::parametric_model::BoxSpec::ConstructionMode::center_corner_point;
    const bool usesCornerPoints =
        spec.constructionMode == renderer::parametric_model::BoxSpec::ConstructionMode::corner_points;
    boxWidthSpinBox_->setEnabled(!usesCornerPoint && !usesCornerPoints);
    boxHeightSpinBox_->setEnabled(!usesCornerPoint && !usesCornerPoints);
    boxDepthSpinBox_->setEnabled(!usesCornerPoint && !usesCornerPoints);
    boxCenterXSpinBox_->setEnabled(!usesCornerPoints);
    boxCenterYSpinBox_->setEnabled(!usesCornerPoints);
    boxCenterZSpinBox_->setEnabled(!usesCornerPoints);
    boxCornerPointXSpinBox_->setEnabled(usesCornerPoint);
    boxCornerPointYSpinBox_->setEnabled(usesCornerPoint);
    boxCornerPointZSpinBox_->setEnabled(usesCornerPoint);
    boxCornerStartXSpinBox_->setEnabled(usesCornerPoints);
    boxCornerStartYSpinBox_->setEnabled(usesCornerPoints);
    boxCornerStartZSpinBox_->setEnabled(usesCornerPoints);
    boxCornerEndXSpinBox_->setEnabled(usesCornerPoints);
    boxCornerEndYSpinBox_->setEnabled(usesCornerPoints);
    boxCornerEndZSpinBox_->setEnabled(usesCornerPoints);
}

void SceneObjectControlWidget::setCylinderSpec(
    const renderer::parametric_model::CylinderSpec& spec,
    const std::vector<renderer::parametric_model::ParametricNodeDescriptor>& nodes)
{
    if (cylinderConstructionModeComboBox_ == nullptr
        || cylinderRadiusSpinBox_ == nullptr
        || cylinderHeightSpinBox_ == nullptr
        || cylinderSegmentsSpinBox_ == nullptr
        || cylinderCenterXSpinBox_ == nullptr
        || cylinderCenterYSpinBox_ == nullptr
        || cylinderCenterZSpinBox_ == nullptr
        || cylinderRadiusPointXSpinBox_ == nullptr
        || cylinderRadiusPointYSpinBox_ == nullptr
        || cylinderRadiusPointZSpinBox_ == nullptr
        || cylinderAxisStartXSpinBox_ == nullptr
        || cylinderAxisStartYSpinBox_ == nullptr
        || cylinderAxisStartZSpinBox_ == nullptr
        || cylinderAxisEndXSpinBox_ == nullptr
        || cylinderAxisEndYSpinBox_ == nullptr
        || cylinderAxisEndZSpinBox_ == nullptr) {
        return;
    }

    const QSignalBlocker modeBlocker(cylinderConstructionModeComboBox_);
    const QSignalBlocker radiusBlocker(cylinderRadiusSpinBox_);
    const QSignalBlocker heightBlocker(cylinderHeightSpinBox_);
    const QSignalBlocker segmentsBlocker(cylinderSegmentsSpinBox_);
    const QSignalBlocker centerXBlocker(cylinderCenterXSpinBox_);
    const QSignalBlocker centerYBlocker(cylinderCenterYSpinBox_);
    const QSignalBlocker centerZBlocker(cylinderCenterZSpinBox_);
    const QSignalBlocker radiusPointXBlocker(cylinderRadiusPointXSpinBox_);
    const QSignalBlocker radiusPointYBlocker(cylinderRadiusPointYSpinBox_);
    const QSignalBlocker radiusPointZBlocker(cylinderRadiusPointZSpinBox_);
    const QSignalBlocker axisStartXBlocker(cylinderAxisStartXSpinBox_);
    const QSignalBlocker axisStartYBlocker(cylinderAxisStartYSpinBox_);
    const QSignalBlocker axisStartZBlocker(cylinderAxisStartZSpinBox_);
    const QSignalBlocker axisEndXBlocker(cylinderAxisEndXSpinBox_);
    const QSignalBlocker axisEndYBlocker(cylinderAxisEndYSpinBox_);
    const QSignalBlocker axisEndZBlocker(cylinderAxisEndZSpinBox_);

    const int modeIndex = cylinderConstructionModeComboBox_->findData(static_cast<int>(spec.constructionMode));
    if (modeIndex >= 0) {
        cylinderConstructionModeComboBox_->setCurrentIndex(modeIndex);
    }
    cylinderRadiusSpinBox_->setValue(spec.radius);
    cylinderHeightSpinBox_->setValue(spec.height);
    cylinderSegmentsSpinBox_->setValue(static_cast<int>(spec.segments));

    renderer::scene_contract::Vec3f center {};
    if (const auto* centerNode = findPointNode(nodes, spec.center); centerNode != nullptr) {
        center = centerNode->point.position;
    }
    renderer::scene_contract::Vec3f radiusPoint {center.x + spec.radius, center.y, center.z};
    if (const auto* radiusNode = findPointNode(nodes, spec.radiusPoint); radiusNode != nullptr) {
        radiusPoint = radiusNode->point.position;
    }
    renderer::scene_contract::Vec3f axisStart {
        center.x,
        center.y - spec.height * 0.5F,
        center.z
    };
    renderer::scene_contract::Vec3f axisEnd {
        center.x,
        center.y + spec.height * 0.5F,
        center.z
    };
    if (const auto* startNode = findPointNode(nodes, spec.axisStart); startNode != nullptr) {
        axisStart = startNode->point.position;
    }
    if (const auto* endNode = findPointNode(nodes, spec.axisEnd); endNode != nullptr) {
        axisEnd = endNode->point.position;
    }
    if (spec.constructionMode == renderer::parametric_model::CylinderSpec::ConstructionMode::axis_endpoints_radius) {
        center = {
            (axisStart.x + axisEnd.x) * 0.5F,
            (axisStart.y + axisEnd.y) * 0.5F,
            (axisStart.z + axisEnd.z) * 0.5F
        };
        const float dx = axisEnd.x - axisStart.x;
        const float dy = axisEnd.y - axisStart.y;
        const float dz = axisEnd.z - axisStart.z;
        cylinderHeightSpinBox_->setValue(std::sqrt(dx * dx + dy * dy + dz * dz));
    }

    cylinderCenterXSpinBox_->setValue(center.x);
    cylinderCenterYSpinBox_->setValue(center.y);
    cylinderCenterZSpinBox_->setValue(center.z);
    cylinderRadiusPointXSpinBox_->setValue(radiusPoint.x);
    cylinderRadiusPointYSpinBox_->setValue(radiusPoint.y);
    cylinderRadiusPointZSpinBox_->setValue(radiusPoint.z);
    cylinderAxisStartXSpinBox_->setValue(axisStart.x);
    cylinderAxisStartYSpinBox_->setValue(axisStart.y);
    cylinderAxisStartZSpinBox_->setValue(axisStart.z);
    cylinderAxisEndXSpinBox_->setValue(axisEnd.x);
    cylinderAxisEndYSpinBox_->setValue(axisEnd.y);
    cylinderAxisEndZSpinBox_->setValue(axisEnd.z);

    const bool usesRadiusPoint =
        spec.constructionMode == renderer::parametric_model::CylinderSpec::ConstructionMode::center_radius_point_height;
    const bool usesAxisEndpoints =
        spec.constructionMode == renderer::parametric_model::CylinderSpec::ConstructionMode::axis_endpoints_radius;
    cylinderRadiusSpinBox_->setEnabled(!usesRadiusPoint);
    cylinderHeightSpinBox_->setEnabled(!usesAxisEndpoints);
    cylinderCenterXSpinBox_->setEnabled(!usesAxisEndpoints);
    cylinderCenterYSpinBox_->setEnabled(!usesAxisEndpoints);
    cylinderCenterZSpinBox_->setEnabled(!usesAxisEndpoints);
    cylinderRadiusPointXSpinBox_->setEnabled(usesRadiusPoint);
    cylinderRadiusPointYSpinBox_->setEnabled(usesRadiusPoint);
    cylinderRadiusPointZSpinBox_->setEnabled(usesRadiusPoint);
    cylinderAxisStartXSpinBox_->setEnabled(usesAxisEndpoints);
    cylinderAxisStartYSpinBox_->setEnabled(usesAxisEndpoints);
    cylinderAxisStartZSpinBox_->setEnabled(usesAxisEndpoints);
    cylinderAxisEndXSpinBox_->setEnabled(usesAxisEndpoints);
    cylinderAxisEndYSpinBox_->setEnabled(usesAxisEndpoints);
    cylinderAxisEndZSpinBox_->setEnabled(usesAxisEndpoints);
}

void SceneObjectControlWidget::setSphereSpec(
    const renderer::parametric_model::SphereSpec& spec,
    const std::vector<renderer::parametric_model::ParametricNodeDescriptor>& nodes)
{
    if (sphereRadiusSpinBox_ == nullptr
        || sphereSlicesSpinBox_ == nullptr
        || sphereStacksSpinBox_ == nullptr
        || sphereConstructionModeComboBox_ == nullptr
        || sphereCenterXSpinBox_ == nullptr
        || sphereCenterYSpinBox_ == nullptr
        || sphereCenterZSpinBox_ == nullptr
        || sphereSurfacePointXSpinBox_ == nullptr
        || sphereSurfacePointYSpinBox_ == nullptr
        || sphereSurfacePointZSpinBox_ == nullptr
        || sphereDiameterStartXSpinBox_ == nullptr
        || sphereDiameterStartYSpinBox_ == nullptr
        || sphereDiameterStartZSpinBox_ == nullptr
        || sphereDiameterEndXSpinBox_ == nullptr
        || sphereDiameterEndYSpinBox_ == nullptr
        || sphereDiameterEndZSpinBox_ == nullptr) {
        return;
    }

    const QSignalBlocker radiusBlocker(sphereRadiusSpinBox_);
    const QSignalBlocker slicesBlocker(sphereSlicesSpinBox_);
    const QSignalBlocker stacksBlocker(sphereStacksSpinBox_);
    const QSignalBlocker modeBlocker(sphereConstructionModeComboBox_);
    const QSignalBlocker centerXBlocker(sphereCenterXSpinBox_);
    const QSignalBlocker centerYBlocker(sphereCenterYSpinBox_);
    const QSignalBlocker centerZBlocker(sphereCenterZSpinBox_);
    const QSignalBlocker surfacePointXBlocker(sphereSurfacePointXSpinBox_);
    const QSignalBlocker surfacePointYBlocker(sphereSurfacePointYSpinBox_);
    const QSignalBlocker surfacePointZBlocker(sphereSurfacePointZSpinBox_);
    const QSignalBlocker diameterStartXBlocker(sphereDiameterStartXSpinBox_);
    const QSignalBlocker diameterStartYBlocker(sphereDiameterStartYSpinBox_);
    const QSignalBlocker diameterStartZBlocker(sphereDiameterStartZSpinBox_);
    const QSignalBlocker diameterEndXBlocker(sphereDiameterEndXSpinBox_);
    const QSignalBlocker diameterEndYBlocker(sphereDiameterEndYSpinBox_);
    const QSignalBlocker diameterEndZBlocker(sphereDiameterEndZSpinBox_);

    sphereRadiusSpinBox_->setValue(spec.radius);
    sphereSlicesSpinBox_->setValue(static_cast<int>(spec.slices));
    sphereStacksSpinBox_->setValue(static_cast<int>(spec.stacks));
    const int modeIndex = sphereConstructionModeComboBox_->findData(static_cast<int>(spec.constructionMode));
    if (modeIndex >= 0) {
        sphereConstructionModeComboBox_->setCurrentIndex(modeIndex);
    }

    renderer::scene_contract::Vec3f center {};
    if (const auto* centerNode = findPointNode(nodes, spec.center); centerNode != nullptr) {
        center = centerNode->point.position;
    }
    renderer::scene_contract::Vec3f surfacePoint {center.x + spec.radius, center.y, center.z};
    if (const auto* surfaceNode = findPointNode(nodes, spec.surfacePoint); surfaceNode != nullptr) {
        surfacePoint = surfaceNode->point.position;
    }
    renderer::scene_contract::Vec3f diameterStart {center.x - spec.radius, center.y, center.z};
    renderer::scene_contract::Vec3f diameterEnd {center.x + spec.radius, center.y, center.z};
    if (const auto* startNode = findPointNode(nodes, spec.diameterStart); startNode != nullptr) {
        diameterStart = startNode->point.position;
    }
    if (const auto* endNode = findPointNode(nodes, spec.diameterEnd); endNode != nullptr) {
        diameterEnd = endNode->point.position;
    }
    if (spec.constructionMode == renderer::parametric_model::SphereSpec::ConstructionMode::diameter_points) {
        center = {
            (diameterStart.x + diameterEnd.x) * 0.5F,
            (diameterStart.y + diameterEnd.y) * 0.5F,
            (diameterStart.z + diameterEnd.z) * 0.5F
        };
        surfacePoint = diameterEnd;
    }

    sphereCenterXSpinBox_->setValue(center.x);
    sphereCenterYSpinBox_->setValue(center.y);
    sphereCenterZSpinBox_->setValue(center.z);
    sphereSurfacePointXSpinBox_->setValue(surfacePoint.x);
    sphereSurfacePointYSpinBox_->setValue(surfacePoint.y);
    sphereSurfacePointZSpinBox_->setValue(surfacePoint.z);
    sphereDiameterStartXSpinBox_->setValue(diameterStart.x);
    sphereDiameterStartYSpinBox_->setValue(diameterStart.y);
    sphereDiameterStartZSpinBox_->setValue(diameterStart.z);
    sphereDiameterEndXSpinBox_->setValue(diameterEnd.x);
    sphereDiameterEndYSpinBox_->setValue(diameterEnd.y);
    sphereDiameterEndZSpinBox_->setValue(diameterEnd.z);

    const bool usesSurfacePoint =
        spec.constructionMode == renderer::parametric_model::SphereSpec::ConstructionMode::center_surface_point;
    const bool usesDiameterPoints =
        spec.constructionMode == renderer::parametric_model::SphereSpec::ConstructionMode::diameter_points;
    sphereRadiusSpinBox_->setEnabled(!usesSurfacePoint && !usesDiameterPoints);
    sphereCenterXSpinBox_->setEnabled(!usesDiameterPoints);
    sphereCenterYSpinBox_->setEnabled(!usesDiameterPoints);
    sphereCenterZSpinBox_->setEnabled(!usesDiameterPoints);
    sphereSurfacePointXSpinBox_->setEnabled(usesSurfacePoint);
    sphereSurfacePointYSpinBox_->setEnabled(usesSurfacePoint);
    sphereSurfacePointZSpinBox_->setEnabled(usesSurfacePoint);
    sphereDiameterStartXSpinBox_->setEnabled(usesDiameterPoints);
    sphereDiameterStartYSpinBox_->setEnabled(usesDiameterPoints);
    sphereDiameterStartZSpinBox_->setEnabled(usesDiameterPoints);
    sphereDiameterEndXSpinBox_->setEnabled(usesDiameterPoints);
    sphereDiameterEndYSpinBox_->setEnabled(usesDiameterPoints);
    sphereDiameterEndZSpinBox_->setEnabled(usesDiameterPoints);
}
