add_executable(interpreter main.cpp)
set_target_properties(interpreter PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
target_link_libraries(interpreter hvml_interpreter_static hvml_parser_static)