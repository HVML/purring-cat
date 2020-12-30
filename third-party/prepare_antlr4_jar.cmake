message(STATUS "***** ${antlr4_jar} ***** ${antlr4_jar_prebuilt} ***** ${antlr4_folder}")

if (NOT EXISTS "${antlr4_jar}" AND EXISTS "${antlr4_jar_prebuilt}")
    file(COPY "${antlr4_jar_prebuilt}" DESTINATION "${antlr4_jar_dir}")
else()    
    execute_process(mvn -f "${antlr4_folder}"
#              -pl '!tool-testsuite,!runtime-testsuite/annotations,!runtime-testsuite/processors,!runtime-testsuite'
                 -DskipTests=true install)
endif()

