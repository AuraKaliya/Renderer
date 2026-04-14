#include "shader_library.h"

#if defined(RENDERER_HAS_OPENGL)
#include <cstddef>

namespace renderer::render_gl {
namespace {

constexpr auto kLitMeshVertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexcoord;

uniform mat4 uWorld;
uniform mat3 uNormalMatrix;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldNormal;
out vec2 vTexcoord;

void main() {
    vWorldNormal = uNormalMatrix * aNormal;
    vTexcoord = aTexcoord;
    gl_Position = uProjection * uView * uWorld * vec4(aPosition, 1.0);
}
)";

constexpr auto kLitMeshFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 uBaseColor;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uAmbientStrength;
uniform sampler2D uBaseColorTexture;
uniform int uUseBaseColorTexture;

in vec3 vWorldNormal;
in vec2 vTexcoord;

void main() {
    vec3 normal = normalize(vWorldNormal);
    vec3 lightDir = normalize(-uLightDirection);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec4 albedo = uBaseColor;
    if (uUseBaseColorTexture == 1) {
        albedo *= texture(uBaseColorTexture, vTexcoord);
    }
    vec3 ambient = uAmbientStrength * albedo.rgb;
    vec3 litColor = ambient + diffuse * albedo.rgb * uLightColor;
    FragColor = vec4(litColor, albedo.a);
}
)";

constexpr auto kSelectionMaskVertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uWorld;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * uView * uWorld * vec4(aPosition, 1.0);
}
)";

constexpr auto kSelectionMaskFragmentShaderSource = R"(
#version 330 core
void main() {
}
)";

constexpr auto kSelectionOutlineVertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uWorld;
uniform mat3 uNormalMatrix;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uOutlineWidth;

out vec3 vWorldNormal;

void main() {
    vec3 expandedPosition = aPosition + normalize(aNormal) * uOutlineWidth;
    vWorldNormal = uNormalMatrix * aNormal;
    gl_Position = uProjection * uView * uWorld * vec4(expandedPosition, 1.0);
}
)";

constexpr auto kSelectionOutlineFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 uOutlineColor;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uAmbientStrength;

in vec3 vWorldNormal;

void main() {
    vec3 normal = normalize(vWorldNormal);
    vec3 lightDir = normalize(-uLightDirection);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 litColor = uOutlineColor.rgb * (max(uAmbientStrength, 0.36) + diffuse * uLightColor);
    FragColor = vec4(litColor, uOutlineColor.a);
}
)";

GLuint compileShader(
    const GlFunctions& gl,
    GLenum type,
    const char* source,
    std::string& error)
{
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

GLuint createProgram(
    const GlFunctions& gl,
    const char* vertexShaderSource,
    const char* fragmentShaderSource,
    std::string& error)
{
    const auto vertexShader = compileShader(gl, GL_VERTEX_SHADER, vertexShaderSource, error);
    if (vertexShader == 0U) {
        return 0U;
    }

    const auto fragmentShader = compileShader(gl, GL_FRAGMENT_SHADER, fragmentShaderSource, error);
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

}  // namespace

bool ShaderLibrary::initialize(const GlFunctions& gl, std::string& error) {
    shutdown(gl);

    litMesh_.program = createProgram(gl, kLitMeshVertexShaderSource, kLitMeshFragmentShaderSource, error);
    if (litMesh_.program == 0U) {
        return false;
    }

    litMesh_.world = gl.GetUniformLocation(litMesh_.program, "uWorld");
    litMesh_.normalMatrix = gl.GetUniformLocation(litMesh_.program, "uNormalMatrix");
    litMesh_.view = gl.GetUniformLocation(litMesh_.program, "uView");
    litMesh_.projection = gl.GetUniformLocation(litMesh_.program, "uProjection");
    litMesh_.baseColor = gl.GetUniformLocation(litMesh_.program, "uBaseColor");
    litMesh_.baseColorTexture = gl.GetUniformLocation(litMesh_.program, "uBaseColorTexture");
    litMesh_.useBaseColorTexture = gl.GetUniformLocation(litMesh_.program, "uUseBaseColorTexture");
    litMesh_.lightDirection = gl.GetUniformLocation(litMesh_.program, "uLightDirection");
    litMesh_.lightColor = gl.GetUniformLocation(litMesh_.program, "uLightColor");
    litMesh_.ambientStrength = gl.GetUniformLocation(litMesh_.program, "uAmbientStrength");

    selectionMask_.program =
        createProgram(gl, kSelectionMaskVertexShaderSource, kSelectionMaskFragmentShaderSource, error);
    if (selectionMask_.program == 0U) {
        shutdown(gl);
        return false;
    }

    selectionMask_.world = gl.GetUniformLocation(selectionMask_.program, "uWorld");
    selectionMask_.view = gl.GetUniformLocation(selectionMask_.program, "uView");
    selectionMask_.projection = gl.GetUniformLocation(selectionMask_.program, "uProjection");

    selectionOutline_.program =
        createProgram(gl, kSelectionOutlineVertexShaderSource, kSelectionOutlineFragmentShaderSource, error);
    if (selectionOutline_.program == 0U) {
        shutdown(gl);
        return false;
    }

    selectionOutline_.world = gl.GetUniformLocation(selectionOutline_.program, "uWorld");
    selectionOutline_.normalMatrix = gl.GetUniformLocation(selectionOutline_.program, "uNormalMatrix");
    selectionOutline_.view = gl.GetUniformLocation(selectionOutline_.program, "uView");
    selectionOutline_.projection = gl.GetUniformLocation(selectionOutline_.program, "uProjection");
    selectionOutline_.outlineWidth = gl.GetUniformLocation(selectionOutline_.program, "uOutlineWidth");
    selectionOutline_.lightDirection = gl.GetUniformLocation(selectionOutline_.program, "uLightDirection");
    selectionOutline_.lightColor = gl.GetUniformLocation(selectionOutline_.program, "uLightColor");
    selectionOutline_.ambientStrength = gl.GetUniformLocation(selectionOutline_.program, "uAmbientStrength");
    selectionOutline_.outlineColor = gl.GetUniformLocation(selectionOutline_.program, "uOutlineColor");

    error.clear();
    return true;
}

void ShaderLibrary::shutdown(const GlFunctions& gl) {
    if (litMesh_.program != 0U) {
        gl.DeleteProgram(litMesh_.program);
    }
    if (selectionMask_.program != 0U) {
        gl.DeleteProgram(selectionMask_.program);
    }
    if (selectionOutline_.program != 0U) {
        gl.DeleteProgram(selectionOutline_.program);
    }

    litMesh_ = {};
    selectionMask_ = {};
    selectionOutline_ = {};
}

const LitMeshBindings& ShaderLibrary::litMesh() const {
    return litMesh_;
}

const SelectionMaskBindings& ShaderLibrary::selectionMask() const {
    return selectionMask_;
}

const SelectionOutlineBindings& ShaderLibrary::selectionOutline() const {
    return selectionOutline_;
}

}  // namespace renderer::render_gl
#endif
