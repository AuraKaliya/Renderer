#include "renderer/render_core/frame_assembler.h"

namespace renderer::render_core {

FramePacket FrameAssembler::build(
    const scene_contract::FrameScene& scene,
    const scene_contract::RenderTargetDesc& target) const {
    FramePacket packet;
    packet.camera = scene.camera;
    packet.light = scene.light;
    packet.opaqueItems = scene.items;
    packet.target = target;
    return packet;
}

}  // namespace renderer::render_core
