find_package(Qt5 QUIET COMPONENTS Core Gui Widgets OpenGL)

if(NOT Qt5_FOUND)
    message(WARNING "Qt5 (Core Gui Widgets OpenGL) not found. Skipping test_viewer.")
    return()
endif()

add_executable(test_viewer
    src/main.cpp
    src/viewer_window.h
    src/viewer_window.cpp
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
