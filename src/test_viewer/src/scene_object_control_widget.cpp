#include "scene_object_control_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

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
        boxWidthSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        boxHeightSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        boxDepthSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);

        connect(boxWidthSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit boxWidthChanged(static_cast<float>(value));
        });
        connect(boxHeightSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit boxHeightChanged(static_cast<float>(value));
        });
        connect(boxDepthSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit boxDepthChanged(static_cast<float>(value));
        });

        primitiveLayout->addRow("Width", boxWidthSpinBox_);
        primitiveLayout->addRow("Height", boxHeightSpinBox_);
        primitiveLayout->addRow("Depth", boxDepthSpinBox_);
    } else if (primitiveKind_ == PrimitivePanelKind::cylinder) {
        cylinderRadiusSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        cylinderHeightSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        cylinderSegmentsSpinBox_ = new QSpinBox(groupBox_);
        cylinderSegmentsSpinBox_->setRange(3, 128);
        cylinderSegmentsSpinBox_->setSingleStep(1);

        connect(cylinderRadiusSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit cylinderRadiusChanged(static_cast<float>(value));
        });
        connect(cylinderHeightSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit cylinderHeightChanged(static_cast<float>(value));
        });
        connect(cylinderSegmentsSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            emit cylinderSegmentsChanged(value);
        });

        primitiveLayout->addRow("Radius", cylinderRadiusSpinBox_);
        primitiveLayout->addRow("Height", cylinderHeightSpinBox_);
        primitiveLayout->addRow("Segments", cylinderSegmentsSpinBox_);
    } else {
        sphereRadiusSpinBox_ = makeFloatSpinBox(groupBox_, 0.05, 10.0, 0.05, 2);
        sphereSlicesSpinBox_ = new QSpinBox(groupBox_);
        sphereSlicesSpinBox_->setRange(3, 128);
        sphereSlicesSpinBox_->setSingleStep(1);
        sphereStacksSpinBox_ = new QSpinBox(groupBox_);
        sphereStacksSpinBox_->setRange(2, 128);
        sphereStacksSpinBox_->setSingleStep(1);

        connect(sphereRadiusSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            emit sphereRadiusChanged(static_cast<float>(value));
        });
        connect(sphereSlicesSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            emit sphereSlicesChanged(value);
        });
        connect(sphereStacksSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            emit sphereStacksChanged(value);
        });

        primitiveLayout->addRow("Radius", sphereRadiusSpinBox_);
        primitiveLayout->addRow("Slices", sphereSlicesSpinBox_);
        primitiveLayout->addRow("Stacks", sphereStacksSpinBox_);
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

void SceneObjectControlWidget::setBoxSpec(const renderer::parametric_model::BoxSpec& spec) {
    if (boxWidthSpinBox_ == nullptr || boxHeightSpinBox_ == nullptr || boxDepthSpinBox_ == nullptr) {
        return;
    }

    const QSignalBlocker widthBlocker(boxWidthSpinBox_);
    const QSignalBlocker heightBlocker(boxHeightSpinBox_);
    const QSignalBlocker depthBlocker(boxDepthSpinBox_);

    boxWidthSpinBox_->setValue(spec.width);
    boxHeightSpinBox_->setValue(spec.height);
    boxDepthSpinBox_->setValue(spec.depth);
}

void SceneObjectControlWidget::setCylinderSpec(const renderer::parametric_model::CylinderSpec& spec) {
    if (cylinderRadiusSpinBox_ == nullptr || cylinderHeightSpinBox_ == nullptr || cylinderSegmentsSpinBox_ == nullptr) {
        return;
    }

    const QSignalBlocker radiusBlocker(cylinderRadiusSpinBox_);
    const QSignalBlocker heightBlocker(cylinderHeightSpinBox_);
    const QSignalBlocker segmentsBlocker(cylinderSegmentsSpinBox_);

    cylinderRadiusSpinBox_->setValue(spec.radius);
    cylinderHeightSpinBox_->setValue(spec.height);
    cylinderSegmentsSpinBox_->setValue(static_cast<int>(spec.segments));
}

void SceneObjectControlWidget::setSphereSpec(const renderer::parametric_model::SphereSpec& spec) {
    if (sphereRadiusSpinBox_ == nullptr || sphereSlicesSpinBox_ == nullptr || sphereStacksSpinBox_ == nullptr) {
        return;
    }

    const QSignalBlocker radiusBlocker(sphereRadiusSpinBox_);
    const QSignalBlocker slicesBlocker(sphereSlicesSpinBox_);
    const QSignalBlocker stacksBlocker(sphereStacksSpinBox_);

    sphereRadiusSpinBox_->setValue(spec.radius);
    sphereSlicesSpinBox_->setValue(static_cast<int>(spec.slices));
    sphereStacksSpinBox_->setValue(static_cast<int>(spec.stacks));
}
