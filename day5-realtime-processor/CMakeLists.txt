# Day 5: Real-Time Data Processor
cmake_minimum_required(VERSION 3.16)

project(realtime_processor VERSION 0.1.0 LANGUAGES CXX)

# TODO: Release by default, C++20, Google Benchmark via FetchContent (as in Days 3-4)

# TODO: an INTERFACE library for include/stream

# TODO: the processor executable (src/processor.cpp)

# TODO: tests, registered with CTest. They need the sample file's path, e.g.:
# target_compile_definitions(test_pipelines PRIVATE
#     SAMPLE_TRADES="${CMAKE_CURRENT_SOURCE_DIR}/data/sample_trades.csv")

# TODO: component benchmarks

# Reference tests (reference-tests/), once you've written your own. Keep this block last,
# so options like ThreadSanitizer apply to them too:
#   cmake -B build -DDAY5_REFERENCE_TESTS=ON && cmake --build build && (cd build && ctest)
option(DAY5_REFERENCE_TESTS "Build the solutions' tests in reference-tests/" OFF)
if(DAY5_REFERENCE_TESTS)
    enable_testing()
    add_subdirectory(reference-tests)
endif()
