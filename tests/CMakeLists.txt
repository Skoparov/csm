cmake_minimum_required(VERSION 3.5)

project(csm_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(include_dirs
    ../include
    ../lib)

set(sources
    test_helpers.h
    tests.cpp)

include_directories(${include_dirs})
add_executable(tests ${sources})
