#include "camera.h"

#include "renderer/scene_contract/math_utils.h"

void Camera::setViewportSize(int width, int height) {
    if (width <= 0 || height <= 0) {
        aspectRatio_ = 1.0F;
        return;
    }
    aspectRatio_ = static_cast<float>(width) / static_cast<float>(height);
}

void Camera::setPosition(const renderer::scene_contract::Vec3f& position) {
    position_ = position;
}

void Camera::setTarget(const renderer::scene_contract::Vec3f& target) {
    target_ = target;
}

void Camera::setUp(const renderer::scene_contract::Vec3f& up) {
    up_ = up;
}

void Camera::setVerticalFovDegrees(float degrees) {
    if (degrees < 20.0F) {
        degrees = 20.0F;
    } else if (degrees > 90.0F) {
        degrees = 90.0F;
    }
    verticalFovDegrees_ = degrees;
}

void Camera::setNearPlane(float value) {
    if (value <= 0.0F) {
        value = 0.01F;
    }
    nearPlane_ = value;
    if (farPlane_ <= nearPlane_) {
        farPlane_ = nearPlane_ + 0.01F;
    }
}

void Camera::setFarPlane(float value) {
    if (value <= nearPlane_) {
        value = nearPlane_ + 0.01F;
    }
    farPlane_ = value;
}

renderer::scene_contract::CameraData Camera::buildCameraData() const {
    renderer::scene_contract::CameraData camera;
    camera.position = position_;
    camera.view = renderer::scene_contract::math::makeLookAt(position_, target_, up_);
    camera.projection = renderer::scene_contract::math::makePerspective(
        verticalFovDegrees_ * renderer::scene_contract::math::kPi / 180.0F,
        aspectRatio_,
        nearPlane_,
        farPlane_);
    return camera;
}
