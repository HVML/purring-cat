cmake_minimum_required(VERSION 3.14)
project(hvml)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_subdirectory(parser)
add_subdirectory(interpreter)
add_subdirectory(json-eval)
add_subdirectory(json-objects)
add_subdirectory(test)
add_subdirectory(bindings)
