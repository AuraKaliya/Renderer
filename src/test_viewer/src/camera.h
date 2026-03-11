#pragma once

#include "renderer/scene_contract/types.h"

class Camera final {
public:
    void setViewportSize(int width, int height);

    void setPosition(const renderer::scene_contract::Vec3f& position);
    void setTarget(const renderer::scene_contract::Vec3f& target);
    void setUp(const renderer::scene_contract::Vec3f& up);

    void setVerticalFovDegrees(float degrees);
    void setNearPlane(float value);
    void setFarPlane(float value);

    [[nodiscard]] renderer::scene_contract::CameraData buildCameraData() const;

private:
    float aspectRatio_ = 1.0F;
    float verticalFovDegrees_ = 50.0F;
    float nearPlane_ = 0.1F;
    float farPlane_ = 100.0F;
    renderer::scene_contract::Vec3f position_ {0.0F, 1.1F, 6.8F};
    renderer::scene_contract::Vec3f target_ {0.0F, 0.2F, 0.0F};
    renderer::scene_contract::Vec3f up_ {0.0F, 1.0F, 0.0F};
};
