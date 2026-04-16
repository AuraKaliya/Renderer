#pragma once

#include <functional>
#include <memory>
#include <vector>

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
class QPointF;
class QPainter;

#include "orbit_camera_controller.h"
#include "model_change_view_state.h"
#include "viewport_zoom_state.h"
#include "viewer_control_panel.h"
#include "renderer/parametric_model/primitive_factory.h"
#include "renderer/render_core/frame_assembler.h"
#include "renderer/render_core/scene_repository.h"
#include "renderer/scene_contract/types.h"
#include "renderer/render_gl/gl_renderer.h"

class ViewerWindow final : public QMainWindow {
public:
    ViewerWindow();

private:
    void bindControlPanelSignals();
    class Viewport final : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    public:
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
        void setModelChangeViewStrategy(model_change_view::ViewStrategy strategy);
        [[nodiscard]] model_change_view::ViewStrategy modelChangeViewStrategy() const;
        void setVerticalFovDegrees(float degrees);
        [[nodiscard]] float verticalFovDegrees() const;
        void setOrthographicHeight(float height);
        [[nodiscard]] float orthographicHeight() const;
        void setBoxConstructionMode(renderer::parametric_model::BoxSpec::ConstructionMode mode);
        void setBoxWidth(float width);
        void setBoxHeight(float height);
        void setBoxDepth(float depth);
        void setBoxCenter(const renderer::scene_contract::Vec3f& center);
        void setBoxCornerPoint(const renderer::scene_contract::Vec3f& cornerPoint);
        void setCylinderConstructionMode(renderer::parametric_model::CylinderSpec::ConstructionMode mode);
        void setCylinderRadius(float radius);
        void setCylinderHeight(float height);
        void setCylinderSegments(std::uint32_t segments);
        void setCylinderCenter(const renderer::scene_contract::Vec3f& center);
        void setCylinderRadiusPoint(const renderer::scene_contract::Vec3f& radiusPoint);
        void setSphereRadius(float radius);
        void setSphereSlices(std::uint32_t slices);
        void setSphereStacks(std::uint32_t stacks);
        void setSphereConstructionMode(renderer::parametric_model::SphereSpec::ConstructionMode mode);
        void setSphereCenter(const renderer::scene_contract::Vec3f& center);
        void setSphereSurfacePoint(const renderer::scene_contract::Vec3f& surfacePoint);
        void setParametricOverlayModelBoundsVisible(bool visible);
        void setParametricOverlayNodePointsVisible(bool visible);
        void setParametricOverlayConstructionLinksVisible(bool visible);
        void setObjectMirrorEnabled(int index, bool enabled);
        void setObjectMirrorAxis(int index, renderer::parametric_model::Axis axis);
        void setObjectMirrorPlaneOffset(int index, float planeOffset);
        void setObjectLinearArrayEnabled(int index, bool enabled);
        void setObjectLinearArrayCount(int index, std::uint32_t count);
        void setObjectLinearArrayOffset(int index, const renderer::scene_contract::Vec3f& offset);
        void addObjectFeature(int index, renderer::parametric_model::FeatureKind kind);
        void removeObjectFeature(int index, renderer::parametric_model::ParametricFeatureId featureId);
        void setObjectFeatureEnabled(
            int index,
            renderer::parametric_model::ParametricFeatureId featureId,
            bool enabled);
        void setObjectNodePosition(
            int index,
            renderer::parametric_model::ParametricNodeId nodeId,
            const renderer::scene_contract::Vec3f& position);
        void addObject(renderer::parametric_model::PrimitiveKind kind);
        void removeSelectedObject();
        void setSelectedObject(renderer::parametric_model::ParametricObjectId objectId);
        void setActiveObject(renderer::parametric_model::ParametricObjectId objectId);
        void setSelectedFeature(renderer::parametric_model::ParametricFeatureId featureId);
        void setActiveFeature(renderer::parametric_model::ParametricFeatureId featureId);
        void focusSelectedObject();
        [[nodiscard]] float nearPlane() const;
        [[nodiscard]] float farPlane() const;
        [[nodiscard]] ViewerControlPanel::CameraPanelState cameraPanelState() const;
        [[nodiscard]] ViewerControlPanel::PanelState controlPanelState() const;

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
        struct SceneObject;

        [[nodiscard]] renderer::scene_contract::TransformData currentObjectTransform(int index) const;
        [[nodiscard]] renderer::scene_contract::RenderableVisualState currentObjectVisualState(int index) const;
        [[nodiscard]] viewport_zoom::AnchorSample sampleViewportZoomAnchor(const QPointF& viewportPosition) const;
        void applyViewportZoom(const QPointF& viewportPosition, float deltaDistance);
        void markModelChanged(model_change_view::ChangeKind changeKind);
        void processPendingModelChange();
        void notifyCameraStateChanged();
        void refreshViewportZoomState();
        void releaseSceneObjectResources(SceneObject& sceneObject);
        void uploadSceneObjectResources(SceneObject& sceneObject);
        void rebuildRepositoryItems();
        void applyFocusBounds(const renderer::scene_contract::Aabb& bounds);
        void applyParametricObjectDescriptor(
            int index,
            const renderer::parametric_model::ParametricObjectDescriptor& descriptor);
        void rebuildObjectMesh(int index);
        void updateSceneTransforms();
        renderer::parametric_model::FeatureDescriptor* ensureObjectFeature(
            renderer::parametric_model::ParametricObjectDescriptor& descriptor,
            renderer::parametric_model::FeatureKind kind);
        [[nodiscard]] const renderer::parametric_model::FeatureDescriptor* findObjectFeature(
            const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
            renderer::parametric_model::FeatureKind kind) const;
        renderer::parametric_model::FeatureDescriptor* findObjectFeatureById(
            renderer::parametric_model::ParametricObjectDescriptor& descriptor,
            renderer::parametric_model::ParametricFeatureId featureId);
        [[nodiscard]] const renderer::parametric_model::FeatureDescriptor* findObjectFeatureById(
            const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
            renderer::parametric_model::ParametricFeatureId featureId) const;
        renderer::parametric_model::PrimitiveDescriptor* ensureObjectPrimitive(
            renderer::parametric_model::ParametricObjectDescriptor& descriptor);
        [[nodiscard]] const renderer::parametric_model::PrimitiveDescriptor* findObjectPrimitive(
            const renderer::parametric_model::ParametricObjectDescriptor& descriptor) const;
        renderer::parametric_model::ParametricNodeDescriptor* findPointNodeById(
            renderer::parametric_model::ParametricObjectDescriptor& descriptor,
            renderer::parametric_model::ParametricNodeId nodeId);
        [[nodiscard]] const renderer::parametric_model::ParametricNodeDescriptor* findPointNodeById(
            const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
            renderer::parametric_model::ParametricNodeId nodeId) const;
        [[nodiscard]] renderer::scene_contract::Vec3f resolvePointNode(
            const renderer::parametric_model::ParametricObjectDescriptor& descriptor,
            renderer::parametric_model::NodeReference reference,
            const renderer::scene_contract::Vec3f& fallback) const;
        renderer::parametric_model::NodeReference ensurePointNode(
            renderer::parametric_model::ParametricObjectDescriptor& descriptor,
            renderer::parametric_model::NodeReference reference,
            const renderer::scene_contract::Vec3f& fallback);
        [[nodiscard]] int findObjectIndexById(renderer::parametric_model::ParametricObjectId objectId) const;
        [[nodiscard]] int pickObjectAt(const QPointF& viewportPosition);
        void selectObjectAt(int index);
        [[nodiscard]] bool featureBelongsToObject(
            renderer::parametric_model::ParametricObjectId objectId,
            renderer::parametric_model::ParametricFeatureId featureId) const;
        [[nodiscard]] renderer::parametric_model::ParametricFeatureId firstFeatureIdForObject(
            renderer::parametric_model::ParametricObjectId objectId) const;
        void normalizeSelectionState();
        [[nodiscard]] renderer::scene_contract::Aabb objectFocusBounds(int index) const;
        [[nodiscard]] renderer::scene_contract::Aabb visibleFocusBounds() const;
        [[nodiscard]] renderer::scene_contract::Aabb visibleSceneWorldBounds() const;
        void paintParametricOverlay();
        void paintParametricBoundsOverlay(
            QPainter& painter,
            const renderer::scene_contract::CameraData& camera,
            const SceneObject& sceneObject,
            const renderer::scene_contract::TransformData& transform) const;
        void paintParametricConstructionLinksOverlay(
            QPainter& painter,
            const renderer::scene_contract::CameraData& camera,
            const SceneObject& sceneObject,
            const renderer::scene_contract::TransformData& transform,
            const std::vector<renderer::parametric_model::ParametricConstructionLinkDescriptor>& links) const;
        void paintParametricNodePointsOverlay(
            QPainter& painter,
            const renderer::scene_contract::CameraData& camera,
            const SceneObject& sceneObject,
            const renderer::scene_contract::TransformData& transform,
            const std::vector<renderer::parametric_model::ParametricNodeUsageDescriptor>& nodeUsages) const;
        void rebuildFramePacket();

        struct SceneObject {
            renderer::render_core::SceneRepository::ItemId itemId = 0;
            renderer::scene_contract::MeshHandle meshHandle = renderer::scene_contract::kInvalidMeshHandle;
            renderer::scene_contract::MaterialHandle materialHandle = renderer::scene_contract::kInvalidMaterialHandle;
            renderer::scene_contract::TextureHandle textureHandle = renderer::scene_contract::kInvalidTextureHandle;
            renderer::parametric_model::ParametricObjectDescriptor parametricObjectDescriptor {};
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

        struct SelectionState {
            renderer::parametric_model::ParametricObjectId selectedObjectId = 0U;
            renderer::parametric_model::ParametricObjectId activeObjectId = 0U;
            renderer::parametric_model::ParametricFeatureId selectedFeatureId = 0U;
            renderer::parametric_model::ParametricFeatureId activeFeatureId = 0U;
        };

        std::vector<SceneObject> sceneObjects_;
        SelectionState selectionState_ {};
        renderer::scene_contract::DirectionalLightData light_ {};
        OrbitCameraController cameraController_;
        renderer::render_core::SceneRepository repository_;
        renderer::render_core::FrameAssembler assembler_;
        renderer::render_core::FramePacket framePacket_;
        renderer::render_gl::GlRenderer renderer_;
        model_change_view::State modelChangeViewState_ {};
        viewport_zoom::State viewportZoomState_ {};
        std::unique_ptr<QOpenGLFramebufferObject> offscreenTarget_;
        std::function<void()> cameraStateChangedCallback_;
        QPoint lastMousePosition_;
        QPoint mousePressPosition_;
        bool leftButtonTracking_ = false;
        bool leftButtonDragged_ = false;
        bool rotating_ = false;
        bool panning_ = false;
        bool showParametricModelBounds_ = true;
        bool showParametricNodePoints_ = true;
        bool showParametricConstructionLinks_ = true;
        QTimer frameTimer_;
        QElapsedTimer animationClock_;
    };
    void syncControlPanel();

    Viewport* viewport_ = nullptr;
    ViewerControlPanel* controlPanel_ = nullptr;
};
