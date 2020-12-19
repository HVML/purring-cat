set(hvml_interpreter_src
    interpreter_basic.cpp
    interpreter_runtime.cpp
    observe_for.c
    adverb_property.c
    ext_tools.c
    str_tools.cpp
)

# static
add_library(hvml_interpreter_static STATIC ${hvml_interpreter_src})
target_include_directories(hvml_interpreter_static PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)
set_target_properties(hvml_interpreter_static PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
set_target_properties(hvml_interpreter_static PROPERTIES OUTPUT_NAME hvml_interpreter_static)

# shared
add_library(hvml_interpreter SHARED ${hvml_interpreter_src})
target_include_directories(hvml_interpreter PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(hvml_interpreter hvml_parser)