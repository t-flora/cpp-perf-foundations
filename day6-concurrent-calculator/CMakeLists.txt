cmake_minimum_required(VERSION 3.16)

project(concurrent_processor VERSION 0.1.0 LANGUAGES CXX)

# Scaling numbers are meaningless without optimizations, so default to Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ThreadSanitizer instruments every memory access to detect data races at runtime.
# Use a separate build directory: cmake -B build-tsan -DDAY6_TSAN=ON
option(DAY6_TSAN "Build with ThreadSanitizer" OFF)

find_package(Threads REQUIRED)

# Google Benchmark, pinned to a release. Declared before our flags so they don't apply to it
include(FetchContent)
FetchContent_Declare(benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.5
    GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE) # don't pull in GoogleTest
FetchContent_MakeAvailable(benchmark)

add_compile_options("$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>")
if(DAY6_TSAN)
    add_compile_options(-fsanitize=thread -g -O1)
    add_link_options(-fsanitize=thread)
endif()

# Header-only concurrency library
add_library(concurrency INTERFACE)
target_include_directories(concurrency INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(concurrency INTERFACE cxx_std_20)
target_link_libraries(concurrency INTERFACE Threads::Threads)

enable_testing()
foreach(name bounded_queue thread_pool workloads)
    add_executable(test_${name} tests/test_${name}.cpp)
    target_link_libraries(test_${name} PRIVATE concurrency)
    add_test(NAME ${name} COMMAND test_${name})
endforeach()

add_executable(race_demo examples/race_demo.cpp)
target_link_libraries(race_demo PRIVATE Threads::Threads)

if(NOT DAY6_TSAN) # sanitizer builds are for correctness, not timing
    add_executable(bench_scaling benchmarks/bench_scaling.cpp)
    target_link_libraries(bench_scaling PRIVATE concurrency benchmark::benchmark)
endif()
