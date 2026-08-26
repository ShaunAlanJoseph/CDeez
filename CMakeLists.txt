cmake_minimum_required(VERSION 3.16)

project(CDeez VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CDEEZ_BUILD_TESTS "Build the test suite" ON)

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
  src/utils/PathUtils.cpp
  src/utils/StringUtils.cpp)

target_include_directories(cdeez_lib PUBLIC src)

target_link_libraries(cdeez_lib PUBLIC SQLite3::SQLite3)

add_executable(cdeez src/main.cpp)

target_link_libraries(cdeez PRIVATE cdeez_lib)

if(CDEEZ_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
