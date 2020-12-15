cmake_minimum_required(VERSION 3.16)
project(hvml)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
    # add_compile_options(-Wall -Wextra -Werror)
endif()

option(UPDATE_SUBMODULES
       "if you don't wanna update submodules, set -DUPDATE_SUBMODULES=OFF"
       ON)

if(UPDATE_SUBMODULES)
    add_custom_target(git-submodules ALL
        COMMAND git submodule update --init)
endif()

add_subdirectory(third-party)
add_subdirectory(parser)
add_subdirectory(interpreter)
add_subdirectory(json-eval)
add_subdirectory(json-objects)
add_subdirectory(test)
add_subdirectory(bindings)
