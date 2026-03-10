add_library(render_core
    include/renderer/render_core/frame_assembler.h
    include/renderer/render_core/scene_repository.h
    src/frame_assembler.cpp
    src/scene_repository.cpp
)

target_include_directories(render_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(render_core
    PUBLIC
        scene_contract
    PRIVATE
        renderer_project_options
)
