cmake_minimum_required(VERSION 3.16) # supports up to C++20, should work for this project

project(vec3 VERSION 0.1.0 LANGUAGES CXX)

# Benchmarks are meaningless without optimizations, so default to Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(VEC3_BUILD_TESTS "Build vec3 tests" ON)
option(VEC3_BUILD_BENCHMARKS "Build the naive chrono benchmark" ON)
option(VEC3_GOOGLE_BENCHMARK "Build the Google Benchmark port (Day 3 warm-up, downloads Google Benchmark)" OFF)

# Warnings for our own targets. AppleClang is a separate compiler ID from Clang
add_compile_options("$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>")

include(GNUInstallDirs)

# Create the header-only interface library
add_library(vec3 INTERFACE)
add_library(vec3::vec3 ALIAS vec3) # same name in-tree as after find_package

# Set the include directory for the library
target_include_directories(vec3 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Require C++20 for anyone using this library
target_compile_features(vec3 INTERFACE cxx_std_20)

# Install + export, so other projects can use find_package(vec3)
include(CMakePackageConfigHelpers)
set(VEC3_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/vec3)

install(TARGETS vec3 EXPORT vec3Targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT vec3Targets NAMESPACE vec3:: DESTINATION ${VEC3_CMAKE_DIR})

configure_package_config_file(cmake/vec3Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/vec3Config.cmake
    INSTALL_DESTINATION ${VEC3_CMAKE_DIR}
)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/vec3ConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT # header-only: any architecture can use it
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vec3Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/vec3ConfigVersion.cmake
    DESTINATION ${VEC3_CMAKE_DIR}
)

if(VEC3_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(VEC3_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
