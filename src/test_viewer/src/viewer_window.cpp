#include "viewer_window.h"

#include <cmath>
#include <utility>

#include <QHBoxLayout>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QDebug>
#include <QWheelEvent>
#include <QWidget>

#include "renderer/scene_contract/math_utils.h"
#include "renderer/parametric_model/primitive_factory.h"

namespace {

constexpr std::size_t kSceneObjectCount = 3;

struct SceneObjectDefaults {
    renderer::scene_contract::ColorRgba color;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    float offsetZ = 0.0F;
    float scale = 1.0F;
    float rotationSpeed = 1.0F;
    bool visible = true;
};

const std::array<SceneObjectDefaults, kSceneObjectCount> kDefaultSceneObjects = {{
    {{0.15F, 0.55F, 0.85F, 1.0F}, -2.0F, 0.0F, 0.0F, 0.95F, 1.0F, true},
    {{0.92F, 0.54F, 0.21F, 1.0F}, 0.0F, 0.1F, 0.0F, 1.0F, -0.8F, true},
    {{0.32F, 0.82F, 0.56F, 1.0F}, 2.0F, 0.05F, 0.0F, 0.9F, 1.25F, true}
}};

const renderer::scene_contract::DirectionalLightData kDefaultLight = {
    {-0.4F, -1.0F, -0.35F},
    {1.0F, 0.98F, 0.92F},
    0.24F
};

const renderer::scene_contract::DirectionalLightData kSphereFocusLight = {
    {-0.15F, -0.65F, -0.95F},
    {1.0F, 0.96F, 0.90F},
    0.10F
};

constexpr float kDefaultCameraDistance = 6.8F;
constexpr float kDefaultVerticalFovDegrees = 50.0F;
constexpr float kSphereFocusVerticalFovDegrees = 38.0F;
constexpr float kDefaultNearPlane = 0.1F;
constexpr float kDefaultFarPlane = 100.0F;
constexpr float kDefaultCameraHeight = 1.1F;
constexpr float kDefaultCameraTargetY = 0.2F;
const float kDefaultCameraPitchRadians = std::atan2(
    kDefaultCameraHeight - kDefaultCameraTargetY,
    kDefaultCameraDistance);
constexpr float kOrbitRotateSensitivity = 0.0125F;
constexpr float kWheelZoomStep = 0.35F;
constexpr float kPanViewportFactor = 1.0F;

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

renderer::scene_contract::RenderableItem makeItem() {
    renderer::scene_contract::RenderableItem item;
    item.transform = makeSceneTransform(0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F);
    return item;
}

renderer::scene_contract::ColorRgba makeColorFromRgbSpin(double red, double green, double blue, float alpha = 1.0F) {
    return {
        static_cast<float>(red),
        static_cast<float>(green),
        static_cast<float>(blue),
        alpha
    };
}

renderer::scene_contract::TextureData makeCheckerTextureData(
    std::int32_t width,
    std::int32_t height,
    std::uint8_t cellSize)
{
    renderer::scene_contract::TextureData texture;
    texture.width = width;
    texture.height = height;
    texture.format = renderer::scene_contract::TextureFormat::rgba8;
    texture.generateMipmaps = true;
    texture.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 255U);

    for (std::int32_t y = 0; y < height; ++y) {
        for (std::int32_t x = 0; x < width; ++x) {
            const bool evenCell = (((x / cellSize) + (y / cellSize)) % 2) == 0;
            const std::size_t index =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4U;
            if (evenCell) {
                texture.pixels[index + 0] = 245U;
                texture.pixels[index + 1] = 238U;
                texture.pixels[index + 2] = 214U;
                texture.pixels[index + 3] = 255U;
            } else {
                texture.pixels[index + 0] = 52U;
                texture.pixels[index + 1] = 110U;
                texture.pixels[index + 2] = 168U;
                texture.pixels[index + 3] = 255U;
            }
        }
    }

    return texture;
}

QSurfaceFormat makeFormat() {
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    return format;
}

renderer::scene_contract::Aabb mergeAabb(
    const renderer::scene_contract::Aabb& left,
    const renderer::scene_contract::Aabb& right)
{
    if (!left.valid) {
        return right;
    }
    if (!right.valid) {
        return left;
    }

    renderer::scene_contract::Aabb merged;
    merged.min = {
        std::min(left.min.x, right.min.x),
        std::min(left.min.y, right.min.y),
        std::min(left.min.z, right.min.z)
    };
    merged.max = {
        std::max(left.max.x, right.max.x),
        std::max(left.max.y, right.max.y),
        std::max(left.max.z, right.max.z)
    };
    merged.valid = true;
    return merged;
}

}  // namespace

ViewerWindow::ViewerWindow() {
    viewport_ = new Viewport([this]() {
        syncControlPanel();
    });
    controlPanel_ = new ViewerControlPanel(this);

    auto* centralWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    viewport_->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(viewport_, 1);
    layout->addWidget(controlPanel_);

    connect(controlPanel_, &ViewerControlPanel::objectVisibleChanged, this, [this](int index, bool visible) {
        viewport_->setObjectVisible(index, visible);
    });
    connect(controlPanel_, &ViewerControlPanel::objectRotationSpeedChanged, this, [this](int index, float speed) {
        viewport_->setObjectRotationSpeed(index, speed);
    });
    connect(controlPanel_, &ViewerControlPanel::objectColorChanged, this, [this](int index, float red, float green, float blue) {
        viewport_->setObjectColor(index, makeColorFromRgbSpin(red, green, blue));
    });
    connect(controlPanel_, &ViewerControlPanel::ambientStrengthChanged, this, [this](float strength) {
        viewport_->setAmbientStrength(strength);
    });
    connect(controlPanel_, &ViewerControlPanel::lightDirectionChanged, this, [this](float x, float y, float z) {
        viewport_->setLightDirection({x, y, z});
    });
    connect(controlPanel_, &ViewerControlPanel::cameraDistanceChanged, this, [this](float distance) {
        viewport_->setCameraDistance(distance);
    });
    connect(controlPanel_, &ViewerControlPanel::verticalFovDegreesChanged, this, [this](float degrees) {
        viewport_->setVerticalFovDegrees(degrees);
    });
    connect(controlPanel_, &ViewerControlPanel::focusPointRequested, this, [this](float x, float y, float z) {
        viewport_->focusOnPoint({x, y, z});
    });
    connect(controlPanel_, &ViewerControlPanel::focusAllRequested, this, [this]() {
        viewport_->focusOnScene();
    });
    connect(controlPanel_, &ViewerControlPanel::resetDefaultsRequested, this, [this]() {
        viewport_->resetDefaults();
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::focusSphereRequested, this, [this]() {
        viewport_->applySphereFocusPreset();
        syncControlPanel();
    });

    setCentralWidget(centralWidget);
    resize(1280, 720);
    setWindowTitle("Renderer Test Viewer");
    syncControlPanel();
}

ViewerWindow::Viewport::Viewport(std::function<void()> cameraStateChangedCallback)
    : cameraStateChangedCallback_(std::move(cameraStateChangedCallback))
{
    setFormat(makeFormat());
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    sceneObjects_[0].meshData = renderer::parametric_model::PrimitiveFactory::makeBox(1.0F, 1.0F, 1.0F);
    sceneObjects_[0].textureData = makeCheckerTextureData(64, 64, 8U);
    sceneObjects_[0].materialData.baseColor = kDefaultSceneObjects[0].color;
    sceneObjects_[0].materialData.useBaseColorTexture = true;
    sceneObjects_[0].offsetX = kDefaultSceneObjects[0].offsetX;
    sceneObjects_[0].offsetY = kDefaultSceneObjects[0].offsetY;
    sceneObjects_[0].offsetZ = kDefaultSceneObjects[0].offsetZ;
    sceneObjects_[0].scale = kDefaultSceneObjects[0].scale;
    sceneObjects_[0].rotationSpeed = kDefaultSceneObjects[0].rotationSpeed;
    sceneObjects_[0].visible = kDefaultSceneObjects[0].visible;

    sceneObjects_[1].meshData = renderer::parametric_model::PrimitiveFactory::makeCylinder(0.55F, 1.35F, 32U);
    sceneObjects_[1].materialData.baseColor = kDefaultSceneObjects[1].color;
    sceneObjects_[1].offsetX = kDefaultSceneObjects[1].offsetX;
    sceneObjects_[1].offsetY = kDefaultSceneObjects[1].offsetY;
    sceneObjects_[1].offsetZ = kDefaultSceneObjects[1].offsetZ;
    sceneObjects_[1].scale = kDefaultSceneObjects[1].scale;
    sceneObjects_[1].rotationSpeed = kDefaultSceneObjects[1].rotationSpeed;
    sceneObjects_[1].visible = kDefaultSceneObjects[1].visible;

    sceneObjects_[2].meshData = renderer::parametric_model::PrimitiveFactory::makeSphere(0.7F, 28U, 18U);
    sceneObjects_[2].materialData.baseColor = kDefaultSceneObjects[2].color;
    sceneObjects_[2].offsetX = kDefaultSceneObjects[2].offsetX;
    sceneObjects_[2].offsetY = kDefaultSceneObjects[2].offsetY;
    sceneObjects_[2].offsetZ = kDefaultSceneObjects[2].offsetZ;
    sceneObjects_[2].scale = kDefaultSceneObjects[2].scale;
    sceneObjects_[2].rotationSpeed = kDefaultSceneObjects[2].rotationSpeed;
    sceneObjects_[2].visible = kDefaultSceneObjects[2].visible;

    light_ = kDefaultLight;
    cameraController_.setOrbitCenter({0.0F, kDefaultCameraTargetY, 0.0F});
    cameraController_.setDistance(kDefaultCameraDistance);
    cameraController_.setProjection(kDefaultVerticalFovDegrees, kDefaultNearPlane, kDefaultFarPlane);
    cameraController_.setPitchRadians(kDefaultCameraPitchRadians);
    cameraController_.setYawRadians(0.0F);

    animationClock_.start();

    frameTimer_.setInterval(16);
    connect(&frameTimer_, &QTimer::timeout, this, [this]() {
        update();
    });
    frameTimer_.start();
}

void ViewerWindow::Viewport::resetDefaults() {
    light_ = kDefaultLight;
    cameraController_.setOrbitCenter({0.0F, kDefaultCameraTargetY, 0.0F});
    cameraController_.setDistance(kDefaultCameraDistance);
    cameraController_.setProjection(kDefaultVerticalFovDegrees, kDefaultNearPlane, kDefaultFarPlane);
    cameraController_.setPitchRadians(kDefaultCameraPitchRadians);
    cameraController_.setYawRadians(0.0F);

    for (int index = 0; index < kSceneObjectCount; ++index) {
        const auto& defaults = kDefaultSceneObjects[index];
        auto& sceneObject = sceneObjects_[index];

        sceneObject.materialData.baseColor = defaults.color;
        sceneObject.offsetX = defaults.offsetX;
        sceneObject.offsetY = defaults.offsetY;
        sceneObject.offsetZ = defaults.offsetZ;
        sceneObject.scale = defaults.scale;
        sceneObject.rotationSpeed = defaults.rotationSpeed;
        sceneObject.visible = defaults.visible;

        if (renderer_.isInitialized() &&
            sceneObject.materialHandle != renderer::scene_contract::kInvalidMaterialHandle) {
            renderer_.updateMaterial(sceneObject.materialHandle, sceneObject.materialData);
        }
    }

    update();
}

void ViewerWindow::Viewport::applySphereFocusPreset() {
    light_ = kSphereFocusLight;
    cameraController_.setProjection(kSphereFocusVerticalFovDegrees, kDefaultNearPlane, kDefaultFarPlane);
    cameraController_.setPitchRadians(kDefaultCameraPitchRadians);
    cameraController_.setYawRadians(0.0F);

    for (int index = 0; index < kSceneObjectCount; ++index) {
        auto& sceneObject = sceneObjects_[index];
        const auto& defaults = kDefaultSceneObjects[index];

        sceneObject.materialData.baseColor = defaults.color;
        sceneObject.offsetX = defaults.offsetX;
        sceneObject.offsetY = defaults.offsetY;
        sceneObject.offsetZ = defaults.offsetZ;
        sceneObject.scale = defaults.scale;
        sceneObject.rotationSpeed = defaults.rotationSpeed;
        sceneObject.visible = defaults.visible;

        if (renderer_.isInitialized() &&
            sceneObject.materialHandle != renderer::scene_contract::kInvalidMaterialHandle) {
            renderer_.updateMaterial(sceneObject.materialHandle, sceneObject.materialData);
        }
    }

    sceneObjects_[0].visible = false;
    sceneObjects_[0].rotationSpeed = 0.0F;

    sceneObjects_[1].visible = false;
    sceneObjects_[1].rotationSpeed = 0.0F;

    sceneObjects_[2].visible = true;
    sceneObjects_[2].rotationSpeed = 1.75F;
    sceneObjects_[2].materialData.baseColor = {0.45F, 0.86F, 0.78F, 1.0F};
    cameraController_.setOrbitCenter({
        sceneObjects_[2].offsetX,
        sceneObjects_[2].offsetY,
        sceneObjects_[2].offsetZ
    });

    if (renderer_.isInitialized() &&
        sceneObjects_[2].materialHandle != renderer::scene_contract::kInvalidMaterialHandle) {
        renderer_.updateMaterial(sceneObjects_[2].materialHandle, sceneObjects_[2].materialData);
    }

    updateSceneTransforms();
    cameraController_.focusOnBounds(repository_.rangeData(sceneObjects_[2].itemId).worldBounds);
    if (cameraStateChangedCallback_) {
        cameraStateChangedCallback_();
    }

    update();
}

void ViewerWindow::Viewport::setObjectVisible(int index, bool visible) {
    if (index < 0 || index >= kSceneObjectCount) {
        return;
    }

    sceneObjects_[index].visible = visible;
    update();
}

bool ViewerWindow::Viewport::objectVisible(int index) const {
    if (index < 0 || index >= kSceneObjectCount) {
        return false;
    }
    return sceneObjects_[index].visible;
}

void ViewerWindow::Viewport::setObjectRotationSpeed(int index, float speed) {
    if (index < 0 || index >= kSceneObjectCount) {
        return;
    }

    sceneObjects_[index].rotationSpeed = speed;
    update();
}

float ViewerWindow::Viewport::objectRotationSpeed(int index) const {
    if (index < 0 || index >= kSceneObjectCount) {
        return 0.0F;
    }
    return sceneObjects_[index].rotationSpeed;
}

void ViewerWindow::Viewport::setObjectColor(int index, const renderer::scene_contract::ColorRgba& color) {
    if (index < 0 || index >= kSceneObjectCount) {
        return;
    }

    auto& sceneObject = sceneObjects_[index];
    sceneObject.materialData.baseColor = color;

    if (renderer_.isInitialized() &&
        sceneObject.materialHandle != renderer::scene_contract::kInvalidMaterialHandle) {
        renderer_.updateMaterial(sceneObject.materialHandle, sceneObject.materialData);
    }

    update();
}

renderer::scene_contract::ColorRgba ViewerWindow::Viewport::objectColor(int index) const {
    if (index < 0 || index >= kSceneObjectCount) {
        return {};
    }
    return sceneObjects_[index].materialData.baseColor;
}

void ViewerWindow::Viewport::setAmbientStrength(float strength) {
    light_.ambientStrength = strength;
    update();
}

float ViewerWindow::Viewport::ambientStrength() const {
    return light_.ambientStrength;
}

void ViewerWindow::Viewport::setLightDirection(const renderer::scene_contract::Vec3f& direction) {
    light_.direction = direction;
    update();
}

renderer::scene_contract::Vec3f ViewerWindow::Viewport::lightDirection() const {
    return light_.direction;
}

void ViewerWindow::Viewport::setCameraDistance(float distance) {
    cameraController_.setDistance(distance);
    update();
}

float ViewerWindow::Viewport::cameraDistance() const {
    return cameraController_.distance();
}

void ViewerWindow::Viewport::setVerticalFovDegrees(float degrees) {
    cameraController_.setVerticalFovDegrees(degrees);
    update();
}

float ViewerWindow::Viewport::verticalFovDegrees() const {
    return cameraController_.verticalFovDegrees();
}

float ViewerWindow::Viewport::nearPlane() const {
    return cameraController_.nearPlane();
}

float ViewerWindow::Viewport::farPlane() const {
    return cameraController_.farPlane();
}

void ViewerWindow::Viewport::setOrbitCenter(const renderer::scene_contract::Vec3f& orbitCenter) {
    cameraController_.setOrbitCenter(orbitCenter);
    update();
}

renderer::scene_contract::Vec3f ViewerWindow::Viewport::orbitCenter() const {
    return cameraController_.orbitCenter();
}

void ViewerWindow::Viewport::focusOnPoint(const renderer::scene_contract::Vec3f& point) {
    cameraController_.focusOnPoint(point);
    if (cameraStateChangedCallback_) {
        cameraStateChangedCallback_();
    }
    update();
}

void ViewerWindow::Viewport::focusOnScene() {
    updateSceneTransforms();
    cameraController_.focusOnBounds(visibleSceneBounds());
    if (cameraStateChangedCallback_) {
        cameraStateChangedCallback_();
    }
    update();
}

renderer::scene_contract::Aabb ViewerWindow::Viewport::objectLocalBounds(int index) const {
    if (index < 0 || index >= kSceneObjectCount) {
        return {};
    }

    return sceneObjects_[index].meshData.localBounds;
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
        if (sceneObject.textureHandle != renderer::scene_contract::kInvalidTextureHandle) {
            renderer_.releaseTexture(sceneObject.textureHandle);
            sceneObject.textureHandle = renderer::scene_contract::kInvalidTextureHandle;
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
            if (!sceneObject.textureData.pixels.empty()) {
                sceneObject.textureHandle = renderer_.uploadTexture(sceneObject.textureData);
                if (sceneObject.textureHandle == renderer::scene_contract::kInvalidTextureHandle) {
                    qWarning() << "Failed to upload scene texture:" << QString::fromStdString(renderer_.lastError());
                } else {
                    sceneObject.materialData.baseColorTexture = sceneObject.textureHandle;
                }
            }

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
        repository_.updateLocalBounds(sceneObject.itemId, sceneObject.meshData.localBounds);
    }
    cameraController_.setViewportSize(width(), height());
    rebuildFramePacket();
}

void ViewerWindow::Viewport::resizeGL(int width, int height) {
    if (width <= 0 || height <= 0) {
        offscreenTarget_.reset();
        return;
    }

    cameraController_.setViewportSize(width, height);

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

void ViewerWindow::Viewport::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        rotating_ = true;
        panning_ = false;
        lastMousePosition_ = event->pos();
        setFocus(Qt::MouseFocusReason);
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        panning_ = true;
        rotating_ = false;
        lastMousePosition_ = event->pos();
        setFocus(Qt::MouseFocusReason);
        event->accept();
        return;
    }

    QOpenGLWidget::mousePressEvent(event);
}

void ViewerWindow::Viewport::mouseMoveEvent(QMouseEvent* event) {
    if (rotating_ && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->pos() - lastMousePosition_;
        lastMousePosition_ = event->pos();

        cameraController_.rotate(
            -static_cast<float>(delta.x()) * kOrbitRotateSensitivity,
            -static_cast<float>(delta.y()) * kOrbitRotateSensitivity);

        if (cameraStateChangedCallback_) {
            cameraStateChangedCallback_();
        }
        update();
        event->accept();
        return;
    }

    if (panning_ && (event->buttons() & Qt::RightButton)) {
        const QPoint delta = event->pos() - lastMousePosition_;
        lastMousePosition_ = event->pos();

        if (height() > 0) {
            const float halfFovRadians = cameraController_.verticalFovDegrees() * 0.5F * 3.14159265358979323846F / 180.0F;
            const float worldUnitsPerPixel =
                2.0F * cameraController_.distance() * std::tan(halfFovRadians) / static_cast<float>(height());

            cameraController_.pan(
                -static_cast<float>(delta.x()) * worldUnitsPerPixel * kPanViewportFactor,
                static_cast<float>(delta.y()) * worldUnitsPerPixel * kPanViewportFactor);
        }

        if (cameraStateChangedCallback_) {
            cameraStateChangedCallback_();
        }
        update();
        event->accept();
        return;
    }

    QOpenGLWidget::mouseMoveEvent(event);
}

void ViewerWindow::Viewport::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        rotating_ = false;
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        panning_ = false;
        event->accept();
        return;
    }

    QOpenGLWidget::mouseReleaseEvent(event);
}

void ViewerWindow::Viewport::leaveEvent(QEvent* event) {
    rotating_ = false;
    panning_ = false;
    QOpenGLWidget::leaveEvent(event);
}

void ViewerWindow::Viewport::focusOutEvent(QFocusEvent* event) {
    rotating_ = false;
    panning_ = false;
    QOpenGLWidget::focusOutEvent(event);
}

void ViewerWindow::Viewport::wheelEvent(QWheelEvent* event) {
    const QPoint angleDelta = event->angleDelta();
    if (angleDelta.y() == 0) {
        QOpenGLWidget::wheelEvent(event);
        return;
    }

    cameraController_.zoom(-static_cast<float>(angleDelta.y()) / 120.0F * kWheelZoomStep);
    if (cameraStateChangedCallback_) {
        cameraStateChangedCallback_();
    }
    update();
    event->accept();
}

void ViewerWindow::Viewport::updateSceneTransforms() {
    const float elapsedSeconds = static_cast<float>(animationClock_.elapsed()) / 1000.0F;
    for (auto& sceneObject : sceneObjects_) {
        repository_.updateTransform(
            sceneObject.itemId,
            makeSceneTransform(
                elapsedSeconds,
                sceneObject.offsetX,
                sceneObject.offsetY,
                sceneObject.offsetZ,
                sceneObject.scale,
                sceneObject.rotationSpeed));
    }
}

renderer::scene_contract::Aabb ViewerWindow::Viewport::visibleSceneBounds() const {
    renderer::scene_contract::Aabb bounds;
    for (const auto& sceneObject : sceneObjects_) {
        if (!sceneObject.visible) {
            continue;
        }
        bounds = mergeAabb(bounds, repository_.rangeData(sceneObject.itemId).worldBounds);
    }
    return bounds;
}

void ViewerWindow::Viewport::rebuildFramePacket() {
    updateSceneTransforms();

    auto scene = repository_.snapshot(cameraController_.buildCameraData());
    scene.light = light_;

    std::vector<renderer::scene_contract::RenderableItem> visibleItems;
    visibleItems.reserve(sceneObjects_.size());

    for (const auto& sceneObject : sceneObjects_) {
        if (!sceneObject.visible) {
            continue;
        }
        if (sceneObject.itemId >= scene.items.size()) {
            continue;
        }
        visibleItems.push_back(scene.items[sceneObject.itemId]);
    }

    scene.items = std::move(visibleItems);
    framePacket_ = assembler_.build(scene, {width(), height()});
}

void ViewerWindow::syncControlPanel() {
    if (controlPanel_ == nullptr || viewport_ == nullptr) {
        return;
    }

    for (int index = 0; index < Viewport::kSceneObjectCount; ++index) {
        controlPanel_->setObjectState(
            index,
            viewport_->objectVisible(index),
            viewport_->objectRotationSpeed(index),
            viewport_->objectColor(index));
        controlPanel_->setObjectBounds(
            index,
            viewport_->objectLocalBounds(index));
    }

    controlPanel_->setLightingState(
        viewport_->ambientStrength(),
        viewport_->lightDirection());
    controlPanel_->setCameraState(
        viewport_->cameraDistance(),
        viewport_->verticalFovDegrees(),
        viewport_->nearPlane(),
        viewport_->farPlane(),
        viewport_->orbitCenter());
}
