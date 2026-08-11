cmake_minimum_required(VERSION 3.29)
project(splinter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SPLINTER_ENGINE_SOURCES CONFIGURE_DEPENDS src/engine/*.cpp)

function(splinter_set_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else ()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif ()
endfunction()

add_library(splinter_engine STATIC ${SPLINTER_ENGINE_SOURCES})
target_include_directories(splinter_engine PUBLIC src)
splinter_set_warnings(splinter_engine)

add_executable(splinter src/main.cpp)
target_link_libraries(splinter PRIVATE splinter_engine)
splinter_set_warnings(splinter)

file(GLOB_RECURSE SPLINTER_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(splinter_tests ${SPLINTER_TEST_SOURCES})
target_link_libraries(splinter_tests PRIVATE splinter_engine)
splinter_set_warnings(splinter_tests)

enable_testing()
add_test(NAME splinter_tests COMMAND splinter_tests)
