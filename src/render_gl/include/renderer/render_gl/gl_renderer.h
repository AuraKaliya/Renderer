#pragma once

#include <cstdint>
#include <string>

#include "renderer/scene_contract/types.h"
#include "renderer/render_core/frame_assembler.h"

namespace renderer::render_gl {

using ProcAddress = void (*)();
using ProcResolver = ProcAddress (*)(const char* name, void* userData);

struct RenderStats {
    std::uint32_t drawCalls = 0;
    std::uint32_t triangleCount = 0;
};

class GlRenderer {
public:
    GlRenderer();
    ~GlRenderer();

    GlRenderer(const GlRenderer&) = delete;
    GlRenderer& operator=(const GlRenderer&) = delete;

    [[nodiscard]] bool initialize(ProcResolver resolver, void* userData = nullptr);
    void shutdown();

    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] const std::string& lastError() const;

    [[nodiscard]] scene_contract::MeshHandle uploadMesh(
        const scene_contract::MeshData& mesh);
    [[nodiscard]] bool updateMesh(
        scene_contract::MeshHandle handle,
        const scene_contract::MeshData& mesh);
    void releaseMesh(scene_contract::MeshHandle handle);

    [[nodiscard]] scene_contract::MaterialHandle uploadMaterial(
        const scene_contract::MaterialData& material);
    [[nodiscard]] bool updateMaterial(
        scene_contract::MaterialHandle handle,
        const scene_contract::MaterialData& material);
    void releaseMaterial(scene_contract::MaterialHandle handle);

    [[nodiscard]] RenderStats render(
        const render_core::FramePacket& packet);

private:
    struct Impl;
    Impl* impl_ = nullptr;
    std::string lastError_;
};

}  // namespace renderer::render_gl
