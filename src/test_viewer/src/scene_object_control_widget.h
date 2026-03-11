#pragma once

#include <QWidget>

#include "renderer/scene_contract/types.h"

class QCheckBox;
class QDoubleSpinBox;

class SceneObjectControlWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SceneObjectControlWidget(const QString& title, QWidget* parent = nullptr);

    void setObjectState(bool visible, float rotationSpeed, const renderer::scene_contract::ColorRgba& color);

signals:
    void visibleChanged(bool visible);
    void rotationSpeedChanged(float speed);
    void colorChanged(float red, float green, float blue);

private:
    QCheckBox* visibleCheckBox_ = nullptr;
    QDoubleSpinBox* speedSpinBox_ = nullptr;
    QDoubleSpinBox* redSpinBox_ = nullptr;
    QDoubleSpinBox* greenSpinBox_ = nullptr;
    QDoubleSpinBox* blueSpinBox_ = nullptr;
};
