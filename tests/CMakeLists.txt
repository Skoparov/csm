cmake_minimum_required(VERSION 3.5)

project(csm_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ../include
    test_framework)

set(sources
    detail/handler_mock.h
    detail/required_calls.h
    detail/transition_checker.h
    transition_table.h
    mocks.h
    tests.cpp)

add_executable(tests ${sources})
