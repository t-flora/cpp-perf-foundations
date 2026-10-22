cmake_minimum_required(VERSION 3.16)

project(container_benchmarks VERSION 0.1.0 LANGUAGES CXX)

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

# Header-only workloads library (containers under test + input generators)
add_library(workloads INTERFACE)
target_include_directories(workloads INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(workloads INTERFACE cxx_std_20)

enable_testing()
add_executable(test_workloads tests/test_workloads.cpp)
target_link_libraries(test_workloads PRIVATE workloads)
add_test(NAME workload_tests COMMAND test_workloads)

add_executable(bench_containers benchmarks/bench_containers.cpp)
target_link_libraries(bench_containers PRIVATE workloads benchmark::benchmark)
