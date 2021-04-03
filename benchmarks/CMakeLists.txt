cmake_minimum_required(VERSION 3.5)

project(csm_benchmarks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(include_dirs ../include)

set(sources
    ../include/csm.h
    benchmarks.cpp)

include_directories(${include_dirs})
add_executable(benchmarks ${sources})
