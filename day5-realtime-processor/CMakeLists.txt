cmake_minimum_required(VERSION 3.16)

project(realtime_processor VERSION 0.1.0 LANGUAGES CXX)

# Latency numbers are meaningless without optimizations, so default to Release
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

# Header-only stream-processing library
add_library(stream INTERFACE)
target_include_directories(stream INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(stream INTERFACE cxx_std_20)

add_executable(processor src/processor.cpp)
target_link_libraries(processor PRIVATE stream)

enable_testing()
foreach(name ring_buffer parser window histogram pipelines)
    add_executable(test_${name} tests/test_${name}.cpp)
    target_link_libraries(test_${name} PRIVATE stream)
    target_compile_definitions(test_${name} PRIVATE
        SAMPLE_TRADES="${CMAKE_CURRENT_SOURCE_DIR}/data/sample_trades.csv")
    add_test(NAME ${name} COMMAND test_${name})
endforeach()

add_executable(bench_components benchmarks/bench_components.cpp)
target_link_libraries(bench_components PRIVATE stream benchmark::benchmark_main)
