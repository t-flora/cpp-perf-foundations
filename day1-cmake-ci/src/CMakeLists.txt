# Add calculator library as an interface library since it is header-only
add_library(calculator INTERFACE)

# Include current source directory for any target that links to calculator
target_include_directories(calculator INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add calculator_run executable
add_executable(calculator_run main.cpp)

# Link calculator_run to calculator
target_link_libraries(calculator_run PRIVATE calculator)

# Set output name for calculator_run
set_target_properties(calculator_run PROPERTIES OUTPUT_NAME "Calculator")