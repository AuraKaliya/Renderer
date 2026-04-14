#include "viewer_window.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

#include <QHBoxLayout>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPointF>
#include <QSurfaceFormat>
#include <QDebug>
#include <QWheelEvent>
#include <QWidget>

#include "camera_focus_bounds_utils.h"
#include "renderer/scene_contract/math_utils.h"
#include "renderer/parametric_model/primitive_factory.h"

namespace {

constexpr std::size_t kDefaultSceneObjectCount = 3;
constexpr int kDefaultWindowWidth = 1240;
constexpr int kDefaultWindowHeight = 680;

struct SceneObjectDefaults {
    renderer::scene_contract::ColorRgba color;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    float offsetZ = 0.0F;
    float scale = 1.0F;
    float rotationSpeed = 1.0F;
    bool visible = true;
};

const std::array<SceneObjectDefaults, kDefaultSceneObjectCount> kDefaultSceneObjects = {{
    {{0.15F, 0.55F, 0.85F, 1.0F}, -2.0F, 0.0F, 0.0F, 0.95F, 1.0F, true},
    {{0.92F, 0.54F, 0.21F, 1.0F}, 0.0F, 0.1F, 0.0F, 1.0F, -0.8F, true},
    {{0.32F, 0.82F, 0.56F, 1.0F}, 2.0F, 0.05F, 0.0F, 0.9F, 1.25F, true}
}};

renderer::parametric_model::PrimitiveDescriptor makeDefaultPrimitiveDescriptor(
    renderer::parametric_model::PrimitiveKind kind)
{
    switch (kind) {
    case renderer::parametric_model::PrimitiveKind::box:
        return renderer::parametric_model::PrimitiveFactory::makeBoxDescriptor(1.0F, 1.0F, 1.0F);
    case renderer::parametric_model::PrimitiveKind::cylinder:
        return renderer::parametric_model::PrimitiveFactory::makeCylinderDescriptor(0.55F, 1.35F, 32U);
    case renderer::parametric_model::PrimitiveKind::sphere:
        return renderer::parametric_model::PrimitiveFactory::makeSphereDescriptor(0.7F, 28U, 18U);
    }

    return renderer::parametric_model::PrimitiveFactory::makeBoxDescriptor(1.0F, 1.0F, 1.0F);
}

SceneObjectDefaults makeDynamicSceneObjectDefaults(
    renderer::parametric_model::PrimitiveKind kind,
    std::size_t index)
{
    if (index < kDefaultSceneObjectCount) {
        return kDefaultSceneObjects[index];
    }

    static const std::array<renderer::scene_contract::ColorRgba, 6> kExtraColors = {{
        {0.86F, 0.33F, 0.28F, 1.0F},
        {0.24F, 0.72F, 0.88F, 1.0F},
        {0.92F, 0.76F, 0.28F, 1.0F},
        {0.48F, 0.80F, 0.42F, 1.0F},
        {0.84F, 0.48F, 0.78F, 1.0F},
        {0.55F, 0.60F, 0.90F, 1.0F}
    }};

    SceneObjectDefaults defaults;
    defaults.color = kExtraColors[index % kExtraColors.size()];
    const int column = static_cast<int>(index % 4U);
    const int row = static_cast<int>(index / 4U);
    defaults.offsetX = (static_cast<float>(column) - 1.5F) * 2.2F;
    defaults.offsetY = kind == renderer::parametric_model::PrimitiveKind::cylinder ? 0.1F : 0.0F;
    defaults.offsetZ = static_cast<float>(row) * 1.8F;
    defaults.scale = kind == renderer::parametric_model::PrimitiveKind::sphere ? 0.9F : 1.0F;
    defaults.rotationSpeed = 0.9F + static_cast<float>(index % 5U) * 0.2F;
    defaults.visible = true;
    return defaults;
}

renderer::parametric_model::ParametricObjectDescriptor makeDefaultParametricObject(
    const renderer::parametric_model::PrimitiveDescriptor& basePrimitive)
{
    auto descriptor = renderer::parametric_model::PrimitiveFactory::makeParametricObject(basePrimitive);
    descriptor.features.push_back(renderer::parametric_model::PrimitiveFactory::makeMirrorFeature());
    descriptor.features.push_back(renderer::parametric_model::PrimitiveFactory::makeLinearArrayFeature());
    return descriptor;
}

const std::array<renderer::parametric_model::ParametricObjectDescriptor, kDefaultSceneObjectCount> kDefaultParametricObjects = {{
    makeDefaultParametricObject({
        renderer::parametric_model::PrimitiveKind::box,
        {1.0F, 1.0F, 1.0F},
        {},
        {}
    }),
    makeDefaultParametricObject({
        renderer::parametric_model::PrimitiveKind::cylinder,
        {},
        {0.55F, 1.35F, 32U},
        {}
    }),
    makeDefaultParametricObject({
        renderer::parametric_model::PrimitiveKind::sphere,
        {},
        {},
        {0.7F, 28U, 18U}
    })
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
constexpr float kViewportZoomRayEpsilon = 0.0001F;
constexpr int kViewportClickDragThresholdPixels = 4;

renderer::scene_contract::Vec3f scaleVec3(
    const renderer::scene_contract::Vec3f& value,
    float factor)
{
    return {
        value.x * factor,
        value.y * factor,
        value.z * factor
    };
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

struct CameraFrame {
    renderer::scene_contract::Vec3f position {};
    renderer::scene_contract::Vec3f forward {0.0F, 0.0F, -1.0F};
    renderer::scene_contract::Vec3f right {1.0F, 0.0F, 0.0F};
    renderer::scene_contract::Vec3f up {0.0F, 1.0F, 0.0F};
};

CameraFrame makeCameraFrame(const renderer::scene_contract::CameraData& camera) {
    CameraFrame frame;
    frame.position = camera.position;
    frame.forward = renderer::scene_contract::math::normalize({
        -camera.view.elements[2],
        -camera.view.elements[6],
        -camera.view.elements[10]
    });
    frame.right = renderer::scene_contract::math::normalize(
        {camera.view.elements[0], camera.view.elements[4], camera.view.elements[8]});
    if (renderer::scene_contract::math::length(frame.right) <= kViewportZoomRayEpsilon) {
        frame.right = {1.0F, 0.0F, 0.0F};
    }
    frame.up = renderer::scene_contract::math::normalize(
        {camera.view.elements[1], camera.view.elements[5], camera.view.elements[9]});
    if (renderer::scene_contract::math::length(frame.up) <= kViewportZoomRayEpsilon) {
        frame.up = renderer::scene_contract::math::normalize(
            renderer::scene_contract::math::cross(frame.right, frame.forward));
    }
    return frame;
}

struct ViewportRay {
    renderer::scene_contract::Vec2f viewportPositionNormalized {};
    renderer::scene_contract::Vec3f origin {};
    renderer::scene_contract::Vec3f direction {0.0F, 0.0F, -1.0F};
    renderer::scene_contract::Vec3f cameraForward {0.0F, 0.0F, -1.0F};
    bool valid = false;
};

ViewportRay makeViewportRay(
    const OrbitCameraController& cameraController,
    int viewportWidth,
    int viewportHeight,
    const QPointF& viewportPosition)
{
    ViewportRay ray;
    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return ray;
    }

    const float normalizedX = static_cast<float>(viewportPosition.x()) / static_cast<float>(viewportWidth);
    const float normalizedY = static_cast<float>(viewportPosition.y()) / static_cast<float>(viewportHeight);
    if (normalizedX < 0.0F || normalizedX > 1.0F || normalizedY < 0.0F || normalizedY > 1.0F) {
        return ray;
    }
    ray.viewportPositionNormalized = {normalizedX, normalizedY};

    const float ndcX = normalizedX * 2.0F - 1.0F;
    const float ndcY = 1.0F - normalizedY * 2.0F;
    const auto camera = cameraController.buildCameraData();
    const auto cameraFrame = makeCameraFrame(camera);
    if (std::abs(camera.projection.elements[0]) <= kViewportZoomRayEpsilon
        || std::abs(camera.projection.elements[5]) <= kViewportZoomRayEpsilon) {
        return ray;
    }

    ray.origin = cameraFrame.position;
    ray.direction = cameraFrame.forward;
    ray.cameraForward = cameraFrame.forward;

    if (cameraController.projectionMode() == OrbitCameraController::ProjectionMode::perspective) {
        const auto horizontalOffset = scaleVec3(cameraFrame.right, ndcX / camera.projection.elements[0]);
        const auto verticalOffset = scaleVec3(cameraFrame.up, ndcY / camera.projection.elements[5]);
        ray.direction = renderer::scene_contract::math::normalize(
            renderer::scene_contract::math::add(
                cameraFrame.forward,
                renderer::scene_contract::math::add(horizontalOffset, verticalOffset)));
    } else {
        ray.origin = renderer::scene_contract::math::add(
            ray.origin,
            renderer::scene_contract::math::add(
                scaleVec3(cameraFrame.right, ndcX / camera.projection.elements[0]),
                scaleVec3(cameraFrame.up, ndcY / camera.projection.elements[5])));
    }

    ray.valid = renderer::scene_contract::math::length(ray.direction) > kViewportZoomRayEpsilon;
    return ray;
}

bool intersectRayAabb(
    const renderer::scene_contract::Vec3f& rayOrigin,
    const renderer::scene_contract::Vec3f& rayDirection,
    const renderer::scene_contract::Aabb& bounds,
    float& hitDistance)
{
    if (!bounds.valid) {
        return false;
    }

    auto intersectAxis = [](
        float origin,
        float direction,
        float minimum,
        float maximum,
        float& nearDistance,
        float& farDistance) {
        if (std::abs(direction) <= kViewportZoomRayEpsilon) {
            return origin >= minimum && origin <= maximum;
        }

        float axisNearDistance = (minimum - origin) / direction;
        float axisFarDistance = (maximum - origin) / direction;
        if (axisNearDistance > axisFarDistance) {
            std::swap(axisNearDistance, axisFarDistance);
        }

        nearDistance = std::max(nearDistance, axisNearDistance);
        farDistance = std::min(farDistance, axisFarDistance);
        return nearDistance <= farDistance;
    };

    float nearDistance = 0.0F;
    float farDistance = std::numeric_limits<float>::max();
    if (!intersectAxis(rayOrigin.x, rayDirection.x, bounds.min.x, bounds.max.x, nearDistance, farDistance)
        || !intersectAxis(rayOrigin.y, rayDirection.y, bounds.min.y, bounds.max.y, nearDistance, farDistance)
        || !intersectAxis(rayOrigin.z, rayDirection.z, bounds.min.z, bounds.max.z, nearDistance, farDistance)) {
        return false;
    }

    hitDistance = nearDistance;
    return farDistance >= 0.0F;
}

renderer::scene_contract::Vec3f transformPoint(
    const renderer::scene_contract::Mat4f& matrix,
    const renderer::scene_contract::Vec3f& point)
{
    return {
        matrix.elements[0] * point.x + matrix.elements[4] * point.y + matrix.elements[8] * point.z + matrix.elements[12],
        matrix.elements[1] * point.x + matrix.elements[5] * point.y + matrix.elements[9] * point.z + matrix.elements[13],
        matrix.elements[2] * point.x + matrix.elements[6] * point.y + matrix.elements[10] * point.z + matrix.elements[14]
    };
}

bool intersectRayTriangle(
    const renderer::scene_contract::Vec3f& rayOrigin,
    const renderer::scene_contract::Vec3f& rayDirection,
    const renderer::scene_contract::Vec3f& vertex0,
    const renderer::scene_contract::Vec3f& vertex1,
    const renderer::scene_contract::Vec3f& vertex2,
    float& hitDistance)
{
    const auto edge1 = renderer::scene_contract::math::subtract(vertex1, vertex0);
    const auto edge2 = renderer::scene_contract::math::subtract(vertex2, vertex0);
    const auto pvec = renderer::scene_contract::math::cross(rayDirection, edge2);
    const float determinant = renderer::scene_contract::math::dot(edge1, pvec);
    if (std::abs(determinant) <= kViewportZoomRayEpsilon) {
        return false;
    }

    const float inverseDeterminant = 1.0F / determinant;
    const auto tvec = renderer::scene_contract::math::subtract(rayOrigin, vertex0);
    const float u = renderer::scene_contract::math::dot(tvec, pvec) * inverseDeterminant;
    if (u < 0.0F || u > 1.0F) {
        return false;
    }

    const auto qvec = renderer::scene_contract::math::cross(tvec, edge1);
    const float v = renderer::scene_contract::math::dot(rayDirection, qvec) * inverseDeterminant;
    if (v < 0.0F || u + v > 1.0F) {
        return false;
    }

    const float distance = renderer::scene_contract::math::dot(edge2, qvec) * inverseDeterminant;
    if (distance < 0.0F) {
        return false;
    }

    hitDistance = distance;
    return true;
}

bool intersectRayMesh(
    const renderer::scene_contract::Vec3f& rayOrigin,
    const renderer::scene_contract::Vec3f& rayDirection,
    const renderer::scene_contract::MeshData& mesh,
    const renderer::scene_contract::Mat4f& world,
    float& hitDistance)
{
    bool hit = false;
    float nearestDistance = std::numeric_limits<float>::max();

    for (std::size_t index = 0; index + 2U < mesh.indices.size(); index += 3U) {
        const auto index0 = mesh.indices[index];
        const auto index1 = mesh.indices[index + 1U];
        const auto index2 = mesh.indices[index + 2U];
        if (index0 >= mesh.vertices.size() || index1 >= mesh.vertices.size() || index2 >= mesh.vertices.size()) {
            continue;
        }

        float triangleHitDistance = 0.0F;
        if (!intersectRayTriangle(
                rayOrigin,
                rayDirection,
                transformPoint(world, mesh.vertices[index0].position),
                transformPoint(world, mesh.vertices[index1].position),
                transformPoint(world, mesh.vertices[index2].position),
                triangleHitDistance)) {
            continue;
        }

        if (triangleHitDistance < nearestDistance) {
            nearestDistance = triangleHitDistance;
            hit = true;
        }
    }

    hitDistance = nearestDistance;
    return hit;
}

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

    bindControlPanelSignals();

    setCentralWidget(centralWidget);
    resize(kDefaultWindowWidth, kDefaultWindowHeight);
    setWindowTitle("Renderer Test Viewer");
    syncControlPanel();
}

void ViewerWindow::bindControlPanelSignals() {
    if (viewport_ == nullptr || controlPanel_ == nullptr) {
        return;
    }

    connect(controlPanel_, &ViewerControlPanel::objectVisibleChanged, this, [this](int index, bool visible) {
        viewport_->setObjectVisible(index, visible);
    });
    connect(controlPanel_, &ViewerControlPanel::objectRotationSpeedChanged, this, [this](int index, float speed) {
        viewport_->setObjectRotationSpeed(index, speed);
    });
    connect(controlPanel_, &ViewerControlPanel::objectColorChanged, this, [this](int index, float red, float green, float blue) {
        viewport_->setObjectColor(index, makeColorFromRgbSpin(red, green, blue));
    });
    connect(controlPanel_, &ViewerControlPanel::objectMirrorEnabledChanged, this, [this](int index, bool enabled) {
        viewport_->setObjectMirrorEnabled(index, enabled);
    });
    connect(controlPanel_, &ViewerControlPanel::objectMirrorAxisChanged, this, [this](int index, int axis) {
        viewport_->setObjectMirrorAxis(index, static_cast<renderer::parametric_model::Axis>(axis));
    });
    connect(controlPanel_, &ViewerControlPanel::objectMirrorPlaneOffsetChanged, this, [this](int index, float planeOffset) {
        viewport_->setObjectMirrorPlaneOffset(index, planeOffset);
    });
    connect(controlPanel_, &ViewerControlPanel::objectLinearArrayEnabledChanged, this, [this](int index, bool enabled) {
        viewport_->setObjectLinearArrayEnabled(index, enabled);
    });
    connect(controlPanel_, &ViewerControlPanel::objectLinearArrayCountChanged, this, [this](int index, int count) {
        viewport_->setObjectLinearArrayCount(index, static_cast<std::uint32_t>(count));
    });
    connect(controlPanel_, &ViewerControlPanel::objectLinearArrayOffsetChanged, this, [this](int index, float x, float y, float z) {
        viewport_->setObjectLinearArrayOffset(index, {x, y, z});
    });
    connect(controlPanel_, &ViewerControlPanel::objectFeatureAddRequested, this, [this](int index, int featureKind) {
        viewport_->addObjectFeature(index, static_cast<renderer::parametric_model::FeatureKind>(featureKind));
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::objectFeatureRemoveRequested, this, [this](int index, int featureId) {
        viewport_->removeObjectFeature(
            index,
            static_cast<renderer::parametric_model::ParametricFeatureId>(featureId));
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::objectFeatureEnabledChanged, this, [this](int index, int featureId, bool enabled) {
        viewport_->setObjectFeatureEnabled(
            index,
            static_cast<renderer::parametric_model::ParametricFeatureId>(featureId),
            enabled);
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::objectAddRequested, this, [this](int primitiveKind) {
        viewport_->addObject(static_cast<renderer::parametric_model::PrimitiveKind>(primitiveKind));
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::deleteSelectedObjectRequested, this, [this]() {
        viewport_->removeSelectedObject();
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::objectSelectionChanged, this, [this](int objectId) {
        viewport_->setSelectedObject(static_cast<renderer::parametric_model::ParametricObjectId>(objectId));
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::objectActivationRequested, this, [this](int objectId) {
        viewport_->setActiveObject(static_cast<renderer::parametric_model::ParametricObjectId>(objectId));
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::featureSelectionChanged, this, [this](int objectId, int featureId) {
        viewport_->setSelectedObject(static_cast<renderer::parametric_model::ParametricObjectId>(objectId));
        viewport_->setSelectedFeature(static_cast<renderer::parametric_model::ParametricFeatureId>(featureId));
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::featureActivationRequested, this, [this](int objectId, int featureId) {
        viewport_->setActiveObject(static_cast<renderer::parametric_model::ParametricObjectId>(objectId));
        viewport_->setActiveFeature(static_cast<renderer::parametric_model::ParametricFeatureId>(featureId));
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::focusSelectedObjectRequested, this, [this]() {
        viewport_->focusSelectedObject();
        syncControlPanel();
    });
    connect(controlPanel_, &ViewerControlPanel::ambientStrengthChanged, this, [this](float strength) {
        viewport_->setAmbientStrength(strength);
    });
    connect(controlPanel_, &ViewerControlPanel::lightDirectionChanged, this, [this](float x, float y, float z) {
        viewport_->setLightDirection({x, y, z});
    });
    connect(controlPanel_, &ViewerControlPanel::projectionModeChanged, this, [this](int mode) {
        viewport_->setProjectionMode(
            mode == 1
                ? OrbitCameraController::ProjectionMode::orthographic
                : OrbitCameraController::ProjectionMode::perspective);
    });
    connect(controlPanel_, &ViewerControlPanel::zoomModeChanged, this, [this](int mode) {
        viewport_->setZoomMode(
            mode == 1
                ? OrbitCameraController::ZoomMode::lens
                : OrbitCameraController::ZoomMode::dolly);
    });
    connect(controlPanel_, &ViewerControlPanel::modelChangeViewStrategyChanged, this, [this](int strategy) {
        viewport_->setModelChangeViewStrategy(
            strategy == 1
                ? model_change_view::ViewStrategy::auto_frame
                : model_change_view::ViewStrategy::keep_view);
    });
    connect(controlPanel_, &ViewerControlPanel::boxWidthChanged, this, [this](float width) {
        viewport_->setBoxWidth(width);
    });
    connect(controlPanel_, &ViewerControlPanel::boxHeightChanged, this, [this](float height) {
        viewport_->setBoxHeight(height);
    });
    connect(controlPanel_, &ViewerControlPanel::boxDepthChanged, this, [this](float depth) {
        viewport_->setBoxDepth(depth);
    });
    connect(controlPanel_, &ViewerControlPanel::cylinderRadiusChanged, this, [this](float radius) {
        viewport_->setCylinderRadius(radius);
    });
    connect(controlPanel_, &ViewerControlPanel::cylinderHeightChanged, this, [this](float height) {
        viewport_->setCylinderHeight(height);
    });
    connect(controlPanel_, &ViewerControlPanel::cylinderSegmentsChanged, this, [this](int segments) {
        viewport_->setCylinderSegments(static_cast<std::uint32_t>(segments));
    });
    connect(controlPanel_, &ViewerControlPanel::sphereRadiusChanged, this, [this](float radius) {
        viewport_->setSphereRadius(radius);
    });
    connect(controlPanel_, &ViewerControlPanel::sphereSlicesChanged, this, [this](int slices) {
        viewport_->setSphereSlices(static_cast<std::uint32_t>(slices));
    });
    connect(controlPanel_, &ViewerControlPanel::sphereStacksChanged, this, [this](int stacks) {
        viewport_->setSphereStacks(static_cast<std::uint32_t>(stacks));
    });
    connect(controlPanel_, &ViewerControlPanel::cameraDistanceChanged, this, [this](float distance) {
        viewport_->setCameraDistance(distance);
    });
    connect(controlPanel_, &ViewerControlPanel::verticalFovDegreesChanged, this, [this](float degrees) {
        viewport_->setVerticalFovDegrees(degrees);
    });
    connect(controlPanel_, &ViewerControlPanel::orthographicHeightChanged, this, [this](float height) {
        viewport_->setOrthographicHeight(height);
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
}

ViewerWindow::Viewport::Viewport(std::function<void()> cameraStateChangedCallback)
    : cameraStateChangedCallback_(std::move(cameraStateChangedCallback))
{
    setFormat(makeFormat());
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    sceneObjects_.resize(kDefaultSceneObjectCount);
    for (std::size_t index = 0; index < kDefaultSceneObjectCount; ++index) {
        auto& sceneObject = sceneObjects_[index];
        const auto& defaults = kDefaultSceneObjects[index];

        sceneObject.parametricObjectDescriptor = kDefaultParametricObjects[index];
        sceneObject.meshData = renderer::parametric_model::PrimitiveFactory::build(sceneObject.parametricObjectDescriptor);
        sceneObject.materialData.baseColor = defaults.color;
        sceneObject.offsetX = defaults.offsetX;
        sceneObject.offsetY = defaults.offsetY;
        sceneObject.offsetZ = defaults.offsetZ;
        sceneObject.scale = defaults.scale;
        sceneObject.rotationSpeed = defaults.rotationSpeed;
        sceneObject.visible = defaults.visible;

        if (index == 0U) {
            sceneObject.textureData = makeCheckerTextureData(64, 64, 8U);
            sceneObject.materialData.useBaseColorTexture = true;
        }
    }

    light_ = kDefaultLight;
    model_change_view::setViewStrategy(modelChangeViewState_, model_change_view::ViewStrategy::keep_view);
    cameraController_.setOrbitCenter({0.0F, kDefaultCameraTargetY, 0.0F});
    cameraController_.setDistance(kDefaultCameraDistance);
    cameraController_.setProjection(kDefaultVerticalFovDegrees, kDefaultNearPlane, kDefaultFarPlane);
    cameraController_.setProjectionMode(OrbitCameraController::ProjectionMode::perspective);
    cameraController_.setPitchRadians(kDefaultCameraPitchRadians);
    cameraController_.setYawRadians(0.0F);
    viewport_zoom::reset(viewportZoomState_);
    refreshViewportZoomState();
    normalizeSelectionState();

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
    cameraController_.setProjectionMode(OrbitCameraController::ProjectionMode::perspective);
    cameraController_.setPitchRadians(kDefaultCameraPitchRadians);
    cameraController_.setYawRadians(0.0F);
    viewport_zoom::reset(viewportZoomState_);

    const bool canTouchRenderer = renderer_.isInitialized() && context() != nullptr;
    if (canTouchRenderer) {
        makeCurrent();
        for (auto& sceneObject : sceneObjects_) {
            releaseSceneObjectResources(sceneObject);
        }
    }

    sceneObjects_.clear();
    sceneObjects_.resize(kDefaultSceneObjectCount);
    for (std::size_t index = 0; index < kDefaultSceneObjectCount; ++index) {
        const auto& defaults = kDefaultSceneObjects[index];
        auto& sceneObject = sceneObjects_[index];
        sceneObject = {};
        sceneObject.parametricObjectDescriptor = kDefaultParametricObjects[index];
        sceneObject.meshData = renderer::parametric_model::PrimitiveFactory::build(sceneObject.parametricObjectDescriptor);
        sceneObject.materialData.baseColor = defaults.color;
        sceneObject.offsetX = defaults.offsetX;
        sceneObject.offsetY = defaults.offsetY;
        sceneObject.offsetZ = defaults.offsetZ;
        sceneObject.scale = defaults.scale;
        sceneObject.rotationSpeed = defaults.rotationSpeed;
        sceneObject.visible = defaults.visible;

        if (index == 0U) {
            sceneObject.textureData = makeCheckerTextureData(64, 64, 8U);
            sceneObject.materialData.useBaseColorTexture = true;
        }

        if (canTouchRenderer) {
            uploadSceneObjectResources(sceneObject);
        }
    }
    if (canTouchRenderer) {
        doneCurrent();
    }

    rebuildRepositoryItems();

    if (!sceneObjects_.empty()) {
        selectionState_.selectedObjectId = sceneObjects_.front().parametricObjectDescriptor.metadata.id;
        selectionState_.activeObjectId = selectionState_.selectedObjectId;
        selectionState_.selectedFeatureId = firstFeatureIdForObject(selectionState_.selectedObjectId);
        selectionState_.activeFeatureId = selectionState_.selectedFeatureId;
    }
    normalizeSelectionState();
    refreshViewportZoomState();
    notifyCameraStateChanged();
    update();
}

void ViewerWindow::Viewport::applySphereFocusPreset() {
    resetDefaults();
    light_ = kSphereFocusLight;
    cameraController_.setProjection(kSphereFocusVerticalFovDegrees, kDefaultNearPlane, kDefaultFarPlane);
    cameraController_.setProjectionMode(OrbitCameraController::ProjectionMode::perspective);
    cameraController_.setPitchRadians(kDefaultCameraPitchRadians);
    cameraController_.setYawRadians(0.0F);
    viewport_zoom::reset(viewportZoomState_);

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

    if (renderer_.isInitialized()
        && context() != nullptr
        && sceneObjects_[2].materialHandle != renderer::scene_contract::kInvalidMaterialHandle) {
        makeCurrent();
        (void)renderer_.updateMaterial(sceneObjects_[2].materialHandle, sceneObjects_[2].materialData);
        doneCurrent();
    }

    selectionState_.selectedObjectId = sceneObjects_[2].parametricObjectDescriptor.metadata.id;
    selectionState_.activeObjectId = selectionState_.selectedObjectId;
    selectionState_.selectedFeatureId = firstFeatureIdForObject(selectionState_.selectedObjectId);
    selectionState_.activeFeatureId = selectionState_.selectedFeatureId;
    normalizeSelectionState();
    updateSceneTransforms();
    cameraController_.focusOnBounds(objectFocusBounds(2));
    refreshViewportZoomState();
    if (cameraStateChangedCallback_) {
        cameraStateChangedCallback_();
    }

    update();
}

void ViewerWindow::Viewport::setObjectVisible(int index, bool visible) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    sceneObjects_[index].visible = visible;
    repository_.updateVisible(sceneObjects_[index].itemId, visible);
    markModelChanged(model_change_view::ChangeKind::scene_visibility);
    processPendingModelChange();
    update();
}

bool ViewerWindow::Viewport::objectVisible(int index) const {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return false;
    }
    return sceneObjects_[index].visible;
}

void ViewerWindow::Viewport::setObjectRotationSpeed(int index, float speed) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    sceneObjects_[index].rotationSpeed = speed;
    update();
}

float ViewerWindow::Viewport::objectRotationSpeed(int index) const {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return 0.0F;
    }
    return sceneObjects_[index].rotationSpeed;
}

void ViewerWindow::Viewport::setObjectColor(int index, const renderer::scene_contract::ColorRgba& color) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto& sceneObject = sceneObjects_[index];
    sceneObject.materialData.baseColor = color;

    if (renderer_.isInitialized() &&
        sceneObject.materialHandle != renderer::scene_contract::kInvalidMaterialHandle) {
        (void)renderer_.updateMaterial(sceneObject.materialHandle, sceneObject.materialData);
    }

    update();
}

renderer::scene_contract::ColorRgba ViewerWindow::Viewport::objectColor(int index) const {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
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
    notifyCameraStateChanged();
    update();
}

float ViewerWindow::Viewport::cameraDistance() const {
    return cameraController_.distance();
}

void ViewerWindow::Viewport::setProjectionMode(OrbitCameraController::ProjectionMode mode) {
    cameraController_.setProjectionMode(mode);
    refreshViewportZoomState();
    notifyCameraStateChanged();
    update();
}

OrbitCameraController::ProjectionMode ViewerWindow::Viewport::projectionMode() const {
    return cameraController_.projectionMode();
}

void ViewerWindow::Viewport::setZoomMode(OrbitCameraController::ZoomMode mode) {
    cameraController_.setZoomMode(mode);
    refreshViewportZoomState();
    notifyCameraStateChanged();
    update();
}

OrbitCameraController::ZoomMode ViewerWindow::Viewport::zoomMode() const {
    return cameraController_.zoomMode();
}

void ViewerWindow::Viewport::setModelChangeViewStrategy(model_change_view::ViewStrategy strategy) {
    model_change_view::setViewStrategy(modelChangeViewState_, strategy);
    notifyCameraStateChanged();
    update();
}

model_change_view::ViewStrategy ViewerWindow::Viewport::modelChangeViewStrategy() const {
    return modelChangeViewState_.viewStrategy;
}

void ViewerWindow::Viewport::setVerticalFovDegrees(float degrees) {
    cameraController_.setVerticalFovDegrees(degrees);
    notifyCameraStateChanged();
    update();
}

float ViewerWindow::Viewport::verticalFovDegrees() const {
    return cameraController_.verticalFovDegrees();
}

void ViewerWindow::Viewport::setOrthographicHeight(float height) {
    cameraController_.setOrthographicHeight(height);
    notifyCameraStateChanged();
    update();
}

float ViewerWindow::Viewport::orthographicHeight() const {
    return cameraController_.orthographicHeight();
}

void ViewerWindow::Viewport::setBoxWidth(float width) {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    auto descriptor = sceneObjects_[objectIndex].parametricObjectDescriptor;
    auto* primitive = ensureObjectPrimitive(descriptor);
    if (primitive == nullptr || primitive->kind != renderer::parametric_model::PrimitiveKind::box) {
        return;
    }

    primitive->box.width = width;
    applyParametricObjectDescriptor(objectIndex, descriptor);
}

void ViewerWindow::Viewport::setBoxHeight(float height) {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    auto descriptor = sceneObjects_[objectIndex].parametricObjectDescriptor;
    auto* primitive = ensureObjectPrimitive(descriptor);
    if (primitive == nullptr || primitive->kind != renderer::parametric_model::PrimitiveKind::box) {
        return;
    }

    primitive->box.height = height;
    applyParametricObjectDescriptor(objectIndex, descriptor);
}

void ViewerWindow::Viewport::setBoxDepth(float depth) {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    auto descriptor = sceneObjects_[objectIndex].parametricObjectDescriptor;
    auto* primitive = ensureObjectPrimitive(descriptor);
    if (primitive == nullptr || primitive->kind != renderer::parametric_model::PrimitiveKind::box) {
        return;
    }

    primitive->box.depth = depth;
    applyParametricObjectDescriptor(objectIndex, descriptor);
}

void ViewerWindow::Viewport::setCylinderRadius(float radius) {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    auto descriptor = sceneObjects_[objectIndex].parametricObjectDescriptor;
    auto* primitive = ensureObjectPrimitive(descriptor);
    if (primitive == nullptr || primitive->kind != renderer::parametric_model::PrimitiveKind::cylinder) {
        return;
    }

    primitive->cylinder.radius = radius;
    applyParametricObjectDescriptor(objectIndex, descriptor);
}

void ViewerWindow::Viewport::setCylinderHeight(float height) {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    auto descriptor = sceneObjects_[objectIndex].parametricObjectDescriptor;
    auto* primitive = ensureObjectPrimitive(descriptor);
    if (primitive == nullptr || primitive->kind != renderer::parametric_model::PrimitiveKind::cylinder) {
        return;
    }

    primitive->cylinder.height = height;
    applyParametricObjectDescriptor(objectIndex, descriptor);
}

void ViewerWindow::Viewport::setCylinderSegments(std::uint32_t segments) {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    auto descriptor = sceneObjects_[objectIndex].parametricObjectDescriptor;
    auto* primitive = ensureObjectPrimitive(descriptor);
    if (primitive == nullptr || primitive->kind != renderer::parametric_model::PrimitiveKind::cylinder) {
        return;
    }

    primitive->cylinder.segments = segments;
    applyParametricObjectDescriptor(objectIndex, descriptor);
}

void ViewerWindow::Viewport::setSphereRadius(float radius) {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    auto descriptor = sceneObjects_[objectIndex].parametricObjectDescriptor;
    auto* primitive = ensureObjectPrimitive(descriptor);
    if (primitive == nullptr || primitive->kind != renderer::parametric_model::PrimitiveKind::sphere) {
        return;
    }

    primitive->sphere.radius = radius;
    applyParametricObjectDescriptor(objectIndex, descriptor);
}

void ViewerWindow::Viewport::setSphereSlices(std::uint32_t slices) {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    auto descriptor = sceneObjects_[objectIndex].parametricObjectDescriptor;
    auto* primitive = ensureObjectPrimitive(descriptor);
    if (primitive == nullptr || primitive->kind != renderer::parametric_model::PrimitiveKind::sphere) {
        return;
    }

    primitive->sphere.slices = slices;
    applyParametricObjectDescriptor(objectIndex, descriptor);
}

void ViewerWindow::Viewport::setSphereStacks(std::uint32_t stacks) {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    auto descriptor = sceneObjects_[objectIndex].parametricObjectDescriptor;
    auto* primitive = ensureObjectPrimitive(descriptor);
    if (primitive == nullptr || primitive->kind != renderer::parametric_model::PrimitiveKind::sphere) {
        return;
    }

    primitive->sphere.stacks = stacks;
    applyParametricObjectDescriptor(objectIndex, descriptor);
}

void ViewerWindow::Viewport::setObjectMirrorEnabled(int index, bool enabled) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto descriptor = sceneObjects_[index].parametricObjectDescriptor;
    auto* mirrorFeature = ensureObjectFeature(descriptor, renderer::parametric_model::FeatureKind::mirror);
    if (mirrorFeature == nullptr) {
        return;
    }

    mirrorFeature->enabled = enabled;
    applyParametricObjectDescriptor(index, descriptor);
}

void ViewerWindow::Viewport::setObjectMirrorAxis(int index, renderer::parametric_model::Axis axis) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto descriptor = sceneObjects_[index].parametricObjectDescriptor;
    auto* mirrorFeature = ensureObjectFeature(descriptor, renderer::parametric_model::FeatureKind::mirror);
    if (mirrorFeature == nullptr) {
        return;
    }

    mirrorFeature->mirror.axis = axis;
    applyParametricObjectDescriptor(index, descriptor);
}

void ViewerWindow::Viewport::setObjectMirrorPlaneOffset(int index, float planeOffset) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto descriptor = sceneObjects_[index].parametricObjectDescriptor;
    auto* mirrorFeature = ensureObjectFeature(descriptor, renderer::parametric_model::FeatureKind::mirror);
    if (mirrorFeature == nullptr) {
        return;
    }

    mirrorFeature->mirror.planeOffset = planeOffset;
    applyParametricObjectDescriptor(index, descriptor);
}

void ViewerWindow::Viewport::setObjectLinearArrayEnabled(int index, bool enabled) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto descriptor = sceneObjects_[index].parametricObjectDescriptor;
    auto* linearArrayFeature = ensureObjectFeature(descriptor, renderer::parametric_model::FeatureKind::linear_array);
    if (linearArrayFeature == nullptr) {
        return;
    }

    linearArrayFeature->enabled = enabled;
    applyParametricObjectDescriptor(index, descriptor);
}

void ViewerWindow::Viewport::setObjectLinearArrayCount(int index, std::uint32_t count) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto descriptor = sceneObjects_[index].parametricObjectDescriptor;
    auto* linearArrayFeature = ensureObjectFeature(descriptor, renderer::parametric_model::FeatureKind::linear_array);
    if (linearArrayFeature == nullptr) {
        return;
    }

    linearArrayFeature->linearArray.count = count;
    applyParametricObjectDescriptor(index, descriptor);
}

void ViewerWindow::Viewport::setObjectLinearArrayOffset(
    int index,
    const renderer::scene_contract::Vec3f& offset)
{
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto descriptor = sceneObjects_[index].parametricObjectDescriptor;
    auto* linearArrayFeature = ensureObjectFeature(descriptor, renderer::parametric_model::FeatureKind::linear_array);
    if (linearArrayFeature == nullptr) {
        return;
    }

    linearArrayFeature->linearArray.offset = offset;
    applyParametricObjectDescriptor(index, descriptor);
}

void ViewerWindow::Viewport::addObjectFeature(int index, renderer::parametric_model::FeatureKind kind) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    if (kind == renderer::parametric_model::FeatureKind::primitive) {
        return;
    }

    auto descriptor = sceneObjects_[index].parametricObjectDescriptor;
    if (findObjectFeature(descriptor, kind) != nullptr) {
        return;
    }

    auto* feature = ensureObjectFeature(descriptor, kind);
    if (feature == nullptr) {
        return;
    }

    if (kind != renderer::parametric_model::FeatureKind::primitive) {
        feature->enabled = false;
    }
    applyParametricObjectDescriptor(index, descriptor);
}

void ViewerWindow::Viewport::removeObjectFeature(
    int index,
    renderer::parametric_model::ParametricFeatureId featureId)
{
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto descriptor = sceneObjects_[index].parametricObjectDescriptor;
    for (auto featureIt = descriptor.features.begin(); featureIt != descriptor.features.end(); ++featureIt) {
        if (featureIt->id != featureId) {
            continue;
        }
        if (featureIt->kind == renderer::parametric_model::FeatureKind::primitive) {
            return;
        }

        descriptor.features.erase(featureIt);
        applyParametricObjectDescriptor(index, descriptor);
        return;
    }
}

void ViewerWindow::Viewport::setObjectFeatureEnabled(
    int index,
    renderer::parametric_model::ParametricFeatureId featureId,
    bool enabled)
{
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto descriptor = sceneObjects_[index].parametricObjectDescriptor;
    auto* feature = findObjectFeatureById(descriptor, featureId);
    if (feature == nullptr) {
        return;
    }
    if (feature->kind == renderer::parametric_model::FeatureKind::primitive) {
        return;
    }

    feature->enabled = enabled;
    applyParametricObjectDescriptor(index, descriptor);
}

void ViewerWindow::Viewport::addObject(renderer::parametric_model::PrimitiveKind kind) {
    SceneObject sceneObject;
    const std::size_t newObjectIndex = sceneObjects_.size();
    const auto defaults = makeDynamicSceneObjectDefaults(kind, newObjectIndex);

    sceneObject.parametricObjectDescriptor = makeDefaultParametricObject(makeDefaultPrimitiveDescriptor(kind));
    sceneObject.meshData = renderer::parametric_model::PrimitiveFactory::build(sceneObject.parametricObjectDescriptor);
    sceneObject.materialData.baseColor = defaults.color;
    sceneObject.offsetX = defaults.offsetX;
    sceneObject.offsetY = defaults.offsetY;
    sceneObject.offsetZ = defaults.offsetZ;
    sceneObject.scale = defaults.scale;
    sceneObject.rotationSpeed = defaults.rotationSpeed;
    sceneObject.visible = defaults.visible;

    if (renderer_.isInitialized() && context() != nullptr) {
        makeCurrent();
        uploadSceneObjectResources(sceneObject);
        doneCurrent();
    }

    sceneObjects_.push_back(std::move(sceneObject));
    rebuildRepositoryItems();

    selectionState_.selectedObjectId = sceneObjects_.back().parametricObjectDescriptor.metadata.id;
    selectionState_.activeObjectId = selectionState_.selectedObjectId;
    selectionState_.selectedFeatureId = firstFeatureIdForObject(selectionState_.selectedObjectId);
    selectionState_.activeFeatureId = selectionState_.selectedFeatureId;
    normalizeSelectionState();
    markModelChanged(model_change_view::ChangeKind::geometry_or_transform);
    processPendingModelChange();
    update();
}

void ViewerWindow::Viewport::removeSelectedObject() {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    const int fallbackIndexBeforeErase =
        sceneObjects_.size() <= 1U
            ? -1
            : std::min(objectIndex, static_cast<int>(sceneObjects_.size()) - 2);

    if (renderer_.isInitialized() && context() != nullptr) {
        makeCurrent();
        releaseSceneObjectResources(sceneObjects_[objectIndex]);
        doneCurrent();
    }

    sceneObjects_.erase(sceneObjects_.begin() + objectIndex);
    rebuildRepositoryItems();
    if (fallbackIndexBeforeErase >= 0 && fallbackIndexBeforeErase < static_cast<int>(sceneObjects_.size())) {
        selectionState_.selectedObjectId = sceneObjects_[fallbackIndexBeforeErase].parametricObjectDescriptor.metadata.id;
        selectionState_.activeObjectId = selectionState_.selectedObjectId;
        selectionState_.selectedFeatureId = firstFeatureIdForObject(selectionState_.selectedObjectId);
        selectionState_.activeFeatureId = selectionState_.selectedFeatureId;
    } else {
        selectionState_.selectedObjectId = 0U;
        selectionState_.activeObjectId = 0U;
        selectionState_.selectedFeatureId = 0U;
        selectionState_.activeFeatureId = 0U;
    }
    normalizeSelectionState();
    markModelChanged(model_change_view::ChangeKind::geometry_or_transform);
    processPendingModelChange();
    update();
}

void ViewerWindow::Viewport::setSelectedObject(renderer::parametric_model::ParametricObjectId objectId) {
    if (findObjectIndexById(objectId) < 0) {
        return;
    }

    selectionState_.selectedObjectId = objectId;
    normalizeSelectionState();
}

void ViewerWindow::Viewport::setActiveObject(renderer::parametric_model::ParametricObjectId objectId) {
    if (findObjectIndexById(objectId) < 0) {
        return;
    }

    selectionState_.activeObjectId = objectId;
    normalizeSelectionState();
}

void ViewerWindow::Viewport::setSelectedFeature(renderer::parametric_model::ParametricFeatureId featureId) {
    if (!featureBelongsToObject(selectionState_.selectedObjectId, featureId)) {
        return;
    }

    selectionState_.selectedFeatureId = featureId;
    normalizeSelectionState();
}

void ViewerWindow::Viewport::setActiveFeature(renderer::parametric_model::ParametricFeatureId featureId) {
    if (!featureBelongsToObject(selectionState_.activeObjectId, featureId)) {
        return;
    }

    selectionState_.activeFeatureId = featureId;
    normalizeSelectionState();
}

void ViewerWindow::Viewport::focusSelectedObject() {
    const int objectIndex = findObjectIndexById(selectionState_.selectedObjectId);
    if (objectIndex < 0) {
        return;
    }

    updateSceneTransforms();
    applyFocusBounds(objectFocusBounds(objectIndex));
    update();
}

float ViewerWindow::Viewport::nearPlane() const {
    return cameraController_.nearPlane();
}

float ViewerWindow::Viewport::farPlane() const {
    return cameraController_.farPlane();
}

ViewerControlPanel::CameraPanelState ViewerWindow::Viewport::cameraPanelState() const {
    ViewerControlPanel::CameraPanelState state;
    state.projectionMode = projectionMode() == OrbitCameraController::ProjectionMode::orthographic ? 1 : 0;
    state.zoomMode = zoomMode() == OrbitCameraController::ZoomMode::lens ? 1 : 0;
    state.distance = cameraDistance();
    state.verticalFovDegrees = verticalFovDegrees();
    state.orthographicHeight = orthographicHeight();
    state.nearPlane = nearPlane();
    state.farPlane = farPlane();
    state.orbitCenter = orbitCenter();
    return state;
}

ViewerControlPanel::PanelState ViewerWindow::Viewport::controlPanelState() const {
    ViewerControlPanel::PanelState state;
    state.objects.resize(sceneObjects_.size());

    for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
        auto& objectState = state.objects[index];
        const auto& descriptor = sceneObjects_[index].parametricObjectDescriptor;
        objectState.id = descriptor.metadata.id;
        objectState.primitiveKind = descriptor.metadata.objectKind;
        const auto* primitive = findObjectPrimitive(descriptor);
        if (primitive != nullptr) {
            objectState.primitive = *primitive;
        }
        objectState.visible = objectVisible(index);
        objectState.rotationSpeed = objectRotationSpeed(index);
        objectState.color = objectColor(index);
        objectState.bounds = objectLocalBounds(index);

        const auto* mirrorFeature = findObjectFeature(
            descriptor,
            renderer::parametric_model::FeatureKind::mirror);
        if (mirrorFeature != nullptr) {
            objectState.mirror.enabled = mirrorFeature->enabled;
            objectState.mirror.axis = static_cast<int>(mirrorFeature->mirror.axis);
            objectState.mirror.planeOffset = mirrorFeature->mirror.planeOffset;
        }

        const auto* linearArrayFeature = findObjectFeature(
            descriptor,
            renderer::parametric_model::FeatureKind::linear_array);
        if (linearArrayFeature != nullptr) {
            objectState.linearArray.enabled = linearArrayFeature->enabled;
            objectState.linearArray.count = static_cast<int>(linearArrayFeature->linearArray.count);
            objectState.linearArray.offset = linearArrayFeature->linearArray.offset;
        }

        objectState.features.clear();
        objectState.features.reserve(descriptor.features.size());
        for (const auto& feature : descriptor.features) {
            objectState.features.push_back({
                feature.id,
                feature.kind,
                feature.enabled
            });
        }
    }

    state.lighting.ambientStrength = ambientStrength();
    state.lighting.lightDirection = lightDirection();
    state.camera = cameraPanelState();
    state.selection.selectedObjectId = selectionState_.selectedObjectId;
    state.selection.activeObjectId = selectionState_.activeObjectId;
    state.selection.selectedFeatureId = selectionState_.selectedFeatureId;
    state.selection.activeFeatureId = selectionState_.activeFeatureId;
    state.modelChangeViewStrategy =
        modelChangeViewStrategy() == model_change_view::ViewStrategy::auto_frame ? 1 : 0;
    return state;
}

viewport_zoom::AnchorSample ViewerWindow::Viewport::sampleViewportZoomAnchor(const QPointF& viewportPosition) const {
    viewport_zoom::AnchorSample sample;
    const auto ray = makeViewportRay(cameraController_, width(), height(), viewportPosition);
    sample.viewportPositionNormalized = ray.viewportPositionNormalized;
    if (!ray.valid) {
        return sample;
    }

    const auto planePoint = orbitCenter();
    const auto planeNormal = ray.cameraForward;
    if (renderer::scene_contract::math::length(planeNormal) <= kViewportZoomRayEpsilon) {
        return sample;
    }

    const float denominator = renderer::scene_contract::math::dot(ray.direction, planeNormal);
    if (std::abs(denominator) <= kViewportZoomRayEpsilon) {
        return sample;
    }

    const float distanceToPlane = renderer::scene_contract::math::dot(
        renderer::scene_contract::math::subtract(planePoint, ray.origin),
        planeNormal) / denominator;
    if (distanceToPlane <= 0.0F) {
        return sample;
    }

    sample.worldPoint = renderer::scene_contract::math::add(
        ray.origin,
        scaleVec3(ray.direction, distanceToPlane));
    sample.valid = true;
    return sample;
}

void ViewerWindow::Viewport::applyViewportZoom(const QPointF& viewportPosition, float deltaDistance) {
    const auto preZoomAnchor = sampleViewportZoomAnchor(viewportPosition);
    if (preZoomAnchor.valid) {
        viewport_zoom::begin(
            viewportZoomState_,
            preZoomAnchor.viewportPositionNormalized,
            preZoomAnchor.worldPoint);
    } else {
        viewport_zoom::reset(viewportZoomState_);
        refreshViewportZoomState();
    }

    cameraController_.zoom(deltaDistance);

    if (!preZoomAnchor.valid) {
        refreshViewportZoomState();
        return;
    }

    const auto postZoomAnchor = sampleViewportZoomAnchor(viewportPosition);
    if (postZoomAnchor.valid) {
        const auto compensation = renderer::scene_contract::math::subtract(
            preZoomAnchor.worldPoint,
            postZoomAnchor.worldPoint);
        cameraController_.setOrbitCenter(
            renderer::scene_contract::math::add(cameraController_.orbitCenter(), compensation));
    }
}

void ViewerWindow::Viewport::markModelChanged(model_change_view::ChangeKind changeKind) {
    model_change_view::markModelChanged(modelChangeViewState_, changeKind);
}

void ViewerWindow::Viewport::processPendingModelChange() {
    const auto actions = model_change_view::pendingActions(modelChangeViewState_);
    if (!actions.updateBoundsAndClip) {
        return;
    }

    updateSceneTransforms();

    const auto bounds = visibleSceneWorldBounds();
    if (actions.autoFrame) {
        applyFocusBounds(visibleFocusBounds());
        model_change_view::clearPending(modelChangeViewState_);
        return;
    }

    if (bounds.valid) {
        cameraController_.updateClipRangeForBounds(bounds);
    } else {
        cameraController_.updateClipRangeForPoint(cameraController_.orbitCenter());
    }

    model_change_view::clearPending(modelChangeViewState_);
    notifyCameraStateChanged();
}

void ViewerWindow::Viewport::setOrbitCenter(const renderer::scene_contract::Vec3f& orbitCenter) {
    cameraController_.setOrbitCenter(orbitCenter);
    refreshViewportZoomState();
    update();
}

renderer::scene_contract::Vec3f ViewerWindow::Viewport::orbitCenter() const {
    return cameraController_.orbitCenter();
}

void ViewerWindow::Viewport::focusOnPoint(const renderer::scene_contract::Vec3f& point) {
    viewport_zoom::reset(viewportZoomState_);
    cameraController_.focusOnPoint(point);
    refreshViewportZoomState();
    notifyCameraStateChanged();
    update();
}

void ViewerWindow::Viewport::focusOnScene() {
    updateSceneTransforms();
    applyFocusBounds(visibleFocusBounds());
    update();
}

renderer::scene_contract::Aabb ViewerWindow::Viewport::objectLocalBounds(int index) const {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
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
        releaseSceneObjectResources(sceneObject);
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
            uploadSceneObjectResources(sceneObject);
        }
    }

    rebuildRepositoryItems();
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
        (void)renderer_.render(framePacket_);
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
        leftButtonTracking_ = true;
        leftButtonDragged_ = false;
        mousePressPosition_ = event->pos();
        lastMousePosition_ = event->pos();
        setFocus(Qt::MouseFocusReason);
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        panning_ = true;
        rotating_ = false;
        leftButtonTracking_ = false;
        leftButtonDragged_ = false;
        lastMousePosition_ = event->pos();
        setFocus(Qt::MouseFocusReason);
        event->accept();
        return;
    }

    QOpenGLWidget::mousePressEvent(event);
}

void ViewerWindow::Viewport::mouseMoveEvent(QMouseEvent* event) {
    if (rotating_ && leftButtonTracking_ && (event->buttons() & Qt::LeftButton)) {
        if (!leftButtonDragged_) {
            const QPoint totalDelta = event->pos() - mousePressPosition_;
            if (totalDelta.manhattanLength() <= kViewportClickDragThresholdPixels) {
                event->accept();
                return;
            }

            leftButtonDragged_ = true;
        }

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
            refreshViewportZoomState();
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
        const QPoint totalDelta = event->pos() - mousePressPosition_;
        const bool clickSelection =
            leftButtonTracking_
            && !leftButtonDragged_
            && totalDelta.manhattanLength() <= kViewportClickDragThresholdPixels;

        rotating_ = false;
        leftButtonTracking_ = false;
        leftButtonDragged_ = false;

        if (clickSelection) {
            const int pickedIndex = pickObjectAt(event->localPos());
            if (pickedIndex >= 0) {
                selectObjectAt(pickedIndex);
            }
        }

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
    leftButtonTracking_ = false;
    leftButtonDragged_ = false;
    QOpenGLWidget::leaveEvent(event);
}

void ViewerWindow::Viewport::focusOutEvent(QFocusEvent* event) {
    rotating_ = false;
    panning_ = false;
    leftButtonTracking_ = false;
    leftButtonDragged_ = false;
    QOpenGLWidget::focusOutEvent(event);
}

void ViewerWindow::Viewport::wheelEvent(QWheelEvent* event) {
    const QPoint angleDelta = event->angleDelta();
    if (angleDelta.y() == 0) {
        QOpenGLWidget::wheelEvent(event);
        return;
    }

    const float zoomDelta = -static_cast<float>(angleDelta.y()) / 120.0F * kWheelZoomStep;
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        applyViewportZoom(event->position(), zoomDelta);
    } else {
        cameraController_.zoom(zoomDelta);
        refreshViewportZoomState();
    }
    updateSceneTransforms();
    const auto bounds = visibleFocusBounds();
    if (bounds.valid) {
        cameraController_.updateClipRangeForBounds(bounds);
    } else {
        cameraController_.updateClipRangeForPoint(cameraController_.orbitCenter());
    }
    notifyCameraStateChanged();
    update();
    event->accept();
}

renderer::scene_contract::TransformData ViewerWindow::Viewport::currentObjectTransform(int index) const {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return {};
    }

    const auto& sceneObject = sceneObjects_[index];
    const float elapsedSeconds = static_cast<float>(animationClock_.elapsed()) / 1000.0F;
    return makeSceneTransform(
        elapsedSeconds,
        sceneObject.offsetX,
        sceneObject.offsetY,
        sceneObject.offsetZ,
        sceneObject.scale,
        sceneObject.rotationSpeed);
}

renderer::scene_contract::RenderableVisualState ViewerWindow::Viewport::currentObjectVisualState(int index) const {
    renderer::scene_contract::RenderableVisualState visualState;
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return visualState;
    }

    const auto objectId = sceneObjects_[index].parametricObjectDescriptor.metadata.id;
    if (objectId != 0U && objectId == selectionState_.activeObjectId) {
        visualState.interaction = renderer::scene_contract::InteractionVisualState::active;
    } else if (objectId != 0U && objectId == selectionState_.selectedObjectId) {
        visualState.interaction = renderer::scene_contract::InteractionVisualState::selected;
    }
    return visualState;
}

void ViewerWindow::Viewport::notifyCameraStateChanged() {
    if (cameraStateChangedCallback_) {
        cameraStateChangedCallback_();
    }
}

void ViewerWindow::Viewport::refreshViewportZoomState() {
    viewportZoomState_.zoomOperationKind = viewport_zoom::chooseZoomOperationKind(
        projectionMode() == OrbitCameraController::ProjectionMode::orthographic,
        zoomMode() == OrbitCameraController::ZoomMode::lens);
}

void ViewerWindow::Viewport::releaseSceneObjectResources(SceneObject& sceneObject) {
    if (!renderer_.isInitialized()) {
        sceneObject.meshHandle = renderer::scene_contract::kInvalidMeshHandle;
        sceneObject.materialHandle = renderer::scene_contract::kInvalidMaterialHandle;
        sceneObject.textureHandle = renderer::scene_contract::kInvalidTextureHandle;
        sceneObject.materialData.baseColorTexture = renderer::scene_contract::kInvalidTextureHandle;
        sceneObject.materialData.useBaseColorTexture = false;
        return;
    }

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
    sceneObject.materialData.baseColorTexture = renderer::scene_contract::kInvalidTextureHandle;
    sceneObject.materialData.useBaseColorTexture = false;
}

void ViewerWindow::Viewport::uploadSceneObjectResources(SceneObject& sceneObject) {
    if (!renderer_.isInitialized()) {
        return;
    }

    sceneObject.textureHandle = renderer::scene_contract::kInvalidTextureHandle;
    sceneObject.materialData.baseColorTexture = renderer::scene_contract::kInvalidTextureHandle;
    sceneObject.materialData.useBaseColorTexture = false;

    if (!sceneObject.textureData.pixels.empty()) {
        sceneObject.textureHandle = renderer_.uploadTexture(sceneObject.textureData);
        if (sceneObject.textureHandle == renderer::scene_contract::kInvalidTextureHandle) {
            qWarning() << "Failed to upload scene texture:" << QString::fromStdString(renderer_.lastError());
        } else {
            sceneObject.materialData.baseColorTexture = sceneObject.textureHandle;
            sceneObject.materialData.useBaseColorTexture = true;
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

void ViewerWindow::Viewport::rebuildRepositoryItems() {
    repository_.clear();
    for (auto& sceneObject : sceneObjects_) {
        auto item = makeItem();
        item.meshHandle = sceneObject.meshHandle;
        item.materialHandle = sceneObject.materialHandle;
        item.visible = sceneObject.visible;
        sceneObject.itemId = repository_.add(item);
        repository_.updateLocalBounds(sceneObject.itemId, sceneObject.meshData.localBounds);
    }
    updateSceneTransforms();
}

void ViewerWindow::Viewport::applyFocusBounds(const renderer::scene_contract::Aabb& bounds) {
    if (bounds.valid) {
        cameraController_.focusOnBounds(bounds);
    } else {
        cameraController_.updateClipRangeForPoint(cameraController_.orbitCenter());
    }
    viewport_zoom::reset(viewportZoomState_);
    refreshViewportZoomState();
    notifyCameraStateChanged();
}

renderer::parametric_model::FeatureDescriptor* ViewerWindow::Viewport::ensureObjectFeature(
    renderer::parametric_model::ParametricObjectDescriptor& descriptor,
    renderer::parametric_model::FeatureKind kind)
{
    for (auto& feature : descriptor.features) {
        if (feature.kind == kind) {
            return &feature;
        }
    }

    if (kind == renderer::parametric_model::FeatureKind::primitive) {
        descriptor.features.insert(
            descriptor.features.begin(),
            renderer::parametric_model::PrimitiveFactory::makePrimitiveFeature({}));
        return &descriptor.features.front();
    }

    if (kind == renderer::parametric_model::FeatureKind::mirror) {
        descriptor.features.push_back(renderer::parametric_model::PrimitiveFactory::makeMirrorFeature());
        return &descriptor.features.back();
    }

    if (kind == renderer::parametric_model::FeatureKind::linear_array) {
        descriptor.features.push_back(renderer::parametric_model::PrimitiveFactory::makeLinearArrayFeature());
        return &descriptor.features.back();
    }

    return nullptr;
}

const renderer::parametric_model::FeatureDescriptor* ViewerWindow::Viewport::findObjectFeature(
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
    renderer::parametric_model::FeatureKind kind) const
{
    for (const auto& feature : descriptor.features) {
        if (feature.kind == kind) {
            return &feature;
        }
    }
    return nullptr;
}

renderer::parametric_model::FeatureDescriptor* ViewerWindow::Viewport::findObjectFeatureById(
    renderer::parametric_model::ParametricObjectDescriptor& descriptor,
    renderer::parametric_model::ParametricFeatureId featureId)
{
    for (auto& feature : descriptor.features) {
        if (feature.id == featureId) {
            return &feature;
        }
    }
    return nullptr;
}

const renderer::parametric_model::FeatureDescriptor* ViewerWindow::Viewport::findObjectFeatureById(
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
    renderer::parametric_model::ParametricFeatureId featureId) const
{
    for (const auto& feature : descriptor.features) {
        if (feature.id == featureId) {
            return &feature;
        }
    }
    return nullptr;
}

renderer::parametric_model::PrimitiveDescriptor* ViewerWindow::Viewport::ensureObjectPrimitive(
    renderer::parametric_model::ParametricObjectDescriptor& descriptor)
{
    auto* feature = ensureObjectFeature(descriptor, renderer::parametric_model::FeatureKind::primitive);
    if (feature == nullptr) {
        return nullptr;
    }
    return &feature->primitive;
}

const renderer::parametric_model::PrimitiveDescriptor* ViewerWindow::Viewport::findObjectPrimitive(
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor) const
{
    const auto* feature = findObjectFeature(descriptor, renderer::parametric_model::FeatureKind::primitive);
    if (feature == nullptr) {
        return nullptr;
    }
    return &feature->primitive;
}

int ViewerWindow::Viewport::findObjectIndexById(renderer::parametric_model::ParametricObjectId objectId) const {
    for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
        if (sceneObjects_[index].parametricObjectDescriptor.metadata.id == objectId) {
            return index;
        }
    }
    return -1;
}

int ViewerWindow::Viewport::pickObjectAt(const QPointF& viewportPosition) {
    const auto ray = makeViewportRay(cameraController_, width(), height(), viewportPosition);
    if (!ray.valid) {
        return -1;
    }

    int pickedIndex = -1;
    float pickedDistance = std::numeric_limits<float>::max();
    for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
        const auto& sceneObject = sceneObjects_[index];
        const auto transform = currentObjectTransform(index);
        repository_.updateVisible(sceneObject.itemId, sceneObject.visible);
        repository_.updateVisualState(sceneObject.itemId, currentObjectVisualState(index));
        repository_.updateTransform(sceneObject.itemId, transform);

        if (!sceneObject.visible) {
            continue;
        }

        float hitDistance = 0.0F;
        const auto bounds = repository_.rangeData(sceneObject.itemId).worldBounds;
        if (!intersectRayAabb(ray.origin, ray.direction, bounds, hitDistance)) {
            continue;
        }

        if (!intersectRayMesh(
                ray.origin,
                ray.direction,
                sceneObject.meshData,
                transform.world,
                hitDistance)) {
            continue;
        }

        if (hitDistance < pickedDistance) {
            pickedDistance = hitDistance;
            pickedIndex = index;
        }
    }

    return pickedIndex;
}

void ViewerWindow::Viewport::selectObjectAt(int index) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    const auto objectId = sceneObjects_[index].parametricObjectDescriptor.metadata.id;
    selectionState_.selectedObjectId = objectId;
    selectionState_.activeObjectId = objectId;
    normalizeSelectionState();
    notifyCameraStateChanged();
    update();
}

bool ViewerWindow::Viewport::featureBelongsToObject(
    renderer::parametric_model::ParametricObjectId objectId,
    renderer::parametric_model::ParametricFeatureId featureId) const
{
    const int objectIndex = findObjectIndexById(objectId);
    if (objectIndex < 0) {
        return false;
    }

    return findObjectFeatureById(sceneObjects_[objectIndex].parametricObjectDescriptor, featureId) != nullptr;
}

renderer::parametric_model::ParametricFeatureId ViewerWindow::Viewport::firstFeatureIdForObject(
    renderer::parametric_model::ParametricObjectId objectId) const
{
    const int objectIndex = findObjectIndexById(objectId);
    if (objectIndex < 0) {
        return 0U;
    }

    const auto& features = sceneObjects_[objectIndex].parametricObjectDescriptor.features;
    if (features.empty()) {
        return 0U;
    }
    return features.front().id;
}

void ViewerWindow::Viewport::normalizeSelectionState() {
    if (sceneObjects_.empty()) {
        selectionState_.selectedObjectId = 0U;
        selectionState_.activeObjectId = 0U;
        selectionState_.selectedFeatureId = 0U;
        selectionState_.activeFeatureId = 0U;
        return;
    }

    if (selectionState_.selectedObjectId == 0U || findObjectIndexById(selectionState_.selectedObjectId) < 0) {
        selectionState_.selectedObjectId = sceneObjects_.front().parametricObjectDescriptor.metadata.id;
    }

    if (selectionState_.activeObjectId == 0U || findObjectIndexById(selectionState_.activeObjectId) < 0) {
        selectionState_.activeObjectId = selectionState_.selectedObjectId;
    }

    if (!featureBelongsToObject(selectionState_.selectedObjectId, selectionState_.selectedFeatureId)) {
        selectionState_.selectedFeatureId = firstFeatureIdForObject(selectionState_.selectedObjectId);
    }

    if (!featureBelongsToObject(selectionState_.activeObjectId, selectionState_.activeFeatureId)) {
        if (featureBelongsToObject(selectionState_.activeObjectId, selectionState_.selectedFeatureId)) {
            selectionState_.activeFeatureId = selectionState_.selectedFeatureId;
        } else {
            selectionState_.activeFeatureId = firstFeatureIdForObject(selectionState_.activeObjectId);
        }
    }
}

void ViewerWindow::Viewport::applyParametricObjectDescriptor(
    int index,
    const renderer::parametric_model::ParametricObjectDescriptor& descriptor)
{
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    sceneObjects_[index].parametricObjectDescriptor = descriptor;
    normalizeSelectionState();
    rebuildObjectMesh(index);
    markModelChanged(model_change_view::ChangeKind::geometry_or_transform);
    processPendingModelChange();
    update();
}

void ViewerWindow::Viewport::rebuildObjectMesh(int index) {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return;
    }

    auto& sceneObject = sceneObjects_[index];
    sceneObject.meshData = renderer::parametric_model::PrimitiveFactory::build(sceneObject.parametricObjectDescriptor);

    if (renderer_.isInitialized()) {
        bool meshReady = false;
        if (sceneObject.meshHandle != renderer::scene_contract::kInvalidMeshHandle) {
            meshReady = renderer_.updateMesh(sceneObject.meshHandle, sceneObject.meshData);
            if (!meshReady) {
                qWarning() << "Failed to update scene mesh, retrying upload:" << QString::fromStdString(renderer_.lastError());
                renderer_.releaseMesh(sceneObject.meshHandle);
                sceneObject.meshHandle = renderer::scene_contract::kInvalidMeshHandle;
            }
        }

        if (!meshReady) {
            sceneObject.meshHandle = renderer_.uploadMesh(sceneObject.meshData);
            if (sceneObject.meshHandle == renderer::scene_contract::kInvalidMeshHandle) {
                qWarning() << "Failed to upload rebuilt scene mesh:" << QString::fromStdString(renderer_.lastError());
            } else {
                repository_.updateMeshHandle(sceneObject.itemId, sceneObject.meshHandle);
            }
        }
    }

    repository_.updateLocalBounds(sceneObject.itemId, sceneObject.meshData.localBounds);
}

void ViewerWindow::Viewport::updateSceneTransforms() {
    for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
        auto& sceneObject = sceneObjects_[index];
        repository_.updateVisible(sceneObject.itemId, sceneObject.visible);
        repository_.updateVisualState(sceneObject.itemId, currentObjectVisualState(index));
        repository_.updateTransform(
            sceneObject.itemId,
            currentObjectTransform(index));
    }
}

renderer::scene_contract::Aabb ViewerWindow::Viewport::visibleFocusBounds() const {
    renderer::scene_contract::Aabb bounds;
    for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
        if (!sceneObjects_[index].visible) {
            continue;
        }
        bounds = camera_focus_bounds::mergeFocusBounds(bounds, objectFocusBounds(index));
    }
    return bounds;
}

renderer::scene_contract::Aabb ViewerWindow::Viewport::visibleSceneWorldBounds() const {
    renderer::scene_contract::Aabb bounds;

    for (const auto& sceneObject : sceneObjects_) {
        if (!sceneObject.visible) {
            continue;
        }
        bounds = mergeAabb(bounds, repository_.rangeData(sceneObject.itemId).worldBounds);
    }

    return bounds;
}

renderer::scene_contract::Aabb ViewerWindow::Viewport::objectFocusBounds(int index) const {
    if (index < 0 || index >= static_cast<int>(sceneObjects_.size())) {
        return {};
    }

    const auto& sceneObject = sceneObjects_[index];
    const auto& localBounds = sceneObject.meshData.localBounds;
    if (!localBounds.valid) {
        return {};
    }

    return camera_focus_bounds::makeStableFocusBounds(localBounds, currentObjectTransform(index));
}

void ViewerWindow::Viewport::rebuildFramePacket() {
    updateSceneTransforms();

    auto scene = repository_.snapshot(cameraController_.buildCameraData());
    scene.light = light_;
    framePacket_ = assembler_.build(scene, {width(), height()});
}

void ViewerWindow::syncControlPanel() {
    if (controlPanel_ == nullptr || viewport_ == nullptr) {
        return;
    }

    controlPanel_->setPanelState(viewport_->controlPanelState());
}
