// Container benchmarks. Names are family/container[/pattern]/N, e.g.
//   lookup/sorted_vector/random/65536   build/unordered_map/1024   insert_sorted/list/4096
// Every benchmark reports items_per_second, so time per element/lookup = 1e9 / items_per_second.
#include <containers/workloads.hpp>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <list>
#include <string>
#include <vector>

namespace {

using namespace containers;

constexpr std::int64_t kMinN = 1 << 4;   // 16
constexpr std::int64_t kMaxN = 1 << 20;  // ~1M
constexpr std::size_t kLookupsPerIteration = 1024;

const char* name(Pattern p) { return p == Pattern::Random ? "random" : "sequential"; }

// Each iteration does a batch of lookups from a long key sequence, cycling through it.
// The sequence holds at least 64K keys, so large containers can't serve every lookup from a
// handful of cached paths
template <typename C>
void BM_Lookup(benchmark::State& state, Pattern pattern) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto entries = make_entries(n);
    C c;
    c.build(entries);
    const auto keys = make_lookups(entries, pattern, std::clamp<std::size_t>(n, 1 << 16, 1 << 20));

    std::size_t pos = 0;
    for (auto _ : state) {
        Value sum = 0;
        for (std::size_t j = 0; j < kLookupsPerIteration; ++j) {
            sum += *c.find(keys[pos]);
            pos = (pos + 1 == keys.size()) ? 0 : pos + 1;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kLookupsPerIteration));
}

// Inputs that get consumed every iteration (build, insert_sorted) cycle through a pool of
// distinct inputs, about 1M elements in total. Rebuilding from the *same* small input lets the
// branch predictor memorize its comparisons (see Day 3), which made sorted_vector's build look
// 4x cheaper at n <= 1024
constexpr std::size_t kPoolElements = 1 << 20;

std::size_t pool_size(std::size_t n) { return std::max<std::size_t>(1, kPoolElements / n); }

// Build from a batch of n entries, then destroy: both are part of a container's cost
template <typename C>
void BM_Build(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<std::vector<Entry>> pool;
    for (std::size_t s = 0; s < pool_size(n); ++s) pool.push_back(make_entries(n, s * n));

    std::size_t next = 0;
    for (auto _ : state) {
        C c;
        c.build(pool[next]);
        benchmark::DoNotOptimize(c);
        next = (next + 1 == pool.size()) ? 0 : next + 1;
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

template <typename C>
void BM_Iterate(benchmark::State& state) {
    const auto entries = make_entries(static_cast<std::size_t>(state.range(0)));
    C c;
    c.build(entries);
    for (auto _ : state) {
        benchmark::DoNotOptimize(c.sum_values());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

template <typename Sequence>
void BM_InsertSorted(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<std::vector<std::uint32_t>> pool;
    for (std::size_t s = 0; s < pool_size(n); ++s) {
        pool.push_back(make_values(n, static_cast<std::uint32_t>(3 + s)));
    }

    std::size_t next = 0;
    for (auto _ : state) {
        Sequence seq;
        insert_sorted(seq, pool[next]);
        benchmark::DoNotOptimize(seq);
        next = (next + 1 == pool.size()) ? 0 : next + 1;
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

benchmark::Benchmark* sizes(benchmark::Benchmark* b, std::int64_t max_n) {
    return b->RangeMultiplier(4)->Range(kMinN, max_n);
}

template <typename C>
void register_lookups(std::int64_t max_n) {
    for (const auto pattern : {Pattern::Random, Pattern::Sequential}) {
        const std::string label = std::string("lookup/") + C::name + "/" + name(pattern);
        sizes(benchmark::RegisterBenchmark(label, BM_Lookup<C>, pattern), max_n);
    }
}

template <typename C>
void register_build_and_iterate() {
    sizes(benchmark::RegisterBenchmark(std::string("build/") + C::name, BM_Build<C>), kMaxN);
    sizes(benchmark::RegisterBenchmark(std::string("iterate/") + C::name, BM_Iterate<C>), kMaxN);
}

void register_benchmarks() {
    register_lookups<StdMap>(kMaxN);
    register_lookups<HashMap>(kMaxN);
    register_lookups<SortedVector>(kMaxN);
    register_lookups<UnsortedVector>(1 << 16); // O(n) per lookup: stop before it takes minutes

    register_build_and_iterate<StdMap>();
    register_build_and_iterate<HashMap>();
    register_build_and_iterate<SortedVector>();
    register_build_and_iterate<UnsortedVector>();
    sizes(benchmark::RegisterBenchmark(std::string("build/") + HashMapReserved::name,
                                       BM_Build<HashMapReserved>),
          kMaxN);

    // O(n^2) for both, so keep n small
    benchmark::RegisterBenchmark("insert_sorted/vector", BM_InsertSorted<std::vector<std::uint32_t>>)
        ->RangeMultiplier(4)
        ->Range(1 << 6, 1 << 14);
    benchmark::RegisterBenchmark("insert_sorted/list", BM_InsertSorted<std::list<std::uint32_t>>)
        ->RangeMultiplier(4)
        ->Range(1 << 6, 1 << 14);
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
