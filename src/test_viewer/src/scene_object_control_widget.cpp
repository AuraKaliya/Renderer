#include "scene_object_control_widget.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSignalBlocker>
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

    formLayout->addRow("Visible", visibleCheckBox_);
    formLayout->addRow("Rotation", speedSpinBox_);
    formLayout->addRow("Red", redSpinBox_);
    formLayout->addRow("Green", greenSpinBox_);
    formLayout->addRow("Blue", blueSpinBox_);

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
