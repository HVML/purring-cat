find_program(PYTHON3 "python3" REQUIRED)
if(NOT PYTHON3)
    message(FATAL_ERROR "you need to install python3 first")
endif()

add_executable(hp main.c)
target_link_libraries(hp hvml_parser_static)

string(REPLACE "${PROJECT_SOURCE_DIR}" "" relative "${CMAKE_CURRENT_SOURCE_DIR}")

enable_testing()

file(GLOB hvmls "test/*.hvml")
foreach(hvml ${hvmls})
    add_test(NAME ${hvml}, COMMAND sh -c "${PROJECT_BINARY_DIR}${relative}/hp ${hvml} | diff - ${hvml}.output")
endforeach()

file(GLOB jsons "test/*.json")
foreach(json ${jsons})
    add_test(NAME ${json}, COMMAND sh -c "${PROJECT_BINARY_DIR}${relative}/hp ${json} | python3 -m json.tool | diff - ${json}.output")
endforeach()

file(GLOB utf8s "test/*.utf8")
foreach(utf8 ${utf8s})
    add_test(NAME ${utf8}, COMMAND sh -c "${PROJECT_BINARY_DIR}${relative}/hp ${utf8} | diff - ${utf8}.output")
endforeach()

