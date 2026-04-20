find_package(Qt5 QUIET COMPONENTS Core Gui Widgets OpenGL)

if(NOT Qt5_FOUND)
    message(WARNING "Qt5 (Core Gui Widgets OpenGL) not found. Skipping test_viewer.")
    return()
endif()

add_executable(test_viewer
    src/main.cpp
    src/camera.h
    src/camera.cpp
    src/camera_clip_utils.h
    src/camera_clip_utils.cpp
    src/camera_focus_bounds_utils.h
    src/camera_focus_bounds_utils.cpp
    src/camera_focus_utils.h
    src/camera_focus_utils.cpp
    src/model_change_view_state.h
    src/model_change_view_state.cpp
    src/viewport_zoom_state.h
    src/viewport_zoom_state.cpp
    src/orbit_camera_controller.h
    src/orbit_camera_controller.cpp
    src/lighting_control_widget.h
    src/lighting_control_widget.cpp
    src/camera_mode_ui_policy.h
    src/camera_mode_ui_policy.cpp
    src/camera_control_widget.h
    src/camera_control_widget.cpp
    src/scene_object_control_widget.h
    src/scene_object_control_widget.cpp
    src/parametric_model_add_adapter.h
    src/parametric_model_add_adapter.cpp
    src/parametric_ui_schema_adapter.h
    src/parametric_ui_schema_adapter.cpp
    src/viewer_ui_types.h
    src/viewer_ui_state_adapter.h
    src/viewer_ui_state_adapter.cpp
    src/viewer_control_panel.h
    src/viewer_control_panel.cpp
    src/viewer_window.h
    src/viewer_window.cpp
)

set_target_properties(test_viewer PROPERTIES
    AUTOMOC ON
)

target_link_libraries(test_viewer
    PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::OpenGL
        render_gl
        parametric_model
)

set_target_properties(test_viewer PROPERTIES
    WIN32_EXECUTABLE TRUE
)

add_executable(viewer_ui_state_adapter_smoke_tests
    src/viewer_ui_types.h
    src/viewer_ui_state_adapter.h
    src/viewer_ui_state_adapter.cpp
    tests/viewer_ui_state_adapter_smoke_tests.cpp
)

target_include_directories(viewer_ui_state_adapter_smoke_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(viewer_ui_state_adapter_smoke_tests
    PRIVATE
        parametric_model
        renderer_project_options
)

add_executable(parametric_ui_schema_adapter_smoke_tests
    src/parametric_ui_schema_adapter.h
    src/parametric_ui_schema_adapter.cpp
    tests/parametric_ui_schema_adapter_smoke_tests.cpp
)

target_include_directories(parametric_ui_schema_adapter_smoke_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(parametric_ui_schema_adapter_smoke_tests
    PRIVATE
        parametric_model
        renderer_project_options
)

add_executable(parametric_model_add_adapter_smoke_tests
    src/parametric_model_add_adapter.h
    src/parametric_model_add_adapter.cpp
    tests/parametric_model_add_adapter_smoke_tests.cpp
)

target_include_directories(parametric_model_add_adapter_smoke_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(parametric_model_add_adapter_smoke_tests
    PRIVATE
        parametric_model
        renderer_project_options
)
