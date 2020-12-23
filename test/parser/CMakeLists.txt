find_program(PYTHON3 "python3" REQUIRED)
if(NOT PYTHON3)
    message(FATAL_ERROR "you need to install python3 first")
endif()

add_executable(hp main.c)
set_target_properties(hp PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
target_link_libraries(hp hvml_parser_static)

string(REPLACE "${PROJECT_SOURCE_DIR}" "" relative "${CMAKE_CURRENT_SOURCE_DIR}")

enable_testing()

set(HP_PROC "${PROJECT_BINARY_DIR}${relative}/hp")
if (MSVC)
    set(TMP_FILE "${CMAKE_CURRENT_BINARY_DIR}"/tmp.file)
endif()

file(GLOB hvmls "test/*.hvml")
foreach(hvml ${hvmls})
if (MSVC)
    add_test(NAME ${hvml}
             COMMAND sh -c "${HP_PROC} ${hvml} > ${TMP_FILE} && fc ${TMP_FILE} ${hvml}.output")
    add_test(NAME ${hvml}_c
             COMMAND sh -c "${HP_PROC} -c ${hvml} > ${TMP_FILE} && fc ${TMP_FILE} ${hvml}.output")
else()
    add_test(NAME ${hvml}
             COMMAND sh -c "${HP_PROC} ${hvml} | diff - ${hvml}.output")
    add_test(NAME ${hvml}_c
             COMMAND sh -c "${HP_PROC} -c ${hvml} | diff - ${hvml}.output")
endif()
endforeach()

file(GLOB jsons "test/*.json")
foreach(json ${jsons})
if (MSVC)
    add_test(NAME ${json}
             COMMAND sh -c "${HP_PROC} ${json} | python3 -m json.tool > ${TMP_FILE} && fc ${TMP_FILE} ${json}.output")
    add_test(NAME ${json}_c
             COMMAND sh -c "${HP_PROC} -c ${json} | python3 -m json.tool > ${TMP_FILE} && fc ${TMP_FILE} ${json}.output")
else ()
    add_test(NAME ${json}
             COMMAND sh -c "${HP_PROC} ${json} | python3 -m json.tool | diff - ${json}.output")
    add_test(NAME ${json}_c
             COMMAND sh -c "${HP_PROC} -c ${json} | python3 -m json.tool | diff - ${json}.output")
endif ()
endforeach()

file(GLOB utf8s "test/*.utf8")
foreach(utf8 ${utf8s})
if (MSVC)
    add_test(NAME ${utf8}
             COMMAND sh -c "${HP_PROC} ${utf8} > ${TMP_FILE} && fc ${TMP_FILE} ${utf8}.output")
else()
    add_test(NAME ${utf8}
             COMMAND sh -c "${HP_PROC} ${utf8} | diff - ${utf8}.output")
endif()
endforeach()

file(GLOB xpaths "test/*.xpath")
foreach(xpath ${xpaths})
if (MSVC)
    add_test(NAME ${xpath}
             COMMAND sh -c "${HP_PROC} ${xpath}")
    add_test(NAME ${xpath}_diff
             COMMAND sh -c "${HP_PROC} ${xpath} > ${TMP_FILE} && fc ${TMP_FILE} ${xpath}.output")
else ()
    add_test(NAME ${xpath}
             COMMAND sh -c "${HP_PROC} ${xpath}")
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        add_test(NAME ${xpath}_v
                 COMMAND sh -c "valgrind --leak-check=full --show-leak-kinds=all -s ${HP_PROC} ${xpath}")
    endif ()
    add_test(NAME ${xpath}_diff
             COMMAND sh -c "${HP_PROC} ${xpath} | diff - ${xpath}.output")
endif ()
endforeach()

file(GLOB antlr4s "test/*.xpath")
foreach(antlr4 ${antlr4s})
if (MSVC)
    add_test(NAME ${antlr4}_a
             COMMAND sh -c "${HP_PROC} --antlr4 ${antlr4}")
    add_test(NAME ${antlr4}_a_diff
             COMMAND sh -c "${HP_PROC} --antlr4 ${antlr4} > ${TMP_FILE} && fc ${TMP_FILE} ${antlr4}.output")
else ()
    add_test(NAME ${antlr4}_a
             COMMAND sh -c "${HP_PROC} --antlr4 ${antlr4}")
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        add_test(NAME ${antlr4}_a_v
                 COMMAND sh -c "valgrind --leak-check=full --show-leak-kinds=all -s ${HP_PROC} --antlr4 ${antlr4}")
    endif ()
    add_test(NAME ${antlr4}_a_diff
             COMMAND sh -c "${HP_PROC} --antlr4 ${antlr4} | diff - ${antlr4}.output")
endif ()
endforeach()

