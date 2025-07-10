# Add test_calculator executable
add_executable(test_calculator test_calculator.cpp)

# Link test_calculator to calculator library
target_link_libraries(test_calculator PRIVATE calculator)
# The line below is not needed since calculator is an interface library and will be linked to the test_calculator target
# target_include_directories(test_calculator PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Define _DEBUG for debug builds using a generator expression
target_compile_definitions(test_calculator PRIVATE $<$<CONFIG:Debug>:_DEBUG>)