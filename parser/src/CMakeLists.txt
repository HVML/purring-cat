add_library(hvml_parser
            hvml_json_parser.c
            hvml_log.c
            hvml_parser.c
)
target_include_directories(hvml_parser PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)

