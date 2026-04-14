#pragma once

#include <string>

#if defined(RENDERER_HAS_OPENGL)
#include "gl_functions.h"

namespace renderer::render_gl {

struct LitMeshBindings {
    GLuint program = 0U;
    GLint world = -1;
    GLint normalMatrix = -1;
    GLint view = -1;
    GLint projection = -1;
    GLint baseColor = -1;
    GLint baseColorTexture = -1;
    GLint useBaseColorTexture = -1;
    GLint lightDirection = -1;
    GLint lightColor = -1;
    GLint ambientStrength = -1;
};

struct SelectionMaskBindings {
    GLuint program = 0U;
    GLint world = -1;
    GLint view = -1;
    GLint projection = -1;
};

struct SelectionOutlineBindings {
    GLuint program = 0U;
    GLint world = -1;
    GLint normalMatrix = -1;
    GLint view = -1;
    GLint projection = -1;
    GLint outlineWidth = -1;
    GLint lightDirection = -1;
    GLint lightColor = -1;
    GLint ambientStrength = -1;
    GLint outlineColor = -1;
};

class ShaderLibrary final {
public:
    bool initialize(const GlFunctions& gl, std::string& error);
    void shutdown(const GlFunctions& gl);

    [[nodiscard]] const LitMeshBindings& litMesh() const;
    [[nodiscard]] const SelectionMaskBindings& selectionMask() const;
    [[nodiscard]] const SelectionOutlineBindings& selectionOutline() const;

private:
    LitMeshBindings litMesh_ {};
    SelectionMaskBindings selectionMask_ {};
    SelectionOutlineBindings selectionOutline_ {};
};

}  // namespace renderer::render_gl
#endif
