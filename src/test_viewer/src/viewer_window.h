#pragma once

#include <array>
#include <functional>
#include <memory>

#include <QElapsedTimer>
#include <QMainWindow>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLWidget>
#include <QPoint>
#include <QTimer>

class QMouseEvent;
class QFocusEvent;
class QEvent;
class QWheelEvent;

#include "orbit_camera_controller.h"
#include "viewer_control_panel.h"
#include "renderer/render_core/frame_assembler.h"
#include "renderer/render_core/scene_repository.h"
#include "renderer/scene_contract/types.h"
#include "renderer/render_gl/gl_renderer.h"

class ViewerWindow final : public QMainWindow {
public:
    ViewerWindow();

private:
    class Viewport final : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    public:
        static constexpr int kSceneObjectCount = 3;

        explicit Viewport(std::function<void()> cameraStateChangedCallback = {});
        ~Viewport() override;

        void resetDefaults();
        void applySphereFocusPreset();

        void setObjectVisible(int index, bool visible);
        [[nodiscard]] bool objectVisible(int index) const;

        void setObjectRotationSpeed(int index, float speed);
        [[nodiscard]] float objectRotationSpeed(int index) const;

        void setObjectColor(int index, const renderer::scene_contract::ColorRgba& color);
        [[nodiscard]] renderer::scene_contract::ColorRgba objectColor(int index) const;

        void setAmbientStrength(float strength);
        [[nodiscard]] float ambientStrength() const;

        void setLightDirection(const renderer::scene_contract::Vec3f& direction);
        [[nodiscard]] renderer::scene_contract::Vec3f lightDirection() const;

        void setCameraDistance(float distance);
        [[nodiscard]] float cameraDistance() const;

        void setProjectionMode(OrbitCameraController::ProjectionMode mode);
        [[nodiscard]] OrbitCameraController::ProjectionMode projectionMode() const;
        void setZoomMode(OrbitCameraController::ZoomMode mode);
        [[nodiscard]] OrbitCameraController::ZoomMode zoomMode() const;
        void setVerticalFovDegrees(float degrees);
        [[nodiscard]] float verticalFovDegrees() const;
        void setOrthographicHeight(float height);
        [[nodiscard]] float orthographicHeight() const;
        [[nodiscard]] float nearPlane() const;
        [[nodiscard]] float farPlane() const;

        void setOrbitCenter(const renderer::scene_contract::Vec3f& orbitCenter);
        [[nodiscard]] renderer::scene_contract::Vec3f orbitCenter() const;
        void focusOnPoint(const renderer::scene_contract::Vec3f& point);
        void focusOnScene();

        [[nodiscard]] renderer::scene_contract::Aabb objectLocalBounds(int index) const;

    protected:
        void initializeGL() override;
        void resizeGL(int width, int height) override;
        void paintGL() override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;

    private:
        [[nodiscard]] renderer::scene_contract::TransformData currentObjectTransform(int index) const;
        void notifyCameraStateChanged();
        void applyFocusBounds(const renderer::scene_contract::Aabb& bounds);
        void updateSceneTransforms();
        [[nodiscard]] renderer::scene_contract::Aabb objectFocusBounds(int index) const;
        [[nodiscard]] renderer::scene_contract::Aabb visibleFocusBounds() const;
        void rebuildFramePacket();

        struct SceneObject {
            renderer::render_core::SceneRepository::ItemId itemId = 0;
            renderer::scene_contract::MeshHandle meshHandle = renderer::scene_contract::kInvalidMeshHandle;
            renderer::scene_contract::MaterialHandle materialHandle = renderer::scene_contract::kInvalidMaterialHandle;
            renderer::scene_contract::TextureHandle textureHandle = renderer::scene_contract::kInvalidTextureHandle;
            renderer::scene_contract::MeshData meshData;
            renderer::scene_contract::TextureData textureData;
            renderer::scene_contract::MaterialData materialData;
            float offsetX = 0.0F;
            float offsetY = 0.0F;
            float offsetZ = 0.0F;
            float scale = 1.0F;
            float rotationSpeed = 1.0F;
            bool visible = true;
        };

        std::array<SceneObject, 3> sceneObjects_ {};
        renderer::scene_contract::DirectionalLightData light_ {};
        OrbitCameraController cameraController_;
        renderer::render_core::SceneRepository repository_;
        renderer::render_core::FrameAssembler assembler_;
        renderer::render_core::FramePacket framePacket_;
        renderer::render_gl::GlRenderer renderer_;
        std::unique_ptr<QOpenGLFramebufferObject> offscreenTarget_;
        std::function<void()> cameraStateChangedCallback_;
        QPoint lastMousePosition_;
        bool rotating_ = false;
        bool panning_ = false;
        QTimer frameTimer_;
        QElapsedTimer animationClock_;
    };
    void syncControlPanel();

    Viewport* viewport_ = nullptr;
    ViewerControlPanel* controlPanel_ = nullptr;
};
