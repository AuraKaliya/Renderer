#pragma once

#include <vector>

#include "renderer/scene_contract/types.h"

namespace renderer::render_core {

struct FrameBuildOptions {
    bool enableFrustumCulling = true;
    bool sortOpaqueFrontToBack = true;
};

struct RenderQueueItem {
    scene_contract::RenderableItem item;
    float cameraDistanceSquared = 0.0F;
};

struct FramePacket {
    scene_contract::CameraData camera;
    scene_contract::DirectionalLightData light {};
    std::vector<RenderQueueItem> opaqueItems;
    scene_contract::RenderTargetDesc target {};
};

class FrameAssembler {
public:
    [[nodiscard]] FramePacket build(
        const scene_contract::FrameScene& scene,
        const scene_contract::RenderTargetDesc& target,
        FrameBuildOptions options = {}) const;
};

}  // namespace renderer::render_core
