#include "renderer/render_gl/gl_renderer.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "renderer/scene_contract/types.h"

#if defined(RENDERER_HAS_OPENGL)
#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif

namespace renderer::render_gl {

#if defined(RENDERER_HAS_OPENGL)
namespace {

constexpr auto kVertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uWorld;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldNormal;

void main() {
    vWorldNormal = mat3(uWorld) * aNormal;
    gl_Position = uProjection * uView * uWorld * vec4(aPosition, 1.0);
}
)";

constexpr auto kFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 uBaseColor;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uAmbientStrength;

in vec3 vWorldNormal;

void main() {
    vec3 normal = normalize(vWorldNormal);
    vec3 lightDir = normalize(-uLightDirection);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 ambient = uAmbientStrength * uBaseColor.rgb;
    vec3 litColor = ambient + diffuse * uBaseColor.rgb * uLightColor;
    FragColor = vec4(litColor, uBaseColor.a);
}
)";

static_assert(sizeof(scene_contract::VertexPNT) == sizeof(float) * 8);

struct GlFunctions {
    PFNGLATTACHSHADERPROC AttachShader = nullptr;
    PFNGLBINDBUFFERPROC BindBuffer = nullptr;
    PFNGLBINDVERTEXARRAYPROC BindVertexArray = nullptr;
    PFNGLBUFFERDATAPROC BufferData = nullptr;
    PFNGLCOMPILESHADERPROC CompileShader = nullptr;
    PFNGLCREATEPROGRAMPROC CreateProgram = nullptr;
    PFNGLCREATESHADERPROC CreateShader = nullptr;
    PFNGLDELETEBUFFERSPROC DeleteBuffers = nullptr;
    PFNGLDELETEPROGRAMPROC DeleteProgram = nullptr;
    PFNGLDELETESHADERPROC DeleteShader = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray = nullptr;
    PFNGLGENBUFFERSPROC GenBuffers = nullptr;
    PFNGLGENVERTEXARRAYSPROC GenVertexArrays = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog = nullptr;
    PFNGLGETPROGRAMIVPROC GetProgramiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog = nullptr;
    PFNGLGETSHADERIVPROC GetShaderiv = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation = nullptr;
    PFNGLLINKPROGRAMPROC LinkProgram = nullptr;
    PFNGLSHADERSOURCEPROC ShaderSource = nullptr;
    PFNGLUNIFORM1FPROC Uniform1f = nullptr;
    PFNGLUNIFORM3FPROC Uniform3f = nullptr;
    PFNGLUNIFORM4FPROC Uniform4f = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv = nullptr;
    PFNGLUSEPROGRAMPROC UseProgram = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer = nullptr;
};

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

GLuint compileShader(
    const GlFunctions& gl,
    GLenum type,
    const char* source,
    std::string& error) {
    const auto shader = gl.CreateShader(type);
    if (shader == 0U) {
        error = "glCreateShader failed.";
        return 0U;
    }

    gl.ShaderSource(shader, 1, &source, nullptr);
    gl.CompileShader(shader);

    GLint compileStatus = 0;
    gl.GetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_TRUE) {
        return shader;
    }

    GLint logLength = 0;
    gl.GetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength), '\0');
    if (logLength > 0) {
        gl.GetShaderInfoLog(shader, logLength, nullptr, log.data());
    }
    gl.DeleteShader(shader);
    error = "Shader compilation failed: " + log;
    return 0U;
}

GLuint createProgram(const GlFunctions& gl, std::string& error) {
    const auto vertexShader = compileShader(gl, GL_VERTEX_SHADER, kVertexShaderSource, error);
    if (vertexShader == 0U) {
        return 0U;
    }

    const auto fragmentShader = compileShader(gl, GL_FRAGMENT_SHADER, kFragmentShaderSource, error);
    if (fragmentShader == 0U) {
        gl.DeleteShader(vertexShader);
        return 0U;
    }

    const auto program = gl.CreateProgram();
    if (program == 0U) {
        gl.DeleteShader(vertexShader);
        gl.DeleteShader(fragmentShader);
        error = "glCreateProgram failed.";
        return 0U;
    }

    gl.AttachShader(program, vertexShader);
    gl.AttachShader(program, fragmentShader);
    gl.LinkProgram(program);

    gl.DeleteShader(vertexShader);
    gl.DeleteShader(fragmentShader);

    GLint linkStatus = 0;
    gl.GetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_TRUE) {
        return program;
    }

    GLint logLength = 0;
    gl.GetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength), '\0');
    if (logLength > 0) {
        gl.GetProgramInfoLog(program, logLength, nullptr, log.data());
    }
    gl.DeleteProgram(program);
    error = "Program link failed: " + log;
    return 0U;
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

}  // namespace

struct GlRenderer::Impl {
    GlFunctions gl;
    GLuint program = 0U;
    GLint worldLocation = -1;
    GLint viewLocation = -1;
    GLint projectionLocation = -1;
    GLint colorLocation = -1;
    GLint lightDirectionLocation = -1;
    GLint lightColorLocation = -1;
    GLint ambientStrengthLocation = -1;
    scene_contract::MeshHandle nextMeshHandle = 1U;
    scene_contract::MaterialHandle nextMaterialHandle = 1U;
    std::unordered_map<scene_contract::MeshHandle, MeshResource> meshes;
    std::unordered_map<scene_contract::MaterialHandle, MaterialResource> materials;
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

    if (!loadFunction(impl->gl.AttachShader, resolver, userData, "glAttachShader", lastError_) ||
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
        !loadFunction(impl->gl.GenVertexArrays, resolver, userData, "glGenVertexArrays", lastError_) ||
        !loadFunction(impl->gl.GetProgramInfoLog, resolver, userData, "glGetProgramInfoLog", lastError_) ||
        !loadFunction(impl->gl.GetProgramiv, resolver, userData, "glGetProgramiv", lastError_) ||
        !loadFunction(impl->gl.GetShaderInfoLog, resolver, userData, "glGetShaderInfoLog", lastError_) ||
        !loadFunction(impl->gl.GetShaderiv, resolver, userData, "glGetShaderiv", lastError_) ||
        !loadFunction(impl->gl.GetUniformLocation, resolver, userData, "glGetUniformLocation", lastError_) ||
        !loadFunction(impl->gl.LinkProgram, resolver, userData, "glLinkProgram", lastError_) ||
        !loadFunction(impl->gl.ShaderSource, resolver, userData, "glShaderSource", lastError_) ||
        !loadFunction(impl->gl.Uniform1f, resolver, userData, "glUniform1f", lastError_) ||
        !loadFunction(impl->gl.Uniform3f, resolver, userData, "glUniform3f", lastError_) ||
        !loadFunction(impl->gl.Uniform4f, resolver, userData, "glUniform4f", lastError_) ||
        !loadFunction(impl->gl.UniformMatrix4fv, resolver, userData, "glUniformMatrix4fv", lastError_) ||
        !loadFunction(impl->gl.UseProgram, resolver, userData, "glUseProgram", lastError_) ||
        !loadFunction(impl->gl.VertexAttribPointer, resolver, userData, "glVertexAttribPointer", lastError_)) {
        return false;
    }

    impl->program = createProgram(impl->gl, lastError_);
    if (impl->program == 0U) {
        return false;
    }

    impl->worldLocation = impl->gl.GetUniformLocation(impl->program, "uWorld");
    impl->viewLocation = impl->gl.GetUniformLocation(impl->program, "uView");
    impl->projectionLocation = impl->gl.GetUniformLocation(impl->program, "uProjection");
    impl->colorLocation = impl->gl.GetUniformLocation(impl->program, "uBaseColor");
    impl->lightDirectionLocation = impl->gl.GetUniformLocation(impl->program, "uLightDirection");
    impl->lightColorLocation = impl->gl.GetUniformLocation(impl->program, "uLightColor");
    impl->ambientStrengthLocation = impl->gl.GetUniformLocation(impl->program, "uAmbientStrength");

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
    impl_->meshes.clear();
    impl_->materials.clear();

    if (impl_->program != 0U) {
        impl_->gl.DeleteProgram(impl_->program);
    }
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
    glClearColor(0.08F, 0.10F, 0.14F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    impl_->gl.UseProgram(impl_->program);

    impl_->gl.UniformMatrix4fv(impl_->viewLocation, 1, GL_FALSE, packet.camera.view.elements);
    impl_->gl.UniformMatrix4fv(impl_->projectionLocation, 1, GL_FALSE, packet.camera.projection.elements);
    impl_->gl.Uniform3f(
        impl_->lightDirectionLocation,
        packet.light.direction.x,
        packet.light.direction.y,
        packet.light.direction.z);
    impl_->gl.Uniform3f(
        impl_->lightColorLocation,
        packet.light.color.x,
        packet.light.color.y,
        packet.light.color.z);
    impl_->gl.Uniform1f(impl_->ambientStrengthLocation, packet.light.ambientStrength);

    for (const auto& item : packet.opaqueItems) {
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

        impl_->gl.UniformMatrix4fv(impl_->worldLocation, 1, GL_FALSE, item.transform.world.elements);
        impl_->gl.Uniform4f(
            impl_->colorLocation,
            material.baseColor.r,
            material.baseColor.g,
            material.baseColor.b,
            material.baseColor.a);


        ::glDrawElements(
            GL_TRIANGLES,
            mesh.indexCount,
            GL_UNSIGNED_INT,
            nullptr);

        ++stats.drawCalls;
        stats.triangleCount += static_cast<std::uint32_t>(mesh.indexCount / 3U);
    }

    impl_->gl.BindVertexArray(0);
    impl_->gl.UseProgram(0);
    lastError_.clear();
    return stats;
#endif
}

}  // namespace renderer::render_gl
