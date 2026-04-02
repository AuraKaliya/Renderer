#pragma once

#include "renderer/scene_contract/types.h"

class OrbitCameraController final {
public:
    enum class ProjectionMode {
        perspective,
        orthographic
    };

    enum class ZoomMode {
        dolly,
        lens
    };

    void setViewportSize(int width, int height);

    void setOrbitCenter(const renderer::scene_contract::Vec3f& center);
    [[nodiscard]] renderer::scene_contract::Vec3f orbitCenter() const;

    void setDistance(float distance);
    [[nodiscard]] float distance() const;

    void setVerticalFovDegrees(float degrees);
    [[nodiscard]] float verticalFovDegrees() const;
    void setOrthographicHeight(float height);
    [[nodiscard]] float orthographicHeight() const;

    void setProjectionMode(ProjectionMode mode);
    [[nodiscard]] ProjectionMode projectionMode() const;
    void setZoomMode(ZoomMode mode);
    [[nodiscard]] ZoomMode zoomMode() const;

    void setProjection(float verticalFovDegrees, float nearPlane, float farPlane);

    void setYawRadians(float radians);
    [[nodiscard]] float yawRadians() const;

    void setPitchRadians(float radians);
    [[nodiscard]] float pitchRadians() const;

    void rotate(float deltaYawRadians, float deltaPitchRadians);
    void zoom(float deltaDistance);
    void pan(float deltaRight, float deltaUp);
    void focusOnPoint(const renderer::scene_contract::Vec3f& point);
    void focusOnBounds(const renderer::scene_contract::Aabb& bounds);
    void updateClipRangeForPoint(const renderer::scene_contract::Vec3f& point);
    void updateClipRangeForBounds(const renderer::scene_contract::Aabb& bounds);

    void setNearPlane(float value);
    [[nodiscard]] float nearPlane() const;
    void setFarPlane(float value);
    [[nodiscard]] float farPlane() const;

    [[nodiscard]] renderer::scene_contract::CameraData buildCameraData() const;

private:
    [[nodiscard]] renderer::scene_contract::Vec3f position() const;

    int viewportWidth_ = 1;
    int viewportHeight_ = 1;
    float distance_ = 6.8F;
    float verticalFovDegrees_ = 50.0F;
    float orthographicHeight_ = 8.0F;
    float yawRadians_ = 0.0F;
    float pitchRadians_ = 0.0F;
    float nearPlane_ = 0.1F;
    float farPlane_ = 100.0F;
    ProjectionMode projectionMode_ = ProjectionMode::perspective;
    ZoomMode zoomMode_ = ZoomMode::dolly;
    renderer::scene_contract::Vec3f orbitCenter_ {0.0F, 0.2F, 0.0F};
};
