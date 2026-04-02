add_library(render_gl
    include/renderer/render_gl/gl_renderer.h
    src/gl_renderer.cpp
)

target_include_directories(render_gl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(render_gl
    PUBLIC
        render_core
    PRIVATE
        renderer_project_options
)

find_package(OpenGL QUIET)
if(OpenGL_FOUND)
    target_link_libraries(render_gl PUBLIC OpenGL::GL)
    target_compile_definitions(render_gl PUBLIC RENDERER_HAS_OPENGL=1)
else()
    message(WARNING "OpenGL not found. render_gl will build in placeholder mode.")
endif()
