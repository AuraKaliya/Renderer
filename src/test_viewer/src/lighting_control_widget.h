#pragma once

#include <QWidget>

#include "renderer/scene_contract/types.h"

class QDoubleSpinBox;

class LightingControlWidget final : public QWidget {
    Q_OBJECT

public:
    explicit LightingControlWidget(QWidget* parent = nullptr);

    void setLightingState(float ambientStrength, const renderer::scene_contract::Vec3f& lightDirection);

signals:
    void ambientStrengthChanged(float strength);
    void lightDirectionChanged(float x, float y, float z);

private:
    QDoubleSpinBox* ambientSpinBox_ = nullptr;
    QDoubleSpinBox* lightXSpinBox_ = nullptr;
    QDoubleSpinBox* lightYSpinBox_ = nullptr;
    QDoubleSpinBox* lightZSpinBox_ = nullptr;
};
