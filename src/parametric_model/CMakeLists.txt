add_library(parametric_model
    include/renderer/parametric_model/primitive_factory.h
    src/primitive_factory.cpp
)

target_include_directories(parametric_model
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(parametric_model
    PUBLIC
        scene_contract
    PRIVATE
        renderer_project_options
)
