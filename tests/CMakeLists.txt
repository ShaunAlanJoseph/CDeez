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
  MatchersTest.cpp)

target_link_libraries(cdeez_tests PRIVATE cdeez_lib Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(cdeez_tests)
