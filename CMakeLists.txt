cmake_minimum_required(VERSION 3.29)
project(splinter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SPLINTER_ENGINE_SOURCES CONFIGURE_DEPENDS src/engine/*.cpp)

add_library(splinter_engine STATIC ${SPLINTER_ENGINE_SOURCES})
target_include_directories(splinter_engine PUBLIC src)

if (MSVC)
    target_compile_options(splinter_engine PRIVATE /W4)
else ()
    target_compile_options(splinter_engine PRIVATE -Wall -Wextra)
endif ()

add_executable(splinter src/main.cpp)
target_link_libraries(splinter PRIVATE splinter_engine)

file(GLOB_RECURSE SPLINTER_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(splinter_tests ${SPLINTER_TEST_SOURCES})
target_link_libraries(splinter_tests PRIVATE splinter_engine)

enable_testing()
add_test(NAME splinter_tests COMMAND splinter_tests)
