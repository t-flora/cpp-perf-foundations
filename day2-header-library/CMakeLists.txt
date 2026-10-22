# Day 2: Header-Only Math Library
cmake_minimum_required(VERSION 3.16)

project(vec3 VERSION 0.1.0 LANGUAGES CXX)

# TODO: Create the header-only INTERFACE library, plus an alias so in-tree users and
# find_package users spell it the same way
# add_library(vec3 INTERFACE)
# add_library(vec3::vec3 ALIAS vec3)

# TODO: Include directories: one path while building, another after install
# target_include_directories(vec3 INTERFACE
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#     $<INSTALL_INTERFACE:include>
# )

# TODO: Require C++20 for anyone who links vec3
# target_compile_features(vec3 INTERFACE ...)

# TODO: Install + export so other projects can find_package(vec3)
# include(GNUInstallDirs)
# include(CMakePackageConfigHelpers)
# install(TARGETS vec3 EXPORT vec3Targets)
# install(DIRECTORY include/ DESTINATION ...)
# install(EXPORT vec3Targets NAMESPACE vec3:: DESTINATION ...)
# configure_package_config_file(...)
# write_basic_package_version_file(...)
# install(FILES ... DESTINATION ...)

# TODO: Tests and benchmarks
# enable_testing()
# add_subdirectory(tests)
# add_subdirectory(benchmarks)
