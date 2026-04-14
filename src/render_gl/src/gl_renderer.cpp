#include "renderer/render_gl/gl_renderer.h"

#include <cstddef>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>

#include "gl_functions.h"
#include "renderer/scene_contract/types.h"
#include "shader_library.h"

namespace renderer::render_gl {

#if defined(RENDERER_HAS_OPENGL)
namespace {

static_assert(sizeof(scene_contract::VertexPNT) == sizeof(float) * 8);

template <typename T>
bool loadFunction(
    T& target,
    ProcResolver resolver,
    void* userData,
    const char* name,
    std::string& error) {
    const auto address = resolver(name, userData);
    if (address == nullptr) {
        error = std::string("Missing OpenGL procedure: ") + name;
        return false;
    }
    target = reinterpret_cast<T>(address);
    return true;
}

struct MeshResource {
    GLuint vertexArray = 0U;
    GLuint vertexBuffer = 0U;
    GLuint indexBuffer = 0U;
    GLsizei indexCount = 0;
};

struct MaterialResource {
    scene_contract::MaterialData material;
};

struct TextureResource {
    GLuint textureId = 0U;
    scene_contract::TextureData descriptor;
};

bool uploadTextureResource(
    const GlFunctions& gl,
    const scene_contract::TextureData& textureData,
    TextureResource& resource,
    std::string& error)
{
    if (textureData.width <= 0 || textureData.height <= 0) {
        error = "Texture upload requires positive width and height.";
        return false;
    }

    if (textureData.format != scene_contract::TextureFormat::rgba8) {
        error = "Only rgba8 textures are currently supported.";
        return false;
    }

    const std::size_t expectedSize =
        static_cast<std::size_t>(textureData.width) *
        static_cast<std::size_t>(textureData.height) * 4U;
    if (textureData.pixels.size() != expectedSize) {
        error = "Texture pixel data size does not match width/height/format.";
        return false;
    }

    if (resource.textureId == 0U) {
        ::glGenTextures(1, &resource.textureId);
    }

    ::glBindTexture(GL_TEXTURE_2D, resource.textureId);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureData.generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    ::glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        textureData.width,
        textureData.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        textureData.pixels.data());

    if (textureData.generateMipmaps) {
        gl.GenerateMipmap(GL_TEXTURE_2D);
    }

    ::glBindTexture(GL_TEXTURE_2D, 0);
    resource.descriptor = textureData;
    error.clear();
    return true;
}

void destroyTextureResource(TextureResource& resource) {
    if (resource.textureId != 0U) {
        ::glDeleteTextures(1, &resource.textureId);
        resource.textureId = 0U;
    }
}

bool uploadMeshResource(
    const GlFunctions& gl,
    const scene_contract::MeshData& mesh,
    MeshResource& resource,
    std::string& error) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        error = "Mesh upload requires both vertices and indices.";
        return false;
    }

    if (resource.vertexArray == 0U) {
        gl.GenVertexArrays(1, &resource.vertexArray);
    }
    if (resource.vertexBuffer == 0U) {
        gl.GenBuffers(1, &resource.vertexBuffer);
    }
    if (resource.indexBuffer == 0U) {
        gl.GenBuffers(1, &resource.indexBuffer);
    }

    gl.BindVertexArray(resource.vertexArray);

    gl.BindBuffer(GL_ARRAY_BUFFER, resource.vertexBuffer);
    gl.BufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(scene_contract::VertexPNT)),
        mesh.vertices.data(),
        GL_STATIC_DRAW);

    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, resource.indexBuffer);
    gl.BufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)),
        mesh.indices.data(),
        GL_STATIC_DRAW);

    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(scene_contract::VertexPNT)),
        reinterpret_cast<const void*>(offsetof(scene_contract::VertexPNT, position)));

    gl.EnableVertexAttribArray(1);
    gl.VertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(scene_contract::VertexPNT)),
        reinterpret_cast<const void*>(offsetof(scene_contract::VertexPNT, normal)));

    gl.EnableVertexAttribArray(2);
    gl.VertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(scene_contract::VertexPNT)),
        reinterpret_cast<const void*>(offsetof(scene_contract::VertexPNT, texcoord)));

    gl.BindVertexArray(0);
    resource.indexCount = static_cast<GLsizei>(mesh.indices.size());
    error.clear();
    return true;
}

void destroyMeshResource(const GlFunctions& gl, MeshResource& resource) {
    if (resource.indexBuffer != 0U) {
        gl.DeleteBuffers(1, &resource.indexBuffer);
        resource.indexBuffer = 0U;
    }
    if (resource.vertexBuffer != 0U) {
        gl.DeleteBuffers(1, &resource.vertexBuffer);
        resource.vertexBuffer = 0U;
    }
    if (resource.vertexArray != 0U) {
        gl.DeleteVertexArrays(1, &resource.vertexArray);
        resource.vertexArray = 0U;
    }
    resource.indexCount = 0;
}

void makeNormalMatrix(const scene_contract::Mat4f& world, float normalMatrix[9]) {
    const float a00 = world.elements[0];
    const float a01 = world.elements[4];
    const float a02 = world.elements[8];
    const float a10 = world.elements[1];
    const float a11 = world.elements[5];
    const float a12 = world.elements[9];
    const float a20 = world.elements[2];
    const float a21 = world.elements[6];
    const float a22 = world.elements[10];

    const float inv00 = a11 * a22 - a12 * a21;
    const float inv01 = a02 * a21 - a01 * a22;
    const float inv02 = a01 * a12 - a02 * a11;
    const float inv10 = a12 * a20 - a10 * a22;
    const float inv11 = a00 * a22 - a02 * a20;
    const float inv12 = a02 * a10 - a00 * a12;
    const float inv20 = a10 * a21 - a11 * a20;
    const float inv21 = a01 * a20 - a00 * a21;
    const float inv22 = a00 * a11 - a01 * a10;

    const float determinant = a00 * inv00 + a01 * inv10 + a02 * inv20;
    if (std::abs(determinant) <= 0.000001F) {
        normalMatrix[0] = a00;
        normalMatrix[1] = a10;
        normalMatrix[2] = a20;
        normalMatrix[3] = a01;
        normalMatrix[4] = a11;
        normalMatrix[5] = a21;
        normalMatrix[6] = a02;
        normalMatrix[7] = a12;
        normalMatrix[8] = a22;
        return;
    }

    const float inverseDeterminant = 1.0F / determinant;

    normalMatrix[0] = inv00 * inverseDeterminant;
    normalMatrix[1] = inv01 * inverseDeterminant;
    normalMatrix[2] = inv02 * inverseDeterminant;
    normalMatrix[3] = inv10 * inverseDeterminant;
    normalMatrix[4] = inv11 * inverseDeterminant;
    normalMatrix[5] = inv12 * inverseDeterminant;
    normalMatrix[6] = inv20 * inverseDeterminant;
    normalMatrix[7] = inv21 * inverseDeterminant;
    normalMatrix[8] = inv22 * inverseDeterminant;
}

struct SelectionFeedbackStyle {
    float width = 0.0F;
    scene_contract::ColorRgba surfaceColor {};
    scene_contract::ColorRgba outlineColor {};
};

bool hasSelectionFeedback(scene_contract::InteractionVisualState interaction) {
    return interaction == scene_contract::InteractionVisualState::selected ||
        interaction == scene_contract::InteractionVisualState::active ||
        interaction == scene_contract::InteractionVisualState::hovered;
}

SelectionFeedbackStyle selectionFeedbackStyle(scene_contract::InteractionVisualState interaction) {
    switch (interaction) {
    case scene_contract::InteractionVisualState::active:
        return {0.018F, {1.0F, 0.82F, 0.22F, 0.32F}, {1.0F, 0.82F, 0.22F, 1.0F}};
    case scene_contract::InteractionVisualState::hovered:
        return {0.014F, {0.92F, 0.96F, 1.0F, 0.24F}, {0.92F, 0.96F, 1.0F, 1.0F}};
    case scene_contract::InteractionVisualState::selected:
        return {0.015F, {0.12F, 0.78F, 1.0F, 0.28F}, {0.12F, 0.78F, 1.0F, 1.0F}};
    case scene_contract::InteractionVisualState::normal:
        break;
    }

    return {};
}

}  // namespace

struct GlRenderer::Impl {
    GlFunctions gl;
    ShaderLibrary shaderLibrary;
    scene_contract::MeshHandle nextMeshHandle = 1U;
    scene_contract::MaterialHandle nextMaterialHandle = 1U;
    scene_contract::TextureHandle nextTextureHandle = 1U;
    std::unordered_map<scene_contract::MeshHandle, MeshResource> meshes;
    std::unordered_map<scene_contract::MaterialHandle, MaterialResource> materials;
    std::unordered_map<scene_contract::TextureHandle, TextureResource> textures;
    bool initialized = false;
};
#else
struct GlRenderer::Impl {
};
#endif

GlRenderer::GlRenderer() = default;

GlRenderer::~GlRenderer() {
    // GPU objects are released explicitly via shutdown() while a context is current.
}

bool GlRenderer::initialize(ProcResolver resolver, void* userData) {
#if !defined(RENDERER_HAS_OPENGL)
    (void)resolver;
    (void)userData;
    lastError_ = "OpenGL support is not available for render_gl.";
    return false;
#else
    if (impl_ != nullptr && impl_->initialized) {
        return true;
    }

    if (resolver == nullptr) {
        lastError_ = "No OpenGL procedure resolver was provided.";
        return false;
    }

    auto impl = std::make_unique<Impl>();

    if (!loadFunction(impl->gl.ActiveTexture, resolver, userData, "glActiveTexture", lastError_) ||
        !loadFunction(impl->gl.AttachShader, resolver, userData, "glAttachShader", lastError_) ||
        !loadFunction(impl->gl.BindBuffer, resolver, userData, "glBindBuffer", lastError_) ||
        !loadFunction(impl->gl.BindVertexArray, resolver, userData, "glBindVertexArray", lastError_) ||
        !loadFunction(impl->gl.BufferData, resolver, userData, "glBufferData", lastError_) ||
        !loadFunction(impl->gl.CompileShader, resolver, userData, "glCompileShader", lastError_) ||
        !loadFunction(impl->gl.CreateProgram, resolver, userData, "glCreateProgram", lastError_) ||
        !loadFunction(impl->gl.CreateShader, resolver, userData, "glCreateShader", lastError_) ||
        !loadFunction(impl->gl.DeleteBuffers, resolver, userData, "glDeleteBuffers", lastError_) ||
        !loadFunction(impl->gl.DeleteProgram, resolver, userData, "glDeleteProgram", lastError_) ||
        !loadFunction(impl->gl.DeleteShader, resolver, userData, "glDeleteShader", lastError_) ||
        !loadFunction(impl->gl.DeleteVertexArrays, resolver, userData, "glDeleteVertexArrays", lastError_) ||
        !loadFunction(impl->gl.EnableVertexAttribArray, resolver, userData, "glEnableVertexAttribArray", lastError_) ||
        !loadFunction(impl->gl.GenBuffers, resolver, userData, "glGenBuffers", lastError_) ||
        !loadFunction(impl->gl.GenerateMipmap, resolver, userData, "glGenerateMipmap", lastError_) ||
        !loadFunction(impl->gl.GenVertexArrays, resolver, userData, "glGenVertexArrays", lastError_) ||
        !loadFunction(impl->gl.GetProgramInfoLog, resolver, userData, "glGetProgramInfoLog", lastError_) ||
        !loadFunction(impl->gl.GetProgramiv, resolver, userData, "glGetProgramiv", lastError_) ||
        !loadFunction(impl->gl.GetShaderInfoLog, resolver, userData, "glGetShaderInfoLog", lastError_) ||
        !loadFunction(impl->gl.GetShaderiv, resolver, userData, "glGetShaderiv", lastError_) ||
        !loadFunction(impl->gl.GetUniformLocation, resolver, userData, "glGetUniformLocation", lastError_) ||
        !loadFunction(impl->gl.LinkProgram, resolver, userData, "glLinkProgram", lastError_) ||
        !loadFunction(impl->gl.ShaderSource, resolver, userData, "glShaderSource", lastError_) ||
        !loadFunction(impl->gl.Uniform1f, resolver, userData, "glUniform1f", lastError_) ||
        !loadFunction(impl->gl.Uniform1i, resolver, userData, "glUniform1i", lastError_) ||
        !loadFunction(impl->gl.Uniform3f, resolver, userData, "glUniform3f", lastError_) ||
        !loadFunction(impl->gl.Uniform4f, resolver, userData, "glUniform4f", lastError_) ||
        !loadFunction(impl->gl.UniformMatrix3fv, resolver, userData, "glUniformMatrix3fv", lastError_) ||
        !loadFunction(impl->gl.UniformMatrix4fv, resolver, userData, "glUniformMatrix4fv", lastError_) ||
        !loadFunction(impl->gl.UseProgram, resolver, userData, "glUseProgram", lastError_) ||
        !loadFunction(impl->gl.VertexAttribPointer, resolver, userData, "glVertexAttribPointer", lastError_)) {
        return false;
    }

    if (!impl->shaderLibrary.initialize(impl->gl, lastError_)) {
        return false;
    }

    impl->initialized = true;
    impl_ = impl.release();
    lastError_.clear();
    return true;
#endif
}

void GlRenderer::shutdown() {
#if defined(RENDERER_HAS_OPENGL)
    if (impl_ == nullptr) {
        return;
    }

    for (auto& [handle, mesh] : impl_->meshes) {
        (void)handle;
        destroyMeshResource(impl_->gl, mesh);
    }
    for (auto& [handle, texture] : impl_->textures) {
        (void)handle;
        destroyTextureResource(texture);
    }
    impl_->meshes.clear();
    impl_->materials.clear();
    impl_->textures.clear();

    impl_->shaderLibrary.shutdown(impl_->gl);
#endif

    delete impl_;
    impl_ = nullptr;
}

bool GlRenderer::isInitialized() const {
#if defined(RENDERER_HAS_OPENGL)
    return impl_ != nullptr && impl_->initialized;
#else
    return false;
#endif
}

const std::string& GlRenderer::lastError() const {
    return lastError_;
}

scene_contract::MeshHandle GlRenderer::uploadMesh(const scene_contract::MeshData& mesh) {
#if !defined(RENDERER_HAS_OPENGL)
    (void)mesh;
    lastError_ = "OpenGL support is not available for render_gl.";
    return scene_contract::kInvalidMeshHandle;
#else
    if (impl_ == nullptr || !impl_->initialized) {
        lastError_ = "GlRenderer::initialize must be called before uploadMesh.";
        return scene_contract::kInvalidMeshHandle;
    }

    MeshResource resource;
    if (!uploadMeshResource(impl_->gl, mesh, resource, lastError_)) {
        destroyMeshResource(impl_->gl, resource);
        return scene_contract::kInvalidMeshHandle;
    }

    const auto handle = impl_->nextMeshHandle++;
    impl_->meshes.emplace(handle, resource);
    lastError_.clear();
    return handle;
#endif
}

bool GlRenderer::updateMesh(
    scene_contract::MeshHandle handle,
    const scene_contract::MeshData& mesh) {
#if !defined(RENDERER_HAS_OPENGL)
    (void)handle;
    (void)mesh;
    lastError_ = "OpenGL support is not available for render_gl.";
    return false;
#else
    if (impl_ == nullptr || !impl_->initialized) {
        lastError_ = "GlRenderer::initialize must be called before updateMesh.";
        return false;
    }

    const auto iterator = impl_->meshes.find(handle);
    if (iterator == impl_->meshes.end()) {
        lastError_ = "updateMesh received an unknown MeshHandle.";
        return false;
    }

    if (!uploadMeshResource(impl_->gl, mesh, iterator->second, lastError_)) {
        return false;
    }

    lastError_.clear();
    return true;
#endif
}

void GlRenderer::releaseMesh(scene_contract::MeshHandle handle) {
#if defined(RENDERER_HAS_OPENGL)
    if (impl_ == nullptr || !impl_->initialized) {
        return;
    }

    const auto iterator = impl_->meshes.find(handle);
    if (iterator == impl_->meshes.end()) {
        return;
    }

    destroyMeshResource(impl_->gl, iterator->second);
    impl_->meshes.erase(iterator);
#else
    (void)handle;
#endif
}

scene_contract::MaterialHandle GlRenderer::uploadMaterial(
    const scene_contract::MaterialData& material) {
#if !defined(RENDERER_HAS_OPENGL)
    (void)material;
    lastError_ = "OpenGL support is not available for render_gl.";
    return scene_contract::kInvalidMaterialHandle;
#else
    if (impl_ == nullptr || !impl_->initialized) {
        lastError_ = "GlRenderer::initialize must be called before uploadMaterial.";
        return scene_contract::kInvalidMaterialHandle;
    }

    const auto handle = impl_->nextMaterialHandle++;
    impl_->materials.emplace(handle, MaterialResource {material});
    lastError_.clear();
    return handle;
#endif
}

bool GlRenderer::updateMaterial(
    scene_contract::MaterialHandle handle,
    const scene_contract::MaterialData& material) {
#if !defined(RENDERER_HAS_OPENGL)
    (void)handle;
    (void)material;
    lastError_ = "OpenGL support is not available for render_gl.";
    return false;
#else
    if (impl_ == nullptr || !impl_->initialized) {
        lastError_ = "GlRenderer::initialize must be called before updateMaterial.";
        return false;
    }

    const auto iterator = impl_->materials.find(handle);
    if (iterator == impl_->materials.end()) {
        lastError_ = "updateMaterial received an unknown MaterialHandle.";
        return false;
    }

    iterator->second.material = material;
    lastError_.clear();
    return true;
#endif
}

void GlRenderer::releaseMaterial(scene_contract::MaterialHandle handle) {
#if defined(RENDERER_HAS_OPENGL)
    if (impl_ == nullptr || !impl_->initialized) {
        return;
    }

    impl_->materials.erase(handle);
#else
    (void)handle;
#endif
}

scene_contract::TextureHandle GlRenderer::uploadTexture(
    const scene_contract::TextureData& texture) {
#if !defined(RENDERER_HAS_OPENGL)
    (void)texture;
    lastError_ = "OpenGL support is not available for render_gl.";
    return scene_contract::kInvalidTextureHandle;
#else
    if (impl_ == nullptr || !impl_->initialized) {
        lastError_ = "GlRenderer::initialize must be called before uploadTexture.";
        return scene_contract::kInvalidTextureHandle;
    }

    const auto handle = impl_->nextTextureHandle++;
    TextureResource resource;
    if (!uploadTextureResource(impl_->gl, texture, resource, lastError_)) {
        destroyTextureResource(resource);
        return scene_contract::kInvalidTextureHandle;
    }
    impl_->textures.emplace(handle, resource);
    lastError_.clear();
    return handle;
#endif
}

bool GlRenderer::updateTexture(
    scene_contract::TextureHandle handle,
    const scene_contract::TextureData& texture) {
#if !defined(RENDERER_HAS_OPENGL)
    (void)handle;
    (void)texture;
    lastError_ = "OpenGL support is not available for render_gl.";
    return false;
#else
    if (impl_ == nullptr || !impl_->initialized) {
        lastError_ = "GlRenderer::initialize must be called before updateTexture.";
        return false;
    }

    const auto iterator = impl_->textures.find(handle);
    if (iterator == impl_->textures.end()) {
        lastError_ = "updateTexture received an unknown TextureHandle.";
        return false;
    }

    if (!uploadTextureResource(impl_->gl, texture, iterator->second, lastError_)) {
        return false;
    }
    lastError_.clear();
    return true;
#endif
}

void GlRenderer::releaseTexture(scene_contract::TextureHandle handle) {
#if defined(RENDERER_HAS_OPENGL)
    if (impl_ == nullptr || !impl_->initialized) {
        return;
    }

    const auto iterator = impl_->textures.find(handle);
    if (iterator == impl_->textures.end()) {
        return;
    }

    destroyTextureResource(iterator->second);
    impl_->textures.erase(iterator);
#else
    (void)handle;
#endif
}

RenderStats GlRenderer::render(const render_core::FramePacket& packet) {
    RenderStats stats;
#if !defined(RENDERER_HAS_OPENGL)
    lastError_ = "OpenGL support is not available for render_gl.";
    return stats;
#else
    if (impl_ == nullptr || !impl_->initialized) {
        lastError_ = "GlRenderer::initialize must be called before render.";
        return stats;
    }

    if (packet.target.width > 0 && packet.target.height > 0) {
        glViewport(0, 0, packet.target.width, packet.target.height);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glClearColor(0.08F, 0.10F, 0.14F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    const auto& litMesh = impl_->shaderLibrary.litMesh();
    impl_->gl.UseProgram(litMesh.program);

    impl_->gl.UniformMatrix4fv(litMesh.view, 1, GL_FALSE, packet.camera.view.elements);
    impl_->gl.UniformMatrix4fv(litMesh.projection, 1, GL_FALSE, packet.camera.projection.elements);
    impl_->gl.Uniform3f(
        litMesh.lightDirection,
        packet.light.direction.x,
        packet.light.direction.y,
        packet.light.direction.z);
    impl_->gl.Uniform3f(
        litMesh.lightColor,
        packet.light.color.x,
        packet.light.color.y,
        packet.light.color.z);
    impl_->gl.Uniform1f(litMesh.ambientStrength, packet.light.ambientStrength);
    impl_->gl.Uniform1i(litMesh.baseColorTexture, 0);

    for (const auto& queueItem : packet.opaqueItems) {
        const auto& item = queueItem.item;
        if (item.meshHandle == scene_contract::kInvalidMeshHandle) {
            continue;
        }
        if (item.materialHandle == scene_contract::kInvalidMaterialHandle) {
            continue;
        }

        const auto meshIterator = impl_->meshes.find(item.meshHandle);
        if (meshIterator == impl_->meshes.end()) {
            continue;
        }

        const auto materialIterator = impl_->materials.find(item.materialHandle);
        if (materialIterator == impl_->materials.end()) {
            continue;
        }

        const auto& mesh = meshIterator->second;
        const auto& material = materialIterator->second.material;
        impl_->gl.BindVertexArray(mesh.vertexArray);

        impl_->gl.UniformMatrix4fv(litMesh.world, 1, GL_FALSE, item.transform.world.elements);
        float normalMatrix[9] {};
        makeNormalMatrix(item.transform.world, normalMatrix);
        impl_->gl.UniformMatrix3fv(litMesh.normalMatrix, 1, GL_FALSE, normalMatrix);
        impl_->gl.Uniform4f(
            litMesh.baseColor,
            material.baseColor.r,
            material.baseColor.g,
            material.baseColor.b,
            material.baseColor.a);

        bool useTexture = false;
        if (material.useBaseColorTexture &&
            material.baseColorTexture != scene_contract::kInvalidTextureHandle) {
            const auto textureIterator = impl_->textures.find(material.baseColorTexture);
            if (textureIterator != impl_->textures.end() &&
                textureIterator->second.textureId != 0U) {
                useTexture = true;
                impl_->gl.ActiveTexture(GL_TEXTURE0);
                ::glBindTexture(GL_TEXTURE_2D, textureIterator->second.textureId);
            }
        }

        impl_->gl.Uniform1i(litMesh.useBaseColorTexture, useTexture ? 1 : 0);
        if (!useTexture) {
            impl_->gl.ActiveTexture(GL_TEXTURE0);
            ::glBindTexture(GL_TEXTURE_2D, 0);
        }

        ::glDrawElements(
            GL_TRIANGLES,
            mesh.indexCount,
            GL_UNSIGNED_INT,
            nullptr);

        ++stats.drawCalls;
        stats.triangleCount += static_cast<std::uint32_t>(mesh.indexCount / 3U);
    }

    impl_->gl.ActiveTexture(GL_TEXTURE0);
    ::glBindTexture(GL_TEXTURE_2D, 0);

    bool hasSelectionItems = false;
    for (const auto& queueItem : packet.opaqueItems) {
        if (hasSelectionFeedback(queueItem.item.visual.interaction)) {
            hasSelectionItems = true;
            break;
        }
    }

    if (hasSelectionItems) {
        ::glEnable(GL_STENCIL_TEST);
        ::glClear(GL_STENCIL_BUFFER_BIT);
        ::glStencilMask(0xFF);
        ::glStencilFunc(GL_ALWAYS, 1, 0xFF);
        ::glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        ::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        ::glDepthMask(GL_FALSE);
        ::glEnable(GL_DEPTH_TEST);
        ::glDepthFunc(GL_LEQUAL);

        const auto& selectionMask = impl_->shaderLibrary.selectionMask();
        impl_->gl.UseProgram(selectionMask.program);
        impl_->gl.UniformMatrix4fv(selectionMask.view, 1, GL_FALSE, packet.camera.view.elements);
        impl_->gl.UniformMatrix4fv(selectionMask.projection, 1, GL_FALSE, packet.camera.projection.elements);

        for (const auto& queueItem : packet.opaqueItems) {
            const auto& item = queueItem.item;
            if (!hasSelectionFeedback(item.visual.interaction)) {
                continue;
            }

            const auto meshIterator = impl_->meshes.find(item.meshHandle);
            if (meshIterator == impl_->meshes.end()) {
                continue;
            }

            const auto& mesh = meshIterator->second;
            impl_->gl.BindVertexArray(mesh.vertexArray);
            impl_->gl.UniformMatrix4fv(selectionMask.world, 1, GL_FALSE, item.transform.world.elements);
            ::glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);

            ++stats.drawCalls;
            stats.triangleCount += static_cast<std::uint32_t>(mesh.indexCount / 3U);
        }

        ::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        ::glStencilMask(0x00);
        ::glStencilFunc(GL_EQUAL, 1, 0xFF);
        ::glEnable(GL_DEPTH_TEST);
        ::glDepthMask(GL_FALSE);
        ::glEnable(GL_BLEND);
        ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const auto& selectionOutline = impl_->shaderLibrary.selectionOutline();
        impl_->gl.UseProgram(selectionOutline.program);
        impl_->gl.UniformMatrix4fv(selectionOutline.view, 1, GL_FALSE, packet.camera.view.elements);
        impl_->gl.UniformMatrix4fv(selectionOutline.projection, 1, GL_FALSE, packet.camera.projection.elements);
        impl_->gl.Uniform3f(
            selectionOutline.lightDirection,
            packet.light.direction.x,
            packet.light.direction.y,
            packet.light.direction.z);
        impl_->gl.Uniform3f(
            selectionOutline.lightColor,
            packet.light.color.x,
            packet.light.color.y,
            packet.light.color.z);
        impl_->gl.Uniform1f(selectionOutline.ambientStrength, packet.light.ambientStrength);

        for (const auto& queueItem : packet.opaqueItems) {
            const auto& item = queueItem.item;
            if (!hasSelectionFeedback(item.visual.interaction)) {
                continue;
            }

            const auto meshIterator = impl_->meshes.find(item.meshHandle);
            if (meshIterator == impl_->meshes.end()) {
                continue;
            }

            const auto style = selectionFeedbackStyle(item.visual.interaction);
            const auto& mesh = meshIterator->second;
            impl_->gl.BindVertexArray(mesh.vertexArray);
            impl_->gl.UniformMatrix4fv(selectionOutline.world, 1, GL_FALSE, item.transform.world.elements);
            float normalMatrix[9] {};
            makeNormalMatrix(item.transform.world, normalMatrix);
            impl_->gl.UniformMatrix3fv(selectionOutline.normalMatrix, 1, GL_FALSE, normalMatrix);
            impl_->gl.Uniform1f(selectionOutline.outlineWidth, 0.0F);
            impl_->gl.Uniform4f(
                selectionOutline.outlineColor,
                style.surfaceColor.r,
                style.surfaceColor.g,
                style.surfaceColor.b,
                style.surfaceColor.a);
            ::glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);

            ++stats.drawCalls;
            stats.triangleCount += static_cast<std::uint32_t>(mesh.indexCount / 3U);
        }

        ::glDisable(GL_BLEND);
        ::glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        ::glEnable(GL_CULL_FACE);
        ::glCullFace(GL_FRONT);

        for (const auto& queueItem : packet.opaqueItems) {
            const auto& item = queueItem.item;
            if (!hasSelectionFeedback(item.visual.interaction)) {
                continue;
            }

            const auto meshIterator = impl_->meshes.find(item.meshHandle);
            if (meshIterator == impl_->meshes.end()) {
                continue;
            }

            const auto style = selectionFeedbackStyle(item.visual.interaction);
            const auto& mesh = meshIterator->second;
            impl_->gl.BindVertexArray(mesh.vertexArray);
            impl_->gl.UniformMatrix4fv(selectionOutline.world, 1, GL_FALSE, item.transform.world.elements);
            float normalMatrix[9] {};
            makeNormalMatrix(item.transform.world, normalMatrix);
            impl_->gl.UniformMatrix3fv(selectionOutline.normalMatrix, 1, GL_FALSE, normalMatrix);
            impl_->gl.Uniform1f(selectionOutline.outlineWidth, style.width);
            impl_->gl.Uniform4f(
                selectionOutline.outlineColor,
                style.outlineColor.r,
                style.outlineColor.g,
                style.outlineColor.b,
                style.outlineColor.a);
            ::glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);

            ++stats.drawCalls;
            stats.triangleCount += static_cast<std::uint32_t>(mesh.indexCount / 3U);
        }

        ::glCullFace(GL_BACK);
        ::glDisable(GL_CULL_FACE);
        ::glStencilMask(0xFF);
        ::glDisable(GL_STENCIL_TEST);
        ::glEnable(GL_DEPTH_TEST);
        ::glDepthFunc(GL_LESS);
        ::glDepthMask(GL_TRUE);
    }

    impl_->gl.BindVertexArray(0);
    impl_->gl.UseProgram(0);
    lastError_.clear();
    return stats;
#endif
}

}  // namespace renderer::render_gl
