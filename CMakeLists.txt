cmake_minimum_required(VERSION 3.16)

project(CDeez VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(cdeez ${SOURCES})

target_include_directories(cdeez PRIVATE src)

find_package(SQLite3 REQUIRED)

target_link_libraries(cdeez PRIVATE SQLite3::SQLite3)
