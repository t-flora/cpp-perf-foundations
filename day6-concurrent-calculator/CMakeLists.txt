# Day 6: Concurrent Task Processor
cmake_minimum_required(VERSION 3.16)

project(concurrent_processor VERSION 0.1.0 LANGUAGES CXX)

# TODO: Release by default, C++20, Google Benchmark via FetchContent (as in Days 3-5)

# TODO: find_package(Threads REQUIRED) and link Threads::Threads

# TODO: an option to build with ThreadSanitizer, e.g.
# option(DAY6_TSAN "Build with ThreadSanitizer" OFF)
# if(DAY6_TSAN)
#     add_compile_options(-fsanitize=thread -g -O1)
#     add_link_options(-fsanitize=thread)
# endif()

# TODO: an INTERFACE library for include/concurrency, tests registered with CTest,
#       examples/race_demo, and the scaling benchmarks

# Reference tests (reference-tests/), once you've written your own. Keep this block last,
# so options like ThreadSanitizer apply to them too:
#   cmake -B build -DDAY6_REFERENCE_TESTS=ON && cmake --build build && (cd build && ctest)
option(DAY6_REFERENCE_TESTS "Build the solutions' tests in reference-tests/" OFF)
if(DAY6_REFERENCE_TESTS)
    enable_testing()
    add_subdirectory(reference-tests)
endif()
