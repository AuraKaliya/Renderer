#include "lighting_control_widget.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QVBoxLayout>

LightingControlWidget::LightingControlWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* group = new QGroupBox("Lighting", this);
    auto* formLayout = new QFormLayout(group);

    ambientSpinBox_ = new QDoubleSpinBox(group);
    ambientSpinBox_->setRange(0.0, 1.0);
    ambientSpinBox_->setSingleStep(0.02);
    ambientSpinBox_->setDecimals(2);
    connect(ambientSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit ambientStrengthChanged(static_cast<float>(value));
    });

    lightXSpinBox_ = new QDoubleSpinBox(group);
    lightXSpinBox_->setRange(-2.0, 2.0);
    lightXSpinBox_->setSingleStep(0.05);
    lightXSpinBox_->setDecimals(2);

    lightYSpinBox_ = new QDoubleSpinBox(group);
    lightYSpinBox_->setRange(-2.0, 2.0);
    lightYSpinBox_->setSingleStep(0.05);
    lightYSpinBox_->setDecimals(2);

    lightZSpinBox_ = new QDoubleSpinBox(group);
    lightZSpinBox_->setRange(-2.0, 2.0);
    lightZSpinBox_->setSingleStep(0.05);
    lightZSpinBox_->setDecimals(2);

    auto emitLight = [this]() {
        emit lightDirectionChanged(
            static_cast<float>(lightXSpinBox_->value()),
            static_cast<float>(lightYSpinBox_->value()),
            static_cast<float>(lightZSpinBox_->value()));
    };

    connect(lightXSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitLight](double) {
        emitLight();
    });
    connect(lightYSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitLight](double) {
        emitLight();
    });
    connect(lightZSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [emitLight](double) {
        emitLight();
    });

    formLayout->addRow("Ambient", ambientSpinBox_);
    formLayout->addRow("Light X", lightXSpinBox_);
    formLayout->addRow("Light Y", lightYSpinBox_);
    formLayout->addRow("Light Z", lightZSpinBox_);

    rootLayout->addWidget(group);
}

void LightingControlWidget::setLightingState(float ambientStrength, const renderer::scene_contract::Vec3f& lightDirection) {
    const QSignalBlocker ambientBlocker(ambientSpinBox_);
    const QSignalBlocker xBlocker(lightXSpinBox_);
    const QSignalBlocker yBlocker(lightYSpinBox_);
    const QSignalBlocker zBlocker(lightZSpinBox_);

    ambientSpinBox_->setValue(ambientStrength);
    lightXSpinBox_->setValue(lightDirection.x);
    lightYSpinBox_->setValue(lightDirection.y);
    lightZSpinBox_->setValue(lightDirection.z);
}
