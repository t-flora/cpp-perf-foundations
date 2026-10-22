# Naive std::chrono timing loop (Day 2)
add_executable(simple_benchmark simple_benchmark.cpp)
target_link_libraries(simple_benchmark PRIVATE vec3::vec3)

# Google Benchmark port (Day 3 warm-up): cmake -B build -DVEC3_GOOGLE_BENCHMARK=ON
if(VEC3_GOOGLE_BENCHMARK)
    include(FetchContent)
    FetchContent_Declare(benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.5 # pin a release, never main
        GIT_SHALLOW    TRUE
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE) # don't pull in GoogleTest
    FetchContent_MakeAvailable(benchmark)

    add_executable(bench_vec3 bench_vec3.cpp)
    target_link_libraries(bench_vec3 PRIVATE vec3::vec3 benchmark::benchmark_main)
endif()
