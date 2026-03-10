#pragma once

#include <cstddef>
#include <vector>

#include "renderer/scene_contract/types.h"

namespace renderer::render_core {

class SceneRepository {
public:
    using ItemId = std::size_t;

    ItemId add(scene_contract::RenderableItem item);
    void clear();
    void updateMeshHandle(ItemId id, scene_contract::MeshHandle meshHandle);
    void updateMaterialHandle(ItemId id, scene_contract::MaterialHandle materialHandle);
    void updateTransform(ItemId id, const scene_contract::TransformData& transform);

    [[nodiscard]] scene_contract::FrameScene snapshot(
        const scene_contract::CameraData& camera) const;

private:
    std::vector<scene_contract::RenderableItem> items_;
};

}  // namespace renderer::render_core
