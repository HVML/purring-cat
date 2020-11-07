set(hvml_interpreter_src
    interpreter_basic.cpp
    interpreter_to_three_part.cpp
    observe_for.c
    adverb_property.c
    ext_tools.c
)

# static
add_library(hvml_interpreter_static STATIC ${hvml_interpreter_src})
target_include_directories(hvml_interpreter_static PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)
set_target_properties(hvml_interpreter_static PROPERTIES OUTPUT_NAME hvml_interpreter)

# shared
add_library(hvml_interpreter SHARED ${hvml_interpreter_src})
target_include_directories(hvml_interpreter PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)