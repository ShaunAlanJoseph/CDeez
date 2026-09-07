cmake_minimum_required(VERSION 3.16)

project(CDeez VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CDEEZ_BUILD_TESTS "Build the test suite" ON)

configure_file(src/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h @ONLY)

find_package(SQLite3 REQUIRED)

add_library(cdeez_lib STATIC
  src/core/Processor.cpp
  src/db/db.cpp
  src/fuzzy/DamerauLevenshteinMatcher.cpp
  src/fuzzy/KMPMatcher.cpp
  src/fuzzy/MatcherBase.cpp
  src/fuzzy/OrderMatches.cpp
  src/fuzzy/SequenceMatcher.cpp
  src/fuzzy/SuffixMatcher.cpp
  src/fuzzy/TokenizedMatcher.cpp
  src/shell/ShellInit.cpp
  src/utils/PathUtils.cpp
  src/utils/StringUtils.cpp)

target_include_directories(cdeez_lib PUBLIC src
                                          ${CMAKE_CURRENT_BINARY_DIR}/generated)

target_link_libraries(cdeez_lib PUBLIC SQLite::SQLite3)

add_executable(cdeez src/main.cpp)

target_link_libraries(cdeez PRIVATE cdeez_lib)

include(GNUInstallDirs)

install(TARGETS cdeez RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

find_program(SCDOC scdoc)

if(SCDOC)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cdeez.1
    COMMAND ${SCDOC} < ${CMAKE_CURRENT_SOURCE_DIR}/docs/cdeez.1.scd
            > ${CMAKE_CURRENT_BINARY_DIR}/cdeez.1
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/docs/cdeez.1.scd
    COMMENT "Generating cdeez.1")

  add_custom_target(cdeez_man ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cdeez.1)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cdeez.1
          DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
else()
  message(STATUS "scdoc not found - the man page will not be built")
endif()

if(CDEEZ_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
