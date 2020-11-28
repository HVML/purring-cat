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


find_program(ANTLR4 NAMES "antlr4" REQUIRED)
if(NOT ANTLR4)
    message(FATAL_ERROR "antlr4 is not installed yet, you may install it under ubuntu by typing: sudo apt install antlr4 antlr4-dev")
endif()

find_program(UUID NAMES "uuid" REQUIRED)
if(NOT UUID)
    message(FATAL_ERROR "uuid is not installed yet, you may install it under ubuntu by typing: sudo apt install uuid uuid-dev")
endif()

set(antlr4-xpath-GENERATED_SRC
   ${CMAKE_BINARY_DIR}/xpath/generated/xpathLexer.cpp
   ${CMAKE_BINARY_DIR}/xpath/generated/xpathParser.cpp
   ${CMAKE_BINARY_DIR}/xpath/generated/xpathBaseListener.cpp
   ${CMAKE_BINARY_DIR}/xpath/generated/xpathBaseVisitor.cpp
   ${CMAKE_BINARY_DIR}/xpath/generated/xpathListener.cpp
   ${CMAKE_BINARY_DIR}/xpath/generated/xpathVisitor.cpp
 )

foreach(src_file ${antlr4-xpath-GENERATED_SRC})
      set_source_files_properties(
          ${src_file}
          PROPERTIES
          GENERATED TRUE
          )
endforeach(src_file ${antlr4-xpath-GENERATED_SRC})

add_custom_target(xpathGenerateParser DEPENDS ${antlr4-xpath-GENERATED_SRC})
add_custom_command(OUTPUT ${antlr4-xpath-GENERATED_SRC}
   COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/xpath/generated/
   COMMAND antlr4 -Werror -Dlanguage=Cpp -listener -visitor -o ${CMAKE_BINARY_DIR}/xpath/generated/ ${PROJECT_SOURCE_DIR}/parser/src/xpath.g4
   DEPENDS ${PROJECT_SOURCE_DIR}/parser/src/xpath.g4
   )

set(antlr4-xpath_SRC ${antlr4-xpath-GENERATED_SRC})

#foreach(src_file ${antlr4-xpath_SRC})
#      set_source_files_properties(
#          ${src_file}
#          PROPERTIES
#          COMPILE_FLAGS "${COMPILE_FLAGS} -I/xxxxxxxusr/include/antlr4-runtime"
#          )
#endforeach(src_file ${antlr4-xpath_SRC})

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
    ${antlr4-xpath_SRC}
)

# static
add_library(hvml_parser_static STATIC ${hvml_parser_src} ${hvml_dom_xpath_src})
target_include_directories(hvml_parser_static PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(hvml_parser_static PRIVATE ".")
target_include_directories(hvml_parser_static PRIVATE "/usr/include/antlr4-runtime")
target_include_directories(hvml_parser_static PRIVATE ${CMAKE_BINARY_DIR}/xpath/generated)

target_link_libraries(hvml_parser_static PRIVATE m)
add_dependencies(hvml_parser_static xpathGenerateParser)
set_target_properties(hvml_parser_static PROPERTIES OUTPUT_NAME hvml_parser)

# shared
add_library(hvml_parser SHARED ${hvml_parser_src} ${hvml_dom_xpath_src})
target_include_directories(hvml_parser PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(hvml_parser PRIVATE ".")
target_include_directories(hvml_parser PRIVATE "/usr/include/antlr4-runtime")
target_include_directories(hvml_parser PRIVATE ${CMAKE_BINARY_DIR}/xpath/generated)
target_link_libraries(hvml_parser PRIVATE m)
add_dependencies(hvml_parser xpathGenerateParser)
set_target_properties(hvml_parser PROPERTIES OUTPUT_NAME hvml_parser)

