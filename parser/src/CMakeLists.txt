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

# We need --wincompat option in Windows.
if (WIN32)
   set(flex_flags "--header-file=${CMAKE_CURRENT_BINARY_DIR}/hvml_dom_xpath_scanner.lex.h --wincompat")
else()
   set(flex_flags "--header-file=${CMAKE_CURRENT_BINARY_DIR}/hvml_dom_xpath_scanner.lex.h")
endif()

message(STATUS ${flex_flags})

FLEX_TARGET(hvmlDomXPathScanner
            hvml_dom_xpath_scanner.l
            ${CMAKE_CURRENT_BINARY_DIR}/hvml_dom_xpath_scanner.lex.c
            COMPILE_FLAGS ${flex_flags}
)
ADD_FLEX_BISON_DEPENDENCY(hvmlDomXPathScanner hvmlDomXPathParser)

set(hvml_dom_xpath_src
    ${FLEX_hvmlDomXPathScanner_OUTPUTS}
    ${BISON_hvmlDomXPathParser_OUTPUTS}
)

set(xpath_src "${CMAKE_CURRENT_SOURCE_DIR}/xpath.g4")
set(xpath_cpp "${CMAKE_CURRENT_BINARY_DIR}/xpathBaseListener.cpp"
              "${CMAKE_CURRENT_BINARY_DIR}/xpathBaseVisitor.cpp"
              "${CMAKE_CURRENT_BINARY_DIR}/xpathLexer.cpp"
              "${CMAKE_CURRENT_BINARY_DIR}/xpathListener.cpp"
              "${CMAKE_CURRENT_BINARY_DIR}/xpathParser.cpp"
              "${CMAKE_CURRENT_BINARY_DIR}/xpathVisitor.cpp")
set(xpath_h "${CMAKE_CURRENT_BINARY_DIR}/xpathBaseListener.h"
            "${CMAKE_CURRENT_BINARY_DIR}/xpathBaseVisitor.h"
            "${CMAKE_CURRENT_BINARY_DIR}/xpathLexer.h"
            "${CMAKE_CURRENT_BINARY_DIR}/xpathListener.h"
            "${CMAKE_CURRENT_BINARY_DIR}/xpathParser.h"
            "${CMAKE_CURRENT_BINARY_DIR}/xpathVisitor.h")
set(xpath_misc  "${CMAKE_CURRENT_BINARY_DIR}/xpath.interp"
                "${CMAKE_CURRENT_BINARY_DIR}/xpathLexer.interp"
                "${CMAKE_CURRENT_BINARY_DIR}/xpathLexer.tokens"
                "${CMAKE_CURRENT_BINARY_DIR}/xpath.tokens")

set(hvml_cpp ${xpath_cpp}
             hvml_dom_xpath_qry.cpp
             xpathDomVisitor.cpp)

set(antlr4_install "${PROJECT_SOURCE_DIR}/third-party/antlr4/runtime/Cpp/build/install")
set(antlr4_jar "${PROJECT_SOURCE_DIR}/third-party/antlr4/tool/target/antlr4-4.9.1-SNAPSHOT-complete.jar")

add_custom_target(xpath DEPENDS ${xpath_cpp} ${xpath_h} ${xpath_misc})
add_custom_command(OUTPUT ${xpath_cpp} ${xpath_h} ${xpath_misc}
    COMMAND java -jar ${antlr4_jar} -Werror -Dlanguage=Cpp -listener -visitor
            -o ${CMAKE_CURRENT_BINARY_DIR} ${xpath_src}
    DEPENDS ${xpath_src})


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
    ${hvml_cpp}
)

set_source_files_properties(${hvml_cpp} PROPERTIES COMPILE_FLAGS -Wno-attributes)

# static
add_library(hvml_parser_static STATIC ${hvml_parser_src} ${hvml_dom_xpath_src})
# To link to static library of antlr4 in Windows, ANTLR4CPP_STATIC is required
target_compile_options(hvml_parser_static PRIVATE "-DANTLR4CPP_STATIC")
# Since antlr4-runtime-static is compiled with /MT, this library also should do.
set_target_properties(hvml_parser_static PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
target_include_directories(hvml_parser_static PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(hvml_parser_static PRIVATE ".")
target_include_directories(hvml_parser_static PRIVATE "${antlr4_install}/include/antlr4-runtime")
target_include_directories(hvml_parser_static PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(hvml_parser_static $<IF:$<BOOL:${WIN32}>,${antlr4_install}/lib/antlr4-runtime-static.lib,${antlr4_install}/lib/libantlr4-runtime.a>)
set_target_properties(hvml_parser_static PROPERTIES OUTPUT_NAME hvml_parser_static)
add_dependencies(hvml_parser_static xpath)

# shared
add_library(hvml_parser SHARED ${hvml_parser_src} ${hvml_dom_xpath_src})
target_include_directories(hvml_parser PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(hvml_parser PRIVATE ".")
target_include_directories(hvml_parser PRIVATE "${antlr4_install}/include/antlr4-runtime")
target_include_directories(hvml_parser PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_directories(hvml_parser PRIVATE "${antlr4_install}/lib")
target_link_libraries(hvml_parser PRIVATE antlr4-runtime)
add_dependencies(hvml_parser xpath)

