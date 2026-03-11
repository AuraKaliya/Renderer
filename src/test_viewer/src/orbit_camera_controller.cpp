#include "orbit_camera_controller.h"

#include <cmath>

#include "camera.h"

namespace {

constexpr float kMinPitchRadians = -1.45F;
constexpr float kMaxPitchRadians = 1.45F;

renderer::scene_contract::Vec3f add(
    const renderer::scene_contract::Vec3f& left,
    const renderer::scene_contract::Vec3f& right)
{
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

renderer::scene_contract::Vec3f subtract(
    const renderer::scene_contract::Vec3f& left,
    const renderer::scene_contract::Vec3f& right)
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

renderer::scene_contract::Vec3f scale(const renderer::scene_contract::Vec3f& value, float factor) {
    return {value.x * factor, value.y * factor, value.z * factor};
}

float length(const renderer::scene_contract::Vec3f& value) {
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

renderer::scene_contract::Vec3f normalize(const renderer::scene_contract::Vec3f& value) {
    const float valueLength = length(value);
    if (valueLength <= 0.0001F) {
        return {0.0F, 0.0F, 0.0F};
    }
    return scale(value, 1.0F / valueLength);
}

renderer::scene_contract::Vec3f cross(
    const renderer::scene_contract::Vec3f& left,
    const renderer::scene_contract::Vec3f& right)
{
    return {
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

float normalizeAngleRadians(float radians) {
    return std::remainder(radians, 2.0F * 3.14159265358979323846F);
}

float clampPitchRadians(float radians) {
    if (radians < kMinPitchRadians) {
        return kMinPitchRadians;
    }
    if (radians > kMaxPitchRadians) {
        return kMaxPitchRadians;
    }
    return radians;
}

}  // namespace

void OrbitCameraController::setViewportSize(int width, int height) {
    viewportWidth_ = width > 0 ? width : 1;
    viewportHeight_ = height > 0 ? height : 1;
}

void OrbitCameraController::setOrbitCenter(const renderer::scene_contract::Vec3f& center) {
    orbitCenter_ = center;
}

renderer::scene_contract::Vec3f OrbitCameraController::orbitCenter() const {
    return orbitCenter_;
}

void OrbitCameraController::setDistance(float distance) {
    if (distance < 1.5F) {
        distance = 1.5F;
    }
    distance_ = distance;
}

float OrbitCameraController::distance() const {
    return distance_;
}

void OrbitCameraController::setVerticalFovDegrees(float degrees) {
    if (degrees < 20.0F) {
        degrees = 20.0F;
    } else if (degrees > 90.0F) {
        degrees = 90.0F;
    }
    verticalFovDegrees_ = degrees;
}

float OrbitCameraController::verticalFovDegrees() const {
    return verticalFovDegrees_;
}

void OrbitCameraController::setProjection(float verticalFovDegrees, float nearPlane, float farPlane) {
    setVerticalFovDegrees(verticalFovDegrees);
    setNearPlane(nearPlane);
    setFarPlane(farPlane);
}

void OrbitCameraController::setYawRadians(float radians) {
    yawRadians_ = normalizeAngleRadians(radians);
}

float OrbitCameraController::yawRadians() const {
    return yawRadians_;
}

void OrbitCameraController::setPitchRadians(float radians) {
    pitchRadians_ = clampPitchRadians(radians);
}

float OrbitCameraController::pitchRadians() const {
    return pitchRadians_;
}

void OrbitCameraController::rotate(float deltaYawRadians, float deltaPitchRadians) {
    setYawRadians(yawRadians_ + deltaYawRadians);
    setPitchRadians(pitchRadians_ + deltaPitchRadians);
}

void OrbitCameraController::zoom(float deltaDistance) {
    setDistance(distance_ + deltaDistance);
}

void OrbitCameraController::pan(float deltaRight, float deltaUp) {
    const auto currentPosition = position();
    const auto forward = normalize(subtract(orbitCenter_, currentPosition));
    auto right = normalize(cross(forward, {0.0F, 1.0F, 0.0F}));
    if (length(right) <= 0.0001F) {
        right = {1.0F, 0.0F, 0.0F};
    }
    const auto up = normalize(cross(right, forward));

    orbitCenter_ = add(
        orbitCenter_,
        add(scale(right, deltaRight), scale(up, deltaUp)));
}

void OrbitCameraController::setNearPlane(float value) {
    if (value <= 0.0F) {
        value = 0.01F;
    }
    nearPlane_ = value;
    if (farPlane_ <= nearPlane_) {
        farPlane_ = nearPlane_ + 0.01F;
    }
}

float OrbitCameraController::nearPlane() const {
    return nearPlane_;
}

void OrbitCameraController::setFarPlane(float value) {
    if (value <= nearPlane_) {
        value = nearPlane_ + 0.01F;
    }
    farPlane_ = value;
}

float OrbitCameraController::farPlane() const {
    return farPlane_;
}

renderer::scene_contract::Vec3f OrbitCameraController::position() const {
    const float cosPitch = std::cos(pitchRadians_);
    const float sinPitch = std::sin(pitchRadians_);
    return {
        orbitCenter_.x + distance_ * cosPitch * std::sin(yawRadians_),
        orbitCenter_.y + distance_ * sinPitch,
        orbitCenter_.z + distance_ * cosPitch * std::cos(yawRadians_)
    };
}

renderer::scene_contract::CameraData OrbitCameraController::buildCameraData() const {
    Camera camera;
    camera.setViewportSize(viewportWidth_, viewportHeight_);
    camera.setTarget(orbitCenter_);
    camera.setUp({0.0F, 1.0F, 0.0F});
    camera.setPosition(position());
    camera.setVerticalFovDegrees(verticalFovDegrees_);
    camera.setNearPlane(nearPlane_);
    camera.setFarPlane(farPlane_);
    return camera.buildCameraData();
}
