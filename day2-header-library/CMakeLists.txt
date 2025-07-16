cmake_minimum_required(VERSION 3.16) # supports up to C++20, should work for this project

project(vec3 VERSION 0.0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type-specific flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Add compile options for better debugging and warnings using generator expression
add_compile_options("$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-g>")

# Create the header-only interface library
add_library(vec3 INTERFACE)

# Set the include directory for the library
target_include_directories(vec3 INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Require C++20 for anyone using this library
target_compile_features(vec3 INTERFACE cxx_std_20)

# Enable testing
enable_testing()

# Add test subdirectory if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()

# Add benchmarks subdirectory if it exists  
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt)
    add_subdirectory(benchmarks)
endif()