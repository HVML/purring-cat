find_program(PYTHON3 "python3" REQUIRED)
if(NOT PYTHON3)
    message(FATAL_ERROR "you need to install python3 first")
endif()

add_executable(hp main.c)
set_target_properties(hp PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
target_link_libraries(hp hvml_parser_static)

string(REPLACE "${PROJECT_SOURCE_DIR}" "" relative "${CMAKE_CURRENT_SOURCE_DIR}")

enable_testing()

if(MSVC)
    file(TO_NATIVE_PATH "$<TARGET_FILE:hp>" HP_PROC)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/tmp.file" tmp_file)
else()
    set(HP_PROC "${PROJECT_BINARY_DIR}${relative}/hp")
endif()

file(GLOB hvmls "test/*.hvml")
foreach(hvml ${hvmls})
if(MSVC)
    add_test(NAME ${hvml}_c
             COMMAND ${CMAKE_COMMAND} -E chdir $<TARGET_FILE_DIR:hp> $ENV{ComSpec} /c "$<TARGET_FILE_NAME:hp> ${hvml} > tmp.file && fc tmp.file ${hvml}.output")
else()
    add_test(NAME ${hvml}
             COMMAND sh -c "${HP_PROC} ${hvml} | diff - ${hvml}.output")
    add_test(NAME ${hvml}_c
             COMMAND sh -c "${HP_PROC} -c ${hvml} | diff - ${hvml}.output")
endif()
endforeach()

file(GLOB jsons "test/*.json")
foreach(json ${jsons})
if(MSVC)
    add_test(NAME ${json}_c
             COMMAND ${CMAKE_COMMAND} -E chdir $<TARGET_FILE_DIR:hp> $ENV{ComSpec} /c "$<TARGET_FILE_NAME:hp> -c ${json} | python3 -X utf8 -m json.tool > tmp.file && fc tmp.file ${json}.output")
else()
    add_test(NAME ${json}
             COMMAND sh -c "${HP_PROC} ${json} | python3 -m json.tool | diff - ${json}.output")
    add_test(NAME ${json}_c
             COMMAND sh -c "${HP_PROC} -c ${json} | python3 -m json.tool | diff - ${json}.output")
endif ()
endforeach()

file(GLOB utf8s "test/*.utf8")
foreach(utf8 ${utf8s})
if(MSVC)
    add_test(NAME ${utf8}
             COMMAND ${CMAKE_COMMAND} -E chdir $<TARGET_FILE_DIR:hp> $ENV{ComSpec} /c "$<TARGET_FILE_NAME:hp> ${utf8} > tmp.file && fc tmp.file ${utf8}.output")
else()
    add_test(NAME ${utf8}
             COMMAND sh -c "${HP_PROC} ${utf8} | diff - ${utf8}.output")
endif()
endforeach()

file(GLOB xpaths "test/*.xpath")
foreach(xpath ${xpaths})
if(MSVC)
    add_test(NAME ${xpath}_diff
             COMMAND ${CMAKE_COMMAND} -E chdir $<TARGET_FILE_DIR:hp> $ENV{ComSpec} /c "$<TARGET_FILE_NAME:hp> ${xpath} > tmp.file && fc tmp.file ${xpath}.output")
else()
    add_test(NAME ${xpath}
             COMMAND sh -c "${HP_PROC} ${xpath}")
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        add_test(NAME ${xpath}_v
                 COMMAND sh -c "valgrind --leak-check=full --show-leak-kinds=all -s ${HP_PROC} ${xpath}")
    endif ()
    add_test(NAME ${xpath}_diff
             COMMAND sh -c "${HP_PROC} ${xpath} | diff - ${xpath}.output")
endif()
endforeach()

file(GLOB antlr4s "test/*.xpath")
foreach(antlr4 ${antlr4s})
if(MSVC)
    add_test(NAME ${antlr4}_a_diff
             COMMAND ${CMAKE_COMMAND} -E chdir $<TARGET_FILE_DIR:hp> $ENV{ComSpec} /c "$<TARGET_FILE_NAME:hp> --antlr4 ${antlr4} > tmp.file && fc tmp.file ${antlr4}.output")
else()
    add_test(NAME ${antlr4}_a
             COMMAND sh -c "${HP_PROC} --antlr4 ${antlr4}")
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        add_test(NAME ${antlr4}_a_v
                 COMMAND sh -c "valgrind --leak-check=full --show-leak-kinds=all -s ${HP_PROC} --antlr4 ${antlr4}")
    endif ()
    add_test(NAME ${antlr4}_a_diff
             COMMAND sh -c "${HP_PROC} --antlr4 ${antlr4} | diff - ${antlr4}.output")
endif()
endforeach()

