add_library(parametric_model
    include/renderer/parametric_model/evaluation_result.h
    include/renderer/parametric_model/model_structure.h
    include/renderer/parametric_model/parametric_evaluator.h
    include/renderer/parametric_model/primitive_factory.h
    include/renderer/parametric_model/types.h
    src/model_structure.cpp
    src/parametric_evaluator.cpp
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
