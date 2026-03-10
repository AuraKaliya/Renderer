add_library(scene_contract INTERFACE)

target_include_directories(scene_contract
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(scene_contract
    INTERFACE
        renderer_project_options
)
