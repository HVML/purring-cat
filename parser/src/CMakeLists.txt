cmake_minimum_required(VERSION 3.16)

#include_directories(../inc)
add_library(hvml_parser
            hvml_json_parser.c
            hvml_log.c
            hvml_parser.c
)
target_include_directories(hvml_parser PUBLIC
                           "${PROJECT_SOURCE_DIR}/inc"
)

