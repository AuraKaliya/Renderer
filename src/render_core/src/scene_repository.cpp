#include "renderer/render_core/scene_repository.h"

#include <utility>

namespace renderer::render_core {

SceneRepository::ItemId SceneRepository::add(scene_contract::RenderableItem item) {
    items_.push_back(std::move(item));
    return items_.size() - 1U;
}

void SceneRepository::clear() {
    items_.clear();
}

void SceneRepository::updateMeshHandle(
    ItemId id,
    scene_contract::MeshHandle meshHandle) {
    if (id >= items_.size()) {
        return;
    }

    items_[id].meshHandle = meshHandle;
}

void SceneRepository::updateMaterialHandle(
    ItemId id,
    scene_contract::MaterialHandle materialHandle) {
    if (id >= items_.size()) {
        return;
    }

    items_[id].materialHandle = materialHandle;
}

void SceneRepository::updateTransform(
    ItemId id,
    const scene_contract::TransformData& transform) {
    if (id >= items_.size()) {
        return;
    }

    items_[id].transform = transform;
}

scene_contract::FrameScene SceneRepository::snapshot(
    const scene_contract::CameraData& camera) const {
    scene_contract::FrameScene scene;
    scene.camera = camera;
    scene.items = items_;
    return scene;
}

}  // namespace renderer::render_core
