# Test executable
add_executable(test_vec3 test_vec3.cpp)

# Link against the header-only vec3 library
target_link_libraries(test_vec3 PRIVATE vec3)

# Register the test with CTest
add_test(NAME vec3_tests COMMAND test_vec3) 