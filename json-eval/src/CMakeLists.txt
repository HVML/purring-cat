find_package(FLEX)
if(NOT FLEX_FOUND)
    message(FATAL_ERROR "you need to install flex first")
endif()
find_package(BISON)
if(NOT BISON_FOUND)
    message(FATAL_ERROR "you need to install bison first")
endif()

# json-eval parser/scanner with flex/bison
BISON_TARGET(hvmlJsonEvalParser
             hvml_je_parser.y
             ${CMAKE_CURRENT_BINARY_DIR}/hvml_je_parser.tab.c
            COMPILE_FLAGS "--graph=${CMAKE_CURRENT_BINARY_DIR}/hvml_je_parser.tab.gv"
)
FLEX_TARGET(hvmlJsonEvalScanner
            hvml_je_scanner.l
            ${CMAKE_CURRENT_BINARY_DIR}/hvml_je_scanner.c
            COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/hvml_je_scanner.lex.h"
)
ADD_FLEX_BISON_DEPENDENCY(hvmlJsonEvalScanner hvmlJsonEvalParser)


set(hvml_je_src
    ${BISON_hvmlJsonEvalParser_OUTPUTS}
    ${FLEX_hvmlJsonEvalScanner_OUTPUTS}
)

# static
add_library(hvml_je_static STATIC ${hvml_je_src})
target_include_directories(hvml_je_static PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)
set_target_properties(hvml_je_static PROPERTIES OUTPUT_NAME hvml_je)

# shared
add_library(hvml_je SHARED ${hvml_je_src})
target_include_directories(hvml_je PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)

