# Reference tests: the solutions' tests for the interfaces in the starter headers.
# The .cpp and .hpp files here are copied from the main branch (scripts/template_sync.py
# there), so don't edit them. Turn them on with -DDAY5_REFERENCE_TESTS=ON.

foreach(name ring_buffer parser)
    add_executable(reference_test_${name} test_${name}.cpp)
    target_include_directories(reference_test_${name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_features(reference_test_${name} PRIVATE cxx_std_20)
endforeach()

# A stub that blocks forever shouldn't hang ctest
foreach(name ring_buffer parser)
    add_test(NAME reference_${name} COMMAND reference_test_${name})
    set_tests_properties(reference_${name} PROPERTIES TIMEOUT 60)
endforeach()
