#pragma once

#include <array>
#include <memory>

#include <QElapsedTimer>
#include <QMainWindow>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLWindow>
#include <QTimer>

#include "renderer/render_core/frame_assembler.h"
#include "renderer/render_core/scene_repository.h"
#include "renderer/scene_contract/types.h"
#include "renderer/render_gl/gl_renderer.h"

class ViewerWindow final : public QMainWindow {
public:
    ViewerWindow();

private:
    class Viewport final : public QOpenGLWindow, protected QOpenGLExtraFunctions {
    public:
        static constexpr int kSceneObjectCount = 3;

        Viewport();
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

        void setVerticalFovDegrees(float degrees);
        [[nodiscard]] float verticalFovDegrees() const;

    protected:
        void initializeGL() override;
        void resizeGL(int width, int height) override;
        void paintGL() override;

    private:
        void rebuildFramePacket();

        struct SceneObject {
            renderer::render_core::SceneRepository::ItemId itemId = 0;
            renderer::scene_contract::MeshHandle meshHandle = renderer::scene_contract::kInvalidMeshHandle;
            renderer::scene_contract::MaterialHandle materialHandle = renderer::scene_contract::kInvalidMaterialHandle;
            renderer::scene_contract::MeshData meshData;
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
        float cameraDistance_ = 6.8F;
        float verticalFovDegrees_ = 50.0F;
        renderer::render_core::SceneRepository repository_;
        renderer::render_core::FrameAssembler assembler_;
        renderer::render_core::FramePacket framePacket_;
        renderer::render_gl::GlRenderer renderer_;
        std::unique_ptr<QOpenGLFramebufferObject> offscreenTarget_;
        QTimer frameTimer_;
        QElapsedTimer animationClock_;
    };

    QWidget* buildControlPanel();

    Viewport* viewport_ = nullptr;
};
