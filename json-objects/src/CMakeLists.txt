set(hvml_jo_src
    hvml_jo.c
)

# static
add_library(hvml_jo_static STATIC ${hvml_jo_src})
target_include_directories(hvml_jo_static PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)
set_target_properties(hvml_jo_static PROPERTIES OUTPUT_NAME hvml_jo)

# shared
add_library(hvml_jo SHARED ${hvml_jo_src})
target_include_directories(hvml_jo PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)

