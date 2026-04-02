#include "orbit_camera_controller.h"

#include <cmath>

#include "camera.h"
#include "camera_clip_utils.h"
#include "camera_focus_utils.h"

namespace {

constexpr float kMinPitchRadians = -1.45F;
constexpr float kMaxPitchRadians = 1.45F;
constexpr float kPerspectiveLensZoomScale = 0.12F;
constexpr float kMinPerspectiveLensZoomStep = 1.0F;
constexpr float kOrthographicZoomScale = 0.15F;
constexpr float kMinOrthographicZoomStep = 0.1F;

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

renderer::scene_contract::Vec3f aabbCenter(const renderer::scene_contract::Aabb& bounds) {
    return {
        (bounds.min.x + bounds.max.x) * 0.5F,
        (bounds.min.y + bounds.max.y) * 0.5F,
        (bounds.min.z + bounds.max.z) * 0.5F
    };
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

void OrbitCameraController::setOrthographicHeight(float height) {
    if (height < 0.1F) {
        height = 0.1F;
    }
    orthographicHeight_ = height;
}

float OrbitCameraController::orthographicHeight() const {
    return orthographicHeight_;
}

void OrbitCameraController::setProjectionMode(ProjectionMode mode) {
    projectionMode_ = mode;
    zoomMode_ = (projectionMode_ == ProjectionMode::orthographic)
        ? ZoomMode::lens
        : ZoomMode::dolly;
}

OrbitCameraController::ProjectionMode OrbitCameraController::projectionMode() const {
    return projectionMode_;
}

void OrbitCameraController::setZoomMode(ZoomMode mode) {
    zoomMode_ = mode;
}

OrbitCameraController::ZoomMode OrbitCameraController::zoomMode() const {
    return zoomMode_;
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
    if (projectionMode_ == ProjectionMode::perspective && zoomMode_ == ZoomMode::lens) {
        const float zoomStep = std::max(kMinPerspectiveLensZoomStep, verticalFovDegrees_ * kPerspectiveLensZoomScale);
        setVerticalFovDegrees(verticalFovDegrees_ + deltaDistance * zoomStep);
        return;
    }

    if (projectionMode_ == ProjectionMode::orthographic && zoomMode_ == ZoomMode::lens) {
        const float zoomStep = std::max(kMinOrthographicZoomStep, orthographicHeight_ * kOrthographicZoomScale);
        setOrthographicHeight(orthographicHeight_ + deltaDistance * zoomStep);
        return;
    }

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

void OrbitCameraController::focusOnPoint(const renderer::scene_contract::Vec3f& point) {
    const auto focusSettings = camera_focus::makeFocusSettingsForPoint(
        point,
        projectionMode_ == ProjectionMode::orthographic
            ? camera_focus::ProjectionMode::orthographic
            : camera_focus::ProjectionMode::perspective,
        distance_,
        orthographicHeight_);
    setOrbitCenter(focusSettings.orbitCenter);
    setDistance(focusSettings.distance);
    setOrthographicHeight(focusSettings.orthographicHeight);
    updateClipRangeForPoint(point);
}

void OrbitCameraController::focusOnBounds(const renderer::scene_contract::Aabb& bounds) {
    if (!bounds.valid) {
        return;
    }

    const auto focusSettings = camera_focus::makeFocusSettingsForBounds(
        bounds,
        projectionMode_ == ProjectionMode::orthographic
            ? camera_focus::ProjectionMode::orthographic
            : camera_focus::ProjectionMode::perspective,
        viewportWidth_,
        viewportHeight_,
        verticalFovDegrees_,
        distance_,
        orthographicHeight_);
    setOrbitCenter(focusSettings.orbitCenter);
    setDistance(focusSettings.distance);
    setOrthographicHeight(focusSettings.orthographicHeight);
    updateClipRangeForBounds(bounds);
}

void OrbitCameraController::updateClipRangeForPoint(const renderer::scene_contract::Vec3f& point) {
    const auto distanceToPoint = length(subtract(position(), point));
    const auto clipRange = camera_clip::makeClipRangeForPoint(distanceToPoint);
    setNearPlane(clipRange.nearPlane);
    setFarPlane(clipRange.farPlane);
}

void OrbitCameraController::updateClipRangeForBounds(const renderer::scene_contract::Aabb& bounds) {
    if (!bounds.valid) {
        return;
    }

    const auto distanceToCenter = length(subtract(position(), aabbCenter(bounds)));
    const auto clipRange = camera_clip::makeClipRangeForBounds(bounds, distanceToCenter);
    setNearPlane(clipRange.nearPlane);
    setFarPlane(clipRange.farPlane);
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
    camera.setProjectionMode(
        projectionMode_ == ProjectionMode::orthographic
            ? Camera::ProjectionMode::orthographic
            : Camera::ProjectionMode::perspective);
    camera.setNearPlane(nearPlane_);
    camera.setFarPlane(farPlane_);
    if (projectionMode_ == ProjectionMode::perspective) {
        camera.setVerticalFovDegrees(verticalFovDegrees_);
    } else {
        camera.setOrthographicHeight(orthographicHeight_);
    }
    return camera.buildCameraData();
}
