#include "viewer_window.h"

#include <utility>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QOpenGLContext>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSurfaceFormat>
#include <QDebug>
#include <QVBoxLayout>
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
constexpr float kSphereFocusCameraDistance = 5.2F;
constexpr float kSphereFocusVerticalFovDegrees = 38.0F;

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

renderer::scene_contract::CameraData makeCamera(
    int viewportWidth,
    int viewportHeight,
    float cameraDistance,
    float verticalFovDegrees)
{
    renderer::scene_contract::CameraData camera;
    const float aspectRatio = viewportHeight > 0
        ? static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)
        : 1.0F;

    camera.position = {0.0F, 1.1F, cameraDistance};
    camera.view = renderer::scene_contract::math::makeLookAt(
        camera.position,
        {0.0F, 0.2F, 0.0F},
        {0.0F, 1.0F, 0.0F});
    camera.projection = renderer::scene_contract::math::makePerspective(
        verticalFovDegrees * renderer::scene_contract::math::kPi / 180.0F,
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

renderer::scene_contract::ColorRgba makeColorFromRgbSpin(double red, double green, double blue, float alpha = 1.0F) {
    return {
        static_cast<float>(red),
        static_cast<float>(green),
        static_cast<float>(blue),
        alpha
    };
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
    viewport_ = new Viewport();

    auto* centralWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* viewportContainer = QWidget::createWindowContainer(viewport_, centralWidget);
    viewportContainer->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(viewportContainer, 1);
    layout->addWidget(buildControlPanel());

    setCentralWidget(centralWidget);
    resize(1280, 720);
    setWindowTitle("Renderer Test Viewer");
}

ViewerWindow::Viewport::Viewport() {
    setFormat(makeFormat());

    sceneObjects_[0].meshData = renderer::parametric_model::PrimitiveFactory::makeBox(1.0F, 1.0F, 1.0F);
    sceneObjects_[0].materialData.baseColor = kDefaultSceneObjects[0].color;
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
    cameraDistance_ = kDefaultCameraDistance;
    verticalFovDegrees_ = kDefaultVerticalFovDegrees;

    animationClock_.start();

    frameTimer_.setInterval(16);
    connect(&frameTimer_, &QTimer::timeout, this, [this]() {
        update();
    });
    frameTimer_.start();
}

void ViewerWindow::Viewport::resetDefaults() {
    light_ = kDefaultLight;
    cameraDistance_ = kDefaultCameraDistance;
    verticalFovDegrees_ = kDefaultVerticalFovDegrees;

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
    cameraDistance_ = kSphereFocusCameraDistance;
    verticalFovDegrees_ = kSphereFocusVerticalFovDegrees;

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

    if (renderer_.isInitialized() &&
        sceneObjects_[2].materialHandle != renderer::scene_contract::kInvalidMaterialHandle) {
        renderer_.updateMaterial(sceneObjects_[2].materialHandle, sceneObjects_[2].materialData);
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
    if (distance < 1.5F) {
        distance = 1.5F;
    }
    cameraDistance_ = distance;
    update();
}

float ViewerWindow::Viewport::cameraDistance() const {
    return cameraDistance_;
}

void ViewerWindow::Viewport::setVerticalFovDegrees(float degrees) {
    if (degrees < 20.0F) {
        degrees = 20.0F;
    } else if (degrees > 90.0F) {
        degrees = 90.0F;
    }
    verticalFovDegrees_ = degrees;
    update();
}

float ViewerWindow::Viewport::verticalFovDegrees() const {
    return verticalFovDegrees_;
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

    auto scene = repository_.snapshot(makeCamera(width(), height(), cameraDistance_, verticalFovDegrees_));
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

QWidget* ViewerWindow::buildControlPanel() {
    static const std::array<const char*, kSceneObjectCount> kObjectNames = {
        "Box",
        "Cylinder",
        "Sphere"
    };

    auto* panel = new QWidget(this);
    panel->setFixedWidth(320);

    auto* rootLayout = new QVBoxLayout(panel);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    std::array<QCheckBox*, kSceneObjectCount> visibleCheckBoxes {};
    std::array<QDoubleSpinBox*, kSceneObjectCount> speedSpinBoxes {};
    std::array<QDoubleSpinBox*, kSceneObjectCount> redSpinBoxes {};
    std::array<QDoubleSpinBox*, kSceneObjectCount> greenSpinBoxes {};
    std::array<QDoubleSpinBox*, kSceneObjectCount> blueSpinBoxes {};

    for (int index = 0; index < Viewport::kSceneObjectCount; ++index) {
        auto* group = new QGroupBox(QString::fromLatin1(kObjectNames[index]), panel);
        auto* formLayout = new QFormLayout(group);

        auto* visibleCheckBox = new QCheckBox(group);
        visibleCheckBox->setChecked(viewport_->objectVisible(index));
        connect(visibleCheckBox, &QCheckBox::toggled, this, [this, index](bool checked) {
            viewport_->setObjectVisible(index, checked);
        });
        visibleCheckBoxes[index] = visibleCheckBox;

        auto* speedSpinBox = new QDoubleSpinBox(group);
        speedSpinBox->setRange(-5.0, 5.0);
        speedSpinBox->setSingleStep(0.05);
        speedSpinBox->setDecimals(2);
        speedSpinBox->setValue(viewport_->objectRotationSpeed(index));
        connect(speedSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, index](double value) {
            viewport_->setObjectRotationSpeed(index, static_cast<float>(value));
        });
        speedSpinBoxes[index] = speedSpinBox;

        const auto color = viewport_->objectColor(index);

        auto* redSpinBox = new QDoubleSpinBox(group);
        redSpinBox->setRange(0.0, 1.0);
        redSpinBox->setSingleStep(0.05);
        redSpinBox->setDecimals(2);
        redSpinBox->setValue(color.r);
        redSpinBoxes[index] = redSpinBox;

        auto* greenSpinBox = new QDoubleSpinBox(group);
        greenSpinBox->setRange(0.0, 1.0);
        greenSpinBox->setSingleStep(0.05);
        greenSpinBox->setDecimals(2);
        greenSpinBox->setValue(color.g);
        greenSpinBoxes[index] = greenSpinBox;

        auto* blueSpinBox = new QDoubleSpinBox(group);
        blueSpinBox->setRange(0.0, 1.0);
        blueSpinBox->setSingleStep(0.05);
        blueSpinBox->setDecimals(2);
        blueSpinBox->setValue(color.b);
        blueSpinBoxes[index] = blueSpinBox;

        auto updateColor = [this, index, redSpinBox, greenSpinBox, blueSpinBox]() {
            viewport_->setObjectColor(
                index,
                makeColorFromRgbSpin(
                    redSpinBox->value(),
                    greenSpinBox->value(),
                    blueSpinBox->value()));
        };

        connect(redSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [updateColor](double) {
            updateColor();
        });
        connect(greenSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [updateColor](double) {
            updateColor();
        });
        connect(blueSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [updateColor](double) {
            updateColor();
        });

        formLayout->addRow("Visible", visibleCheckBox);
        formLayout->addRow("Rotation", speedSpinBox);
        formLayout->addRow("Red", redSpinBox);
        formLayout->addRow("Green", greenSpinBox);
        formLayout->addRow("Blue", blueSpinBox);

        rootLayout->addWidget(group);
    }

    auto* lightGroup = new QGroupBox("Lighting", panel);
    auto* lightLayout = new QFormLayout(lightGroup);

    auto* ambientSpinBox = new QDoubleSpinBox(lightGroup);
    ambientSpinBox->setRange(0.0, 1.0);
    ambientSpinBox->setSingleStep(0.02);
    ambientSpinBox->setDecimals(2);
    ambientSpinBox->setValue(viewport_->ambientStrength());
    connect(ambientSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        viewport_->setAmbientStrength(static_cast<float>(value));
    });

    const auto direction = viewport_->lightDirection();

    auto* lightXSpinBox = new QDoubleSpinBox(lightGroup);
    lightXSpinBox->setRange(-2.0, 2.0);
    lightXSpinBox->setSingleStep(0.05);
    lightXSpinBox->setDecimals(2);
    lightXSpinBox->setValue(direction.x);

    auto* lightYSpinBox = new QDoubleSpinBox(lightGroup);
    lightYSpinBox->setRange(-2.0, 2.0);
    lightYSpinBox->setSingleStep(0.05);
    lightYSpinBox->setDecimals(2);
    lightYSpinBox->setValue(direction.y);

    auto* lightZSpinBox = new QDoubleSpinBox(lightGroup);
    lightZSpinBox->setRange(-2.0, 2.0);
    lightZSpinBox->setSingleStep(0.05);
    lightZSpinBox->setDecimals(2);
    lightZSpinBox->setValue(direction.z);

    auto updateLightDirection = [this, lightXSpinBox, lightYSpinBox, lightZSpinBox]() {
        viewport_->setLightDirection({
            static_cast<float>(lightXSpinBox->value()),
            static_cast<float>(lightYSpinBox->value()),
            static_cast<float>(lightZSpinBox->value())
        });
    };

    connect(lightXSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [updateLightDirection](double) {
        updateLightDirection();
    });
    connect(lightYSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [updateLightDirection](double) {
        updateLightDirection();
    });
    connect(lightZSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [updateLightDirection](double) {
        updateLightDirection();
    });

    lightLayout->addRow("Ambient", ambientSpinBox);
    lightLayout->addRow("Light X", lightXSpinBox);
    lightLayout->addRow("Light Y", lightYSpinBox);
    lightLayout->addRow("Light Z", lightZSpinBox);

    auto* cameraGroup = new QGroupBox("Camera", panel);
    auto* cameraLayout = new QFormLayout(cameraGroup);

    auto* distanceSpinBox = new QDoubleSpinBox(cameraGroup);
    distanceSpinBox->setRange(1.5, 20.0);
    distanceSpinBox->setSingleStep(0.1);
    distanceSpinBox->setDecimals(2);
    distanceSpinBox->setValue(viewport_->cameraDistance());
    connect(distanceSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        viewport_->setCameraDistance(static_cast<float>(value));
    });

    auto* fovSpinBox = new QDoubleSpinBox(cameraGroup);
    fovSpinBox->setRange(20.0, 90.0);
    fovSpinBox->setSingleStep(1.0);
    fovSpinBox->setDecimals(1);
    fovSpinBox->setValue(viewport_->verticalFovDegrees());
    connect(fovSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        viewport_->setVerticalFovDegrees(static_cast<float>(value));
    });

    cameraLayout->addRow("Distance", distanceSpinBox);
    cameraLayout->addRow("Vertical FOV", fovSpinBox);

    auto syncObjectControls = [this, visibleCheckBoxes, speedSpinBoxes, redSpinBoxes, greenSpinBoxes, blueSpinBoxes](int index) {
        const QSignalBlocker visibleBlocker(visibleCheckBoxes[index]);
        const QSignalBlocker speedBlocker(speedSpinBoxes[index]);
        const QSignalBlocker redBlocker(redSpinBoxes[index]);
        const QSignalBlocker greenBlocker(greenSpinBoxes[index]);
        const QSignalBlocker blueBlocker(blueSpinBoxes[index]);

        const auto color = viewport_->objectColor(index);
        visibleCheckBoxes[index]->setChecked(viewport_->objectVisible(index));
        speedSpinBoxes[index]->setValue(viewport_->objectRotationSpeed(index));
        redSpinBoxes[index]->setValue(color.r);
        greenSpinBoxes[index]->setValue(color.g);
        blueSpinBoxes[index]->setValue(color.b);
    };

    auto syncLightControls = [this, ambientSpinBox, lightXSpinBox, lightYSpinBox, lightZSpinBox]() {
        const QSignalBlocker ambientBlocker(ambientSpinBox);
        const QSignalBlocker xBlocker(lightXSpinBox);
        const QSignalBlocker yBlocker(lightYSpinBox);
        const QSignalBlocker zBlocker(lightZSpinBox);

        const auto directionValue = viewport_->lightDirection();
        ambientSpinBox->setValue(viewport_->ambientStrength());
        lightXSpinBox->setValue(directionValue.x);
        lightYSpinBox->setValue(directionValue.y);
        lightZSpinBox->setValue(directionValue.z);
    };

    auto syncCameraControls = [this, distanceSpinBox, fovSpinBox]() {
        const QSignalBlocker distanceBlocker(distanceSpinBox);
        const QSignalBlocker fovBlocker(fovSpinBox);

        distanceSpinBox->setValue(viewport_->cameraDistance());
        fovSpinBox->setValue(viewport_->verticalFovDegrees());
    };

    auto* actionGroup = new QGroupBox("Actions", panel);
    auto* actionLayout = new QVBoxLayout(actionGroup);

    auto* resetButton = new QPushButton("Reset Defaults", actionGroup);
    connect(resetButton, &QPushButton::clicked, this, [this, syncObjectControls, syncLightControls, syncCameraControls]() {
        viewport_->resetDefaults();
        for (int index = 0; index < Viewport::kSceneObjectCount; ++index) {
            syncObjectControls(index);
        }
        syncLightControls();
        syncCameraControls();
    });

    auto* sphereFocusButton = new QPushButton("Focus Sphere", actionGroup);
    connect(sphereFocusButton, &QPushButton::clicked, this, [this, syncObjectControls, syncLightControls, syncCameraControls]() {
        viewport_->applySphereFocusPreset();
        for (int index = 0; index < Viewport::kSceneObjectCount; ++index) {
            syncObjectControls(index);
        }
        syncLightControls();
        syncCameraControls();
    });

    actionLayout->addWidget(resetButton);
    actionLayout->addWidget(sphereFocusButton);

    rootLayout->addWidget(lightGroup);
    rootLayout->addWidget(cameraGroup);
    rootLayout->addWidget(actionGroup);
    rootLayout->addStretch(1);
    return panel;
}
