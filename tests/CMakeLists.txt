find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
  include(FetchContent)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.0)
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

add_executable(cdeez_tests
  StringUtilsTest.cpp
  PathUtilsTest.cpp
  MatchersTest.cpp
  DbTest.cpp
  ProcessorTest.cpp
  ShellInitTest.cpp
  VersionTest.cpp)

target_link_libraries(cdeez_tests PRIVATE cdeez_lib Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(cdeez_tests)

# Drives the real cd wrapper in whichever of zsh, bash and fish are installed,
# and asserts on the directory it lands in. Shells that are missing are
# reported as skipped rather than failing the run.
add_test(NAME shell_integration
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/shell/run.sh $<TARGET_FILE:cdeez>)
