cmake_minimum_required(VERSION 3.24)

project(Renderer VERSION 0.1.0 LANGUAGES CXX)

option(RENDERER_BUILD_TEST_VIEWER "Build the Qt test viewer." ON)
option(RENDERER_WARNINGS_AS_ERRORS "Treat warnings as errors." OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(renderer_project_options INTERFACE)
target_compile_features(renderer_project_options INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(renderer_project_options INTERFACE /W4)
    if(RENDERER_WARNINGS_AS_ERRORS)
        target_compile_options(renderer_project_options INTERFACE /WX)
    endif()
else()
    target_compile_options(renderer_project_options INTERFACE -Wall -Wextra -Wpedantic)
    if(RENDERER_WARNINGS_AS_ERRORS)
        target_compile_options(renderer_project_options INTERFACE -Werror)
    endif()
endif()

add_subdirectory(src/scene_contract)
add_subdirectory(src/render_core)
add_subdirectory(src/render_gl)
add_subdirectory(src/parametric_model)

if(RENDERER_BUILD_TEST_VIEWER)
    add_subdirectory(src/test_viewer)
endif()
