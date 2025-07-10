cmake_minimum_required(VERSION 3.16) # supports up to C++20, should work for this project

project(calculator VERSION 0.0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type-specific flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Add compile options for better debugging and warnings using generator expression
add_compile_options("$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-g>")

# Enable testing
enable_testing()

# Add src & test subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Register the test with CTest
add_test(NAME calculator_tests COMMAND test_calculator)