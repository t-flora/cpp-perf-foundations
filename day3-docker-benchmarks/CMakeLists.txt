cmake_minimum_required(VERSION 3.16)

project(sorting_benchmarks VERSION 0.1.0 LANGUAGES CXX)

# Benchmarks are meaningless without optimizations, so default to Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Google Benchmark, pinned to a release. Declared before our warning flags so they
# don't apply to its sources
include(FetchContent)
FetchContent_Declare(benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.5
    GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE) # don't pull in GoogleTest
FetchContent_MakeAvailable(benchmark)

add_compile_options("$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>")

# Header-only sorting library
add_library(sorting INTERFACE)
target_include_directories(sorting INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(sorting INTERFACE cxx_std_20)

enable_testing()
add_executable(test_sorting tests/test_sorting.cpp)
target_link_libraries(test_sorting PRIVATE sorting)
add_test(NAME sorting_tests COMMAND test_sorting)

add_executable(bench_sorting benchmarks/bench_sorting.cpp)
target_link_libraries(bench_sorting PRIVATE sorting benchmark::benchmark)
