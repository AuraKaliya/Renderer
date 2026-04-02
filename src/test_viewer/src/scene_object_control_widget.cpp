#include "scene_object_control_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

SceneObjectControlWidget::SceneObjectControlWidget(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* group = new QGroupBox(title, this);
    auto* formLayout = new QFormLayout(group);

    visibleCheckBox_ = new QCheckBox(group);
    connect(visibleCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit visibleChanged(checked);
    });

    speedSpinBox_ = new QDoubleSpinBox(group);
    speedSpinBox_->setRange(-5.0, 5.0);
    speedSpinBox_->setSingleStep(0.05);
    speedSpinBox_->setDecimals(2);
    connect(speedSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit rotationSpeedChanged(static_cast<float>(value));
    });

    redSpinBox_ = new QDoubleSpinBox(group);
    redSpinBox_->setRange(0.0, 1.0);
    redSpinBox_->setSingleStep(0.05);
    redSpinBox_->setDecimals(2);

    greenSpinBox_ = new QDoubleSpinBox(group);
    greenSpinBox_->setRange(0.0, 1.0);
    greenSpinBox_->setSingleStep(0.05);
    greenSpinBox_->setDecimals(2);

    blueSpinBox_ = new QDoubleSpinBox(group);
    blueSpinBox_->setRange(0.0, 1.0);
    blueSpinBox_->setSingleStep(0.05);
    blueSpinBox_->setDecimals(2);

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

    mirrorEnabledCheckBox_ = new QCheckBox(group);
    connect(mirrorEnabledCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit mirrorEnabledChanged(checked);
    });

    mirrorAxisComboBox_ = new QComboBox(group);
    mirrorAxisComboBox_->addItem("X", static_cast<int>(renderer::parametric_model::Axis::x));
    mirrorAxisComboBox_->addItem("Y", static_cast<int>(renderer::parametric_model::Axis::y));
    mirrorAxisComboBox_->addItem("Z", static_cast<int>(renderer::parametric_model::Axis::z));
    connect(mirrorAxisComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        emit mirrorAxisChanged(mirrorAxisComboBox_->itemData(index).toInt());
    });

    mirrorPlaneOffsetSpinBox_ = new QDoubleSpinBox(group);
    mirrorPlaneOffsetSpinBox_->setRange(-10.0, 10.0);
    mirrorPlaneOffsetSpinBox_->setSingleStep(0.1);
    mirrorPlaneOffsetSpinBox_->setDecimals(2);
    connect(mirrorPlaneOffsetSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit mirrorPlaneOffsetChanged(static_cast<float>(value));
    });

    linearArrayEnabledCheckBox_ = new QCheckBox(group);
    connect(linearArrayEnabledCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit linearArrayEnabledChanged(checked);
    });

    linearArrayCountSpinBox_ = new QSpinBox(group);
    linearArrayCountSpinBox_->setRange(1, 64);
    connect(linearArrayCountSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        emit linearArrayCountChanged(value);
    });

    linearArrayOffsetXSpinBox_ = new QDoubleSpinBox(group);
    linearArrayOffsetXSpinBox_->setRange(-10.0, 10.0);
    linearArrayOffsetXSpinBox_->setSingleStep(0.1);
    linearArrayOffsetXSpinBox_->setDecimals(2);

    linearArrayOffsetYSpinBox_ = new QDoubleSpinBox(group);
    linearArrayOffsetYSpinBox_->setRange(-10.0, 10.0);
    linearArrayOffsetYSpinBox_->setSingleStep(0.1);
    linearArrayOffsetYSpinBox_->setDecimals(2);

    linearArrayOffsetZSpinBox_ = new QDoubleSpinBox(group);
    linearArrayOffsetZSpinBox_->setRange(-10.0, 10.0);
    linearArrayOffsetZSpinBox_->setSingleStep(0.1);
    linearArrayOffsetZSpinBox_->setDecimals(2);

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

    formLayout->addRow("Visible", visibleCheckBox_);
    formLayout->addRow("Rotation", speedSpinBox_);
    formLayout->addRow("Red", redSpinBox_);
    formLayout->addRow("Green", greenSpinBox_);
    formLayout->addRow("Blue", blueSpinBox_);
    formLayout->addRow("Mirror Enabled", mirrorEnabledCheckBox_);
    formLayout->addRow("Mirror Axis", mirrorAxisComboBox_);
    formLayout->addRow("Mirror Offset", mirrorPlaneOffsetSpinBox_);
    formLayout->addRow("Array Enabled", linearArrayEnabledCheckBox_);
    formLayout->addRow("Array Count", linearArrayCountSpinBox_);
    formLayout->addRow("Array Offset X", linearArrayOffsetXSpinBox_);
    formLayout->addRow("Array Offset Y", linearArrayOffsetYSpinBox_);
    formLayout->addRow("Array Offset Z", linearArrayOffsetZSpinBox_);

    rootLayout->addWidget(group);
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
