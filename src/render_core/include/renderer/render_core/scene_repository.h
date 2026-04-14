#pragma once

#include <cstddef>
#include <vector>

#include "renderer/scene_contract/types.h"

namespace renderer::render_core {

struct ItemRangeData {
    scene_contract::Aabb localBounds {};
    scene_contract::Aabb worldBounds {};
};

struct SceneRangeData {
    scene_contract::Aabb worldBounds {};
};

class SceneRepository {
public:
    using ItemId = std::size_t;

    ItemId add(scene_contract::RenderableItem item);
    void clear();
    void updateMeshHandle(ItemId id, scene_contract::MeshHandle meshHandle);
    void updateMaterialHandle(ItemId id, scene_contract::MaterialHandle materialHandle);
    void updateTransform(ItemId id, const scene_contract::TransformData& transform);
    void updateLocalBounds(ItemId id, const scene_contract::Aabb& localBounds);
    void updateVisible(ItemId id, bool visible);
    void updateVisualState(ItemId id, const scene_contract::RenderableVisualState& visualState);

    [[nodiscard]] ItemRangeData rangeData(ItemId id) const;
    [[nodiscard]] std::vector<ItemRangeData> snapshotRangeData() const;
    [[nodiscard]] SceneRangeData sceneRangeData() const;

    [[nodiscard]] scene_contract::FrameScene snapshot(
        const scene_contract::CameraData& camera) const;

private:
    struct ItemRecord {
        scene_contract::RenderableItem item;
        ItemRangeData rangeData {};
    };

    std::vector<ItemRecord> items_;
};

}  // namespace renderer::render_core
