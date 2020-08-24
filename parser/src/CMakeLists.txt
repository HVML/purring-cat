set(hvml_parser_src
    hvml_json_parser.c
    hvml_log.c
    hvml_parser.c
)

# static
add_library(hvml_parser_static STATIC ${hvml_parser_src})
target_include_directories(hvml_parser_static PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)
set_target_properties(hvml_parser_static PROPERTIES OUTPUT_NAME hvml_parser)

# shared
add_library(hvml_parser SHARED ${hvml_parser_src})
target_include_directories(hvml_parser PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)

