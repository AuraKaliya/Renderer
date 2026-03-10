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
        Viewport();
        ~Viewport() override;

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
        };

        std::array<SceneObject, 3> sceneObjects_ {};
        renderer::render_core::SceneRepository repository_;
        renderer::render_core::FrameAssembler assembler_;
        renderer::render_core::FramePacket framePacket_;
        renderer::render_gl::GlRenderer renderer_;
        std::unique_ptr<QOpenGLFramebufferObject> offscreenTarget_;
        QTimer frameTimer_;
        QElapsedTimer animationClock_;
    };
};
