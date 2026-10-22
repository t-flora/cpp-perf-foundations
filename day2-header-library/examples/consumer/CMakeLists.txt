# A separate project that uses an *installed* vec3, the way a real user would:
#   cmake --install <vec3-build> --prefix <prefix>
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=<prefix>
cmake_minimum_required(VERSION 3.16)
project(vec3_consumer LANGUAGES CXX)

find_package(vec3 0.1 REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE vec3::vec3) # brings include path + C++20 along

enable_testing()
add_test(NAME consumer COMMAND consumer)
