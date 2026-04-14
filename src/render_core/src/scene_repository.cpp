#include "renderer/render_core/scene_repository.h"

#include <algorithm>
#include <utility>

namespace renderer::render_core {

namespace {

scene_contract::Vec3f transformPoint(
    const scene_contract::Mat4f& matrix,
    const scene_contract::Vec3f& point)
{
    return {
        matrix.elements[0] * point.x + matrix.elements[4] * point.y + matrix.elements[8] * point.z + matrix.elements[12],
        matrix.elements[1] * point.x + matrix.elements[5] * point.y + matrix.elements[9] * point.z + matrix.elements[13],
        matrix.elements[2] * point.x + matrix.elements[6] * point.y + matrix.elements[10] * point.z + matrix.elements[14]
    };
}

scene_contract::Aabb mergeAabb(const scene_contract::Aabb& left, const scene_contract::Aabb& right) {
    if (!left.valid) {
        return right;
    }
    if (!right.valid) {
        return left;
    }

    scene_contract::Aabb merged;
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

scene_contract::Aabb transformAabb(
    const scene_contract::Aabb& localBounds,
    const scene_contract::TransformData& transform)
{
    if (!localBounds.valid) {
        return {};
    }

    const scene_contract::Vec3f corners[8] = {
        {localBounds.min.x, localBounds.min.y, localBounds.min.z},
        {localBounds.max.x, localBounds.min.y, localBounds.min.z},
        {localBounds.min.x, localBounds.max.y, localBounds.min.z},
        {localBounds.max.x, localBounds.max.y, localBounds.min.z},
        {localBounds.min.x, localBounds.min.y, localBounds.max.z},
        {localBounds.max.x, localBounds.min.y, localBounds.max.z},
        {localBounds.min.x, localBounds.max.y, localBounds.max.z},
        {localBounds.max.x, localBounds.max.y, localBounds.max.z}
    };

    scene_contract::Aabb bounds;
    bounds.min = transformPoint(transform.world, corners[0]);
    bounds.max = bounds.min;
    bounds.valid = true;

    for (int index = 1; index < 8; ++index) {
        const auto point = transformPoint(transform.world, corners[index]);
        bounds.min.x = std::min(bounds.min.x, point.x);
        bounds.min.y = std::min(bounds.min.y, point.y);
        bounds.min.z = std::min(bounds.min.z, point.z);
        bounds.max.x = std::max(bounds.max.x, point.x);
        bounds.max.y = std::max(bounds.max.y, point.y);
        bounds.max.z = std::max(bounds.max.z, point.z);
    }

    return bounds;
}

}  // namespace

SceneRepository::ItemId SceneRepository::add(scene_contract::RenderableItem item) {
    ItemRecord record;
    record.item = std::move(item);
    record.rangeData.worldBounds = transformAabb(record.rangeData.localBounds, record.item.transform);
    record.item.worldBounds = record.rangeData.worldBounds;
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
    items_[id].rangeData.worldBounds = transformAabb(items_[id].rangeData.localBounds, items_[id].item.transform);
    items_[id].item.worldBounds = items_[id].rangeData.worldBounds;
}

void SceneRepository::updateLocalBounds(
    ItemId id,
    const scene_contract::Aabb& localBounds) {
    if (id >= items_.size()) {
        return;
    }

    items_[id].rangeData.localBounds = localBounds;
    items_[id].rangeData.worldBounds = transformAabb(items_[id].rangeData.localBounds, items_[id].item.transform);
    items_[id].item.worldBounds = items_[id].rangeData.worldBounds;
}

void SceneRepository::updateVisible(ItemId id, bool visible) {
    if (id >= items_.size()) {
        return;
    }

    items_[id].item.visible = visible;
}

void SceneRepository::updateVisualState(
    ItemId id,
    const scene_contract::RenderableVisualState& visualState)
{
    if (id >= items_.size()) {
        return;
    }

    items_[id].item.visual = visualState;
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

SceneRangeData SceneRepository::sceneRangeData() const {
    SceneRangeData sceneRange;

    for (const auto& item : items_) {
        sceneRange.worldBounds = mergeAabb(sceneRange.worldBounds, item.rangeData.worldBounds);
    }

    return sceneRange;
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
