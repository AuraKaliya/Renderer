#include "viewer_window.h"

#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QDebug>
#include <QWidget>

#include "renderer/scene_contract/math_utils.h"
#include "renderer/parametric_model/primitive_factory.h"

namespace {

renderer::render_gl::ProcAddress resolveGlProc(const char* name, void* userData) {
    (void)userData;
    auto* context = QOpenGLContext::currentContext();
    if (context == nullptr) {
        return nullptr;
    }
    return context->getProcAddress(name);
}

renderer::scene_contract::TransformData makeSceneTransform(
    float angleRadians,
    float offsetX,
    float offsetY,
    float offsetZ,
    float scaleValue,
    float speedMultiplier)
{
    const float animatedAngle = angleRadians * speedMultiplier;
    const auto translation = renderer::scene_contract::math::makeTranslation(offsetX, offsetY, offsetZ);
    const auto rotationY = renderer::scene_contract::math::makeRotationY(animatedAngle);
    const auto rotationX = renderer::scene_contract::math::makeRotationX(animatedAngle * 0.35F);
    const auto scale = renderer::scene_contract::math::makeScale(scaleValue, scaleValue, scaleValue);

    renderer::scene_contract::TransformData transform;
    transform.world = renderer::scene_contract::math::multiply(
        translation,
        renderer::scene_contract::math::multiply(rotationY, renderer::scene_contract::math::multiply(rotationX, scale)));
    return transform;
}

renderer::scene_contract::CameraData makeCamera(int viewportWidth, int viewportHeight) {
    renderer::scene_contract::CameraData camera;
    const float aspectRatio = viewportHeight > 0
        ? static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)
        : 1.0F;

    camera.position = {0.0F, 1.1F, 6.8F};
    camera.view = renderer::scene_contract::math::makeLookAt(
        camera.position,
        {0.0F, 0.2F, 0.0F},
        {0.0F, 1.0F, 0.0F});
    camera.projection = renderer::scene_contract::math::makePerspective(
        50.0F * renderer::scene_contract::math::kPi / 180.0F,
        aspectRatio,
        0.1F,
        100.0F);
    return camera;
}

renderer::scene_contract::RenderableItem makeItem() {
    renderer::scene_contract::RenderableItem item;
    item.transform = makeSceneTransform(0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F);
    return item;
}

QSurfaceFormat makeFormat() {
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    return format;
}

}  // namespace

ViewerWindow::ViewerWindow() {
    auto* viewport = new Viewport();
    setCentralWidget(QWidget::createWindowContainer(viewport, this));
    resize(1280, 720);
    setWindowTitle("Renderer Test Viewer");
}

ViewerWindow::Viewport::Viewport() {
    setFormat(makeFormat());

    sceneObjects_[0].meshData = renderer::parametric_model::PrimitiveFactory::makeBox(1.0F, 1.0F, 1.0F);
    sceneObjects_[0].materialData.baseColor = {0.15F, 0.55F, 0.85F, 1.0F};

    sceneObjects_[1].meshData = renderer::parametric_model::PrimitiveFactory::makeCylinder(0.55F, 1.35F, 32U);
    sceneObjects_[1].materialData.baseColor = {0.92F, 0.54F, 0.21F, 1.0F};

    sceneObjects_[2].meshData = renderer::parametric_model::PrimitiveFactory::makeSphere(0.7F, 28U, 18U);
    sceneObjects_[2].materialData.baseColor = {0.32F, 0.82F, 0.56F, 1.0F};

    animationClock_.start();

    frameTimer_.setInterval(16);
    connect(&frameTimer_, &QTimer::timeout, this, [this]() {
        update();
    });
    frameTimer_.start();
}

ViewerWindow::Viewport::~Viewport() {
    if (context() == nullptr) {
        return;
    }

    makeCurrent();
    for (auto& sceneObject : sceneObjects_) {
        if (sceneObject.meshHandle != renderer::scene_contract::kInvalidMeshHandle) {
            renderer_.releaseMesh(sceneObject.meshHandle);
            sceneObject.meshHandle = renderer::scene_contract::kInvalidMeshHandle;
        }
        if (sceneObject.materialHandle != renderer::scene_contract::kInvalidMaterialHandle) {
            renderer_.releaseMaterial(sceneObject.materialHandle);
            sceneObject.materialHandle = renderer::scene_contract::kInvalidMaterialHandle;
        }
    }
    renderer_.shutdown();
    doneCurrent();
}

void ViewerWindow::Viewport::initializeGL() {
    initializeOpenGLFunctions();
    if (!renderer_.initialize(&resolveGlProc, nullptr)) {
        qWarning() << "Failed to initialize render_gl:" << QString::fromStdString(renderer_.lastError());
    } else {
        for (auto& sceneObject : sceneObjects_) {
            sceneObject.meshHandle = renderer_.uploadMesh(sceneObject.meshData);
            if (sceneObject.meshHandle == renderer::scene_contract::kInvalidMeshHandle) {
                qWarning() << "Failed to upload scene mesh:" << QString::fromStdString(renderer_.lastError());
            }

            sceneObject.materialHandle = renderer_.uploadMaterial(sceneObject.materialData);
            if (sceneObject.materialHandle == renderer::scene_contract::kInvalidMaterialHandle) {
                qWarning() << "Failed to upload scene material:" << QString::fromStdString(renderer_.lastError());
            }
        }
    }

    for (auto& sceneObject : sceneObjects_) {
        auto item = makeItem();
        item.meshHandle = sceneObject.meshHandle;
        item.materialHandle = sceneObject.materialHandle;
        sceneObject.itemId = repository_.add(item);
    }
    rebuildFramePacket();
}

void ViewerWindow::Viewport::resizeGL(int width, int height) {
    if (width <= 0 || height <= 0) {
        offscreenTarget_.reset();
        return;
    }

    offscreenTarget_ = std::make_unique<QOpenGLFramebufferObject>(
        width,
        height,
        QOpenGLFramebufferObject::CombinedDepthStencil);
    rebuildFramePacket();
}

void ViewerWindow::Viewport::paintGL() {
    if (!offscreenTarget_) {
        return;
    }

    rebuildFramePacket();

    offscreenTarget_->bind();
    if (renderer_.isInitialized()) {
        renderer_.render(framePacket_);
    } else {
        glViewport(0, 0, offscreenTarget_->width(), offscreenTarget_->height());
        glClearColor(0.45F, 0.08F, 0.08F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    offscreenTarget_->release();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, offscreenTarget_->handle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebufferObject());
    glBlitFramebuffer(
        0, 0, offscreenTarget_->width(), offscreenTarget_->height(),
        0, 0, width(), height(),
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
}

void ViewerWindow::Viewport::rebuildFramePacket() {
    const float elapsedSeconds = static_cast<float>(animationClock_.elapsed()) / 1000.0F;
    repository_.updateTransform(
        sceneObjects_[0].itemId,
        makeSceneTransform(elapsedSeconds, -2.0F, 0.0F, 0.0F, 0.95F, 1.0F));
    repository_.updateTransform(
        sceneObjects_[1].itemId,
        makeSceneTransform(elapsedSeconds, 0.0F, 0.1F, 0.0F, 1.0F, -0.8F));
    repository_.updateTransform(
        sceneObjects_[2].itemId,
        makeSceneTransform(elapsedSeconds, 2.0F, 0.05F, 0.0F, 0.9F, 1.25F));
    framePacket_ = assembler_.build(repository_.snapshot(makeCamera(width(), height())), {width(), height()});
}
