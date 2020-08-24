add_executable(hp main.c)
target_link_libraries(hp hvml_parser_static)

string(REPLACE "${PROJECT_SOURCE_DIR}" "" relative "${CMAKE_CURRENT_SOURCE_DIR}")

enable_testing()

file(GLOB hvmls "test/*.hvml")
foreach(hvml ${hvmls})
  add_test(NAME ${hvml}, COMMAND sh -c "${PROJECT_BINARY_DIR}/${relative}/hp ${hvml} | diff - ${hvml}.output")
endforeach()

