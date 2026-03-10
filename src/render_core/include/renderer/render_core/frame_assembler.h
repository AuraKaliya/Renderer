#pragma once

#include <vector>

#include "renderer/scene_contract/types.h"

namespace renderer::render_core {

struct FramePacket {
    scene_contract::CameraData camera;
    std::vector<scene_contract::RenderableItem> opaqueItems;
    scene_contract::RenderTargetDesc target {};
};

class FrameAssembler {
public:
    [[nodiscard]] FramePacket build(
        const scene_contract::FrameScene& scene,
        const scene_contract::RenderTargetDesc& target) const;
};

}  // namespace renderer::render_core
