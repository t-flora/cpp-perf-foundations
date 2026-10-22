# Day 3: Containerized Benchmark Runner
cmake_minimum_required(VERSION 3.16)

project(sorting_benchmarks VERSION 0.1.0 LANGUAGES CXX)

# TODO: Default to Release when no build type is given. Why does that matter here?

# TODO: C++20

# TODO: Fetch Google Benchmark
# include(FetchContent)
# FetchContent_Declare(benchmark
#     GIT_REPOSITORY https://github.com/google/benchmark.git
#     GIT_TAG        <a release tag, never main>
# )
# set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(benchmark)

# TODO: an INTERFACE library for include/sorting

# TODO: test_sorting, registered with CTest

# TODO: bench_sorting, linked against benchmark::benchmark (or benchmark::benchmark_main)
