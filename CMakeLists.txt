cmake_minimum_required(VERSION 3.29)
project(splinter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SPLINTER_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(splinter ${SPLINTER_SOURCES})
target_include_directories(splinter PRIVATE src)
