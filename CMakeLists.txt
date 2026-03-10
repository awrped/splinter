cmake_minimum_required(VERSION 4.1)
project(splinter)

set(CMAKE_CXX_STANDARD 20)

add_library(splinter SHARED src/library.cpp)
