cmake_minimum_required(VERSION 3.16)

project(tcp_echo VERSION 0.1.0 LANGUAGES CXX)

# Latency numbers are meaningless without optimizations, so default to Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    message(FATAL_ERROR "Day 7 uses POSIX sockets: build on Linux, macOS, or WSL")
endif()

# The server is multithreaded, so its tests are worth running under ThreadSanitizer too.
# Use a separate build directory: cmake -B build-tsan -DDAY7_TSAN=ON
option(DAY7_TSAN "Build with ThreadSanitizer" OFF)

find_package(Threads REQUIRED)

add_compile_options("$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>")
if(DAY7_TSAN)
    add_compile_options(-fsanitize=thread -g -O1)
    add_link_options(-fsanitize=thread)
endif()

# Header-only networking library
add_library(net INTERFACE)
target_include_directories(net INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(net INTERFACE cxx_std_20)
target_link_libraries(net INTERFACE Threads::Threads)

add_executable(echo_server src/echo_server.cpp)
target_link_libraries(echo_server PRIVATE net)
add_executable(echo_client src/echo_client.cpp)
target_link_libraries(echo_client PRIVATE net)
add_executable(transport_probe src/transport_probe.cpp)
target_link_libraries(transport_probe PRIVATE net)

enable_testing()
foreach(name framing socket echo_server)
    add_executable(test_${name} tests/test_${name}.cpp)
    target_link_libraries(test_${name} PRIVATE net)
    add_test(NAME ${name} COMMAND test_${name})
endforeach()
