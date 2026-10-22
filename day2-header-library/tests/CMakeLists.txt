# Runtime tests
add_executable(test_basics test_vec3_basics.cpp)
target_link_libraries(test_basics PRIVATE vec3::vec3)
add_test(NAME vec3_tests COMMAND test_basics)

# Compile-time tests: this target building at all is the test
add_executable(test_constexpr test_vec3_constexpr.cpp)
target_link_libraries(test_constexpr PRIVATE vec3::vec3)
add_test(NAME vec3_constexpr_tests COMMAND test_constexpr)

# Packaging test: install vec3, then build examples/consumer against it with find_package
add_test(NAME vec3_find_package
    COMMAND ${CMAKE_COMMAND}
        -DBUILD_DIR=${PROJECT_BINARY_DIR}
        -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DCONFIG=$<CONFIG>
        -DCTEST=${CMAKE_CTEST_COMMAND}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_find_package.cmake
)
