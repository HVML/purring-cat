set(antlr4_folder "${CMAKE_CURRENT_SOURCE_DIR}/antlr4")
set(antlr4_cpp_folder "${antlr4_folder}/runtime/Cpp")
set(antlr4_jar "${antlr4_folder}/tool/target/antlr4-4.9.1-SNAPSHOT-complete.jar")
set(antlr4_demo "${antlr4_cpp_folder}/build/install/share/antlr4-demo")
set(antlr4_main "${antlr4_cpp_folder}/demo/Linux/main.cpp")

add_custom_target(antlr4 DEPENDS "${antlr4_jar}")
add_custom_command(OUTPUT "${antlr4_jar}"
  COMMAND mvn -f "${antlr4_folder}"
              -pl '!tool-testsuite,!runtime-testsuite/annotations,!runtime-testsuite/processors,!runtime-testsuite'
              -DskipTests=true install)


add_custom_target(antlr4demo ALL DEPENDS ${antlr4_demo})
add_custom_command(OUTPUT ${antlr4_demo}
  COMMAND "${CMAKE_COMMAND}" -E make_directory build
  COMMAND "${CMAKE_COMMAND}" -DANTLR_JAR_LOCATION="${antlr4_jar}"
                             -DWITH_DEMO=True
                             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                             -DCMAKE_INSTALL_PREFIX=build/install
                             -B build
  COMMAND "${CMAKE_COMMAND}" --build build
  COMMAND "${CMAKE_COMMAND}" --install build
  WORKING_DIRECTORY "${antlr4_cpp_folder}")

add_dependencies(antlr4demo antlr4)

