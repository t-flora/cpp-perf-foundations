// Google Benchmark port of simple_benchmark.cpp (Day 3, milestone 1).
#include <benchmark/benchmark.h>
#include <vec3/vec3.hpp>

using vec3::Vec3f;

// v1 and v2 never change inside the loop, so the compiler may compute `v1 + v2` once
// and only repeat the store. That is what simple_benchmark.cpp ends up timing.
// DoNotOptimize on the *inputs* tells the compiler they may have changed, which forces
// it to redo the work every iteration.
template <typename Op>
static void BM_Op(benchmark::State& state, Op op) {
    Vec3f v1(1.0f, 2.0f, 3.0f);
    Vec3f v2(4.0f, 5.0f, 6.0f);
    for (auto _ : state) {
        benchmark::DoNotOptimize(v1);
        benchmark::DoNotOptimize(v2);
        auto result = op(v1, v2);
        benchmark::DoNotOptimize(result);
    }
}

// The naive port: only the result is protected, so the work can be hoisted
static void BM_AddNaive(benchmark::State& state) {
    const Vec3f v1(1.0f, 2.0f, 3.0f);
    const Vec3f v2(4.0f, 5.0f, 6.0f);
    for (auto _ : state) {
        auto result = v1 + v2;
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_AddNaive);
BENCHMARK_CAPTURE(BM_Op, add, [](const Vec3f& a, const Vec3f& b) { return a + b; });
BENCHMARK_CAPTURE(BM_Op, subtract, [](const Vec3f& a, const Vec3f& b) { return a - b; });
BENCHMARK_CAPTURE(BM_Op, scale, [](const Vec3f& a, const Vec3f&) { return a * 2.5f; });
BENCHMARK_CAPTURE(BM_Op, dot, [](const Vec3f& a, const Vec3f& b) { return a.dot(b); });
BENCHMARK_CAPTURE(BM_Op, cross, [](const Vec3f& a, const Vec3f& b) { return a.cross(b); });
BENCHMARK_CAPTURE(BM_Op, length, [](const Vec3f& a, const Vec3f&) { return a.length(); });
BENCHMARK_CAPTURE(BM_Op, normalized, [](const Vec3f& a, const Vec3f&) { return a.normalized(); });
