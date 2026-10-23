// Scaling and contention benchmarks.
//
//   primes/serial, primes/static/T, primes/dynamic/T   compute-bound, uneven work per number
//   sum/serial, sum/parallel/T                          memory-bound (256 MB array)
//   counter/<kind>/real_time/threads:T                  T threads hammering counters
//
// Everything uses real (wall-clock) time: CPU time summed over threads would hide whether
// adding threads made anything finish sooner.
#include <concurrency/workloads.hpp>

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace concurrency;

constexpr std::uint64_t kPrimeLimit = 3'000'000;
constexpr std::uint64_t kPrimeGrain = 10'000;
constexpr std::size_t kSumElements = std::size_t{1} << 25; // 32M x 8 B = 256 MB, far beyond the caches

const int kMaxThreads = static_cast<int>(std::thread::hardware_concurrency());

const std::vector<std::uint64_t>& sum_data() {
    static const std::vector<std::uint64_t> data = [] {
        std::vector<std::uint64_t> v(kSumElements);
        std::iota(v.begin(), v.end(), std::uint64_t{0});
        return v;
    }();
    return data;
}

// ---- Scaling ---------------------------------------------------------------------------------

void BM_PrimesSerial(benchmark::State& state) {
    for (auto _ : state) benchmark::DoNotOptimize(count_primes_serial(0, kPrimeLimit));
    state.SetItemsProcessed(state.iterations() * kPrimeLimit);
}

void BM_PrimesStatic(benchmark::State& state) {
    const auto threads = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) benchmark::DoNotOptimize(count_primes_static(0, kPrimeLimit, threads));
    state.SetItemsProcessed(state.iterations() * kPrimeLimit);
}

void BM_PrimesDynamic(benchmark::State& state) {
    ThreadPool pool(static_cast<std::size_t>(state.range(0))); // created outside the timed loop
    for (auto _ : state) benchmark::DoNotOptimize(count_primes_dynamic(pool, 0, kPrimeLimit, kPrimeGrain));
    state.SetItemsProcessed(state.iterations() * kPrimeLimit);
}

void BM_SumSerial(benchmark::State& state) {
    const auto& data = sum_data();
    for (auto _ : state) benchmark::DoNotOptimize(sum_serial(data));
    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(data.size() * sizeof(std::uint64_t)));
}

void BM_SumParallel(benchmark::State& state) {
    const auto& data = sum_data();
    const auto threads = static_cast<std::size_t>(state.range(0));
    ThreadPool pool(threads);
    for (auto _ : state) benchmark::DoNotOptimize(sum_parallel(pool, data, threads * 4));
    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(data.size() * sizeof(std::uint64_t)));
}

// ---- Contention ------------------------------------------------------------------------------
// The same relaxed fetch_add each time; only *where* the counters live changes.

std::mutex g_mutex;
std::uint64_t g_locked_counter = 0;
std::atomic<std::uint64_t> g_shared{0};

// Per-thread counters, packed: 8 x 8 B = one 64 B line (half of an M2's 128 B line)
std::atomic<std::uint64_t> g_adjacent[64];

// Per-thread counters padded to 64 B, which is what libc++ reports as
// std::hardware_destructive_interference_size on Apple silicon...
struct alignas(64) Padded64 {
    std::atomic<std::uint64_t> v{0};
};
Padded64 g_padded64[64];

// ...and padded to 128 B, the M2's actual cache line size (sysctl hw.cachelinesize)
struct alignas(128) Padded128 {
    std::atomic<std::uint64_t> v{0};
};
Padded128 g_padded128[64];

void BM_CounterMutex(benchmark::State& state) {
    for (auto _ : state) {
        std::lock_guard lock(g_mutex);
        ++g_locked_counter;
    }
    state.SetItemsProcessed(state.iterations());
}

void BM_CounterSharedAtomic(benchmark::State& state) {
    for (auto _ : state) g_shared.fetch_add(1, std::memory_order_relaxed);
    state.SetItemsProcessed(state.iterations());
}

void BM_CounterAdjacent(benchmark::State& state) {
    auto& mine = g_adjacent[state.thread_index()];
    for (auto _ : state) mine.fetch_add(1, std::memory_order_relaxed);
    state.SetItemsProcessed(state.iterations());
}

void BM_CounterPadded64(benchmark::State& state) {
    auto& mine = g_padded64[state.thread_index()].v;
    for (auto _ : state) mine.fetch_add(1, std::memory_order_relaxed);
    state.SetItemsProcessed(state.iterations());
}

void BM_CounterPadded128(benchmark::State& state) {
    auto& mine = g_padded128[state.thread_index()].v;
    for (auto _ : state) mine.fetch_add(1, std::memory_order_relaxed);
    state.SetItemsProcessed(state.iterations());
}

void register_benchmarks() {
    using benchmark::RegisterBenchmark;
    RegisterBenchmark("primes/serial", BM_PrimesSerial)->UseRealTime()->Unit(benchmark::kMillisecond);
    RegisterBenchmark("primes/static", BM_PrimesStatic)->DenseRange(1, kMaxThreads)->UseRealTime()->Unit(benchmark::kMillisecond);
    RegisterBenchmark("primes/dynamic", BM_PrimesDynamic)->DenseRange(1, kMaxThreads)->UseRealTime()->Unit(benchmark::kMillisecond);
    RegisterBenchmark("sum/serial", BM_SumSerial)->UseRealTime()->Unit(benchmark::kMillisecond);
    RegisterBenchmark("sum/parallel", BM_SumParallel)->DenseRange(1, kMaxThreads)->UseRealTime()->Unit(benchmark::kMillisecond);

    struct Counter {
        const char* name;
        void (*fn)(benchmark::State&);
    };
    const Counter counters[] = {{"counter/mutex", BM_CounterMutex},
                                {"counter/shared_atomic", BM_CounterSharedAtomic},
                                {"counter/per_thread_adjacent", BM_CounterAdjacent},
                                {"counter/per_thread_padded64", BM_CounterPadded64},
                                {"counter/per_thread_padded128", BM_CounterPadded128}};
    for (const auto& c : counters) RegisterBenchmark(c.name, c.fn)->DenseThreadRange(1, kMaxThreads)->UseRealTime();
}

} // namespace

int main(int argc, char** argv) {
    register_benchmarks();
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
