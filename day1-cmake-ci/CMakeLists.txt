# Day 1: CMake + CI Hello World
cmake_minimum_required(VERSION 3.16)

# TODO: Set your project name and version
project(calculator_project VERSION 1.0)

# TODO: Set C++ standard
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TODO: Create a library target for your calculator
# Header-only? Use an INTERFACE library instead: add_library(calculator INTERFACE)
# add_library(calculator src/calculator.cpp)
# target_include_directories(calculator PUBLIC src)

# TODO: Create an executable target for the demo
# add_executable(calculator_demo src/main.cpp)
# target_link_libraries(calculator_demo PRIVATE calculator)

# TODO: Add testing support
# enable_testing()
# add_executable(test_calculator tests/test_calculator.cpp)
# target_link_libraries(test_calculator PRIVATE calculator)
# add_test(NAME calculator_tests COMMAND test_calculator)
#
# Prefer GoogleTest? find_package(GTest REQUIRED) and link GTest::gtest_main instead.
