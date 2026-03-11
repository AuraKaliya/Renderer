#include "renderer/render_core/scene_repository.h"

#include <utility>

namespace renderer::render_core {

SceneRepository::ItemId SceneRepository::add(scene_contract::RenderableItem item) {
    ItemRecord record;
    record.item = std::move(item);
    items_.push_back(std::move(record));
    return items_.size() - 1U;
}

void SceneRepository::clear() {
    items_.clear();
}

void SceneRepository::updateMeshHandle(ItemId id, scene_contract::MeshHandle meshHandle) {
    if (id >= items_.size()) {
        return;
    }

    items_[id].item.meshHandle = meshHandle;
}



void SceneRepository::updateMaterialHandle(
    ItemId id,
    scene_contract::MaterialHandle materialHandle) {
    if (id >= items_.size()) {
        return;
    }

    items_[id].item.materialHandle = materialHandle;
}

void SceneRepository::updateTransform(
    ItemId id,
    const scene_contract::TransformData& transform) {
    if (id >= items_.size()) {
        return;
    }

    items_[id].item.transform = transform;
}

void SceneRepository::updateLocalBounds(
    ItemId id,
    const scene_contract::Aabb& localBounds) {
    if (id >= items_.size()) {
        return;
    }

    items_[id].rangeData.localBounds = localBounds;
}

ItemRangeData SceneRepository::rangeData(ItemId id) const {
    if (id >= items_.size()) {
        return {};
    }

    return items_[id].rangeData;
}

std::vector<ItemRangeData> SceneRepository::snapshotRangeData() const {
    std::vector<ItemRangeData> ranges;
    ranges.reserve(items_.size());

    for (const auto& item : items_) {
        ranges.push_back(item.rangeData);
    }

    return ranges;
}

scene_contract::FrameScene SceneRepository::snapshot(
    const scene_contract::CameraData& camera) const {
    scene_contract::FrameScene scene;
    scene.camera = camera;
    scene.items.reserve(items_.size());
    for (const auto& item : items_) {
        scene.items.push_back(item.item);
    }
    return scene;
}

}  // namespace renderer::render_core
