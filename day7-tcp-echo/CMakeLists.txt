cmake_minimum_required(VERSION 3.16)

project(tcp_echo VERSION 0.1.0 LANGUAGES CXX)

# TODO: Release by default, C++20, warnings (as in Days 3-6)

# TODO: find_package(Threads REQUIRED) and link Threads::Threads

# TODO: a ThreadSanitizer option, as in Day 6

# TODO: an INTERFACE library for include/net, the echo_server and echo_client
#       executables, and tests registered with CTest

# Reference tests (reference-tests/), once you've written your own. Keep this block last,
# so options like ThreadSanitizer apply to them too:
#   cmake -B build -DDAY7_REFERENCE_TESTS=ON && cmake --build build && (cd build && ctest)
option(DAY7_REFERENCE_TESTS "Build the solutions' tests in reference-tests/" OFF)
if(DAY7_REFERENCE_TESTS)
    enable_testing()
    add_subdirectory(reference-tests)
endif()
