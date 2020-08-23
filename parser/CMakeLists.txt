cmake_minimum_required(VERSION 3.16)
project(hvml_parser)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_subdirectory(src)

add_executable(hp main.c)
target_link_libraries(hp hvml_parser)

enable_testing()
add_test(NAME Runs COMMAND sh -c "./hp ../test/sample.hvml | diff - ../test/sample.hvml.output")

file(GLOB hvmls "test/*.hvml")
foreach(hvml ${hvmls})
  add_test(NAME ${hvml}, COMMAND sh -c "./hp ${hvml} | diff - ${hvml}.output")
endforeach()

