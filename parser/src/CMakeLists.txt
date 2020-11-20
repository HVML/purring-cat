find_package(FLEX)
if(NOT FLEX_FOUND)
    message(FATAL_ERROR "you need to install flex first")
endif()
find_package(BISON)
if(NOT BISON_FOUND)
    message(FATAL_ERROR "you need to install bison first")
endif()

# xpath parser/scanner with flex/bison
BISON_TARGET(hvmlDomXPathParser
             hvml_dom_xpath_parser.y
             ${CMAKE_CURRENT_BINARY_DIR}/hvml_dom_xpath_parser.tab.c
            COMPILE_FLAGS "--graph=${CMAKE_CURRENT_BINARY_DIR}/hvml_dom_xpath_parser.tab.gv"
)
FLEX_TARGET(hvmlDomXPathScanner
            hvml_dom_xpath_scanner.l
            ${CMAKE_CURRENT_BINARY_DIR}/hvml_dom_xpath_scanner.lex.c
            COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/hvml_dom_xpath_scanner.lex.h"
)
ADD_FLEX_BISON_DEPENDENCY(hvmlDomXPathScanner hvmlDomXPathParser)

set(hvml_dom_xpath_src
    ${FLEX_hvmlDomXPathScanner_OUTPUTS}
    ${BISON_hvmlDomXPathParser_OUTPUTS}
)

set(hvml_parser_src
    hvml_dom.c
    hvml_dom_printf.c
    hvml_dom_xpath_parser.c
    hvml_jo.c
    hvml_jo_printf.c
    hvml_json_parser.c
    hvml_log.c
    hvml_parser.c
    hvml_string.c
    hvml_utf8.c
)

# static
add_library(hvml_parser_static STATIC ${hvml_parser_src} ${hvml_dom_xpath_src})
target_include_directories(hvml_parser_static PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)
target_include_directories(hvml_parser_static PRIVATE
                           "."
)
set_target_properties(hvml_parser_static PROPERTIES OUTPUT_NAME hvml_parser)

# shared
add_library(hvml_parser SHARED ${hvml_parser_src} ${hvml_dom_xpath_src})
target_include_directories(hvml_parser PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
)
target_include_directories(hvml_parser PRIVATE
                           "."
)

