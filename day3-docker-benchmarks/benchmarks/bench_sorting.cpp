// Sorting benchmarks: {std::sort, quicksort, quicksort_naive, radix_sort} x
// {random, sorted, reversed, few_unique} x sizes 2^4..2^20.
//
// Names are family/algorithm/distribution/N, e.g. sort/radix_sort/random/1048576,
// so --benchmark_filter=radix or --benchmark_filter=/sorted/ select slices.
#include <sorting/inputs.hpp>
#include <sorting/sorting.hpp>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace {

using sorting::Distribution;
using SortFn = void (*)(std::span<std::uint32_t>);

void std_sort(std::span<std::uint32_t> a) { std::sort(a.begin(), a.end()); }

struct Algorithm {
    const char* name;
    SortFn sort;
    std::int64_t max_n;
};

constexpr std::int64_t kMinN = 1 << 4;
constexpr std::int64_t kMaxN = 1 << 20;

const Algorithm kAlgorithms[] = {
    {"std_sort", std_sort, kMaxN},
    {"quicksort", sorting::quicksort, kMaxN},
    {"radix_sort", sorting::radix_sort, kMaxN},
    {"quicksort_naive", sorting::quicksort_naive, 1 << 14}, // O(n^2): keep it small
};

// Distinct inputs to cycle through, about kPoolElements elements in total. If every iteration
// sorts the *same* small array, the branch predictor memorizes its comparisons and
// comparison sorts look several times faster than they are (see sort_repeated/ below).
constexpr std::size_t kPoolElements = 1 << 20;

class InputPool {
public:
    InputPool(Distribution dist, std::size_t n, std::size_t total_elements) {
        const std::size_t count = std::max<std::size_t>(1, total_elements / n);
        for (std::size_t s = 0; s < count; ++s) {
            inputs_.push_back(sorting::make_input(dist, n, static_cast<std::uint32_t>(42 + s)));
        }
    }
    const std::vector<std::uint32_t>& next() {
        const auto& input = inputs_[next_];
        next_ = (next_ + 1 == inputs_.size()) ? 0 : next_ + 1;
        return input;
    }

private:
    std::vector<std::vector<std::uint32_t>> inputs_;
    std::size_t next_ = 0;
};

// Sorting in place destroys the input, so each iteration copies an unsorted input first.
// The copy is inside the timed region; BM_CopyBaseline measures it on its own.
void BM_Sort(benchmark::State& state, SortFn sort, Distribution dist, std::size_t pool_elements) {
    const auto n = static_cast<std::size_t>(state.range(0));
    InputPool pool(dist, n, pool_elements); // generated outside the timed loop, fixed seeds
    std::vector<std::uint32_t> data(n);
    for (auto _ : state) {
        const auto& input = pool.next();
        std::copy(input.begin(), input.end(), data.begin());
        sort(data);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory(); // the sorted data must really be written
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetComplexityN(state.range(0));
}

void BM_CopyBaseline(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    InputPool pool(Distribution::Random, n, kPoolElements);
    std::vector<std::uint32_t> data(n);
    for (auto _ : state) {
        const auto& input = pool.next();
        std::copy(input.begin(), input.end(), data.begin());
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// The alternative to a baseline: stop the clock around the copy. Pausing and resuming has a
// fixed cost of its own (clock reads plus bookkeeping), which swamps the copy it is meant to
// exclude, and at small n even the sort.
void BM_SortPaused(benchmark::State& state, SortFn sort) {
    const auto n = static_cast<std::size_t>(state.range(0));
    InputPool pool(Distribution::Random, n, kPoolElements);
    std::vector<std::uint32_t> data(n);
    for (auto _ : state) {
        state.PauseTiming();
        const auto& input = pool.next();
        std::copy(input.begin(), input.end(), data.begin());
        state.ResumeTiming();
        sort(data);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

void register_benchmarks() {
    for (const auto& algo : kAlgorithms) {
        for (const auto dist : sorting::kAllDistributions) {
            const std::string name =
                std::string("sort/") + algo.name + "/" + std::string(sorting::name(dist));
            benchmark::RegisterBenchmark(name, BM_Sort, algo.sort, dist, kPoolElements)
                ->RangeMultiplier(4)
                ->Range(kMinN, algo.max_n)
                ->Complexity(); // let Google Benchmark fit the big-O per family
        }
    }

    // The same random array every iteration: a demonstration of what *not* to do
    for (const auto& algo : {kAlgorithms[0], kAlgorithms[2]}) {
        benchmark::RegisterBenchmark(std::string("sort_repeated/") + algo.name + "/random",
                                     BM_Sort, algo.sort, Distribution::Random, std::size_t{0})
            ->RangeMultiplier(4)
            ->Range(1 << 8, 1 << 16);
    }

    benchmark::RegisterBenchmark("copy_baseline/random", BM_CopyBaseline)
        ->RangeMultiplier(4)
        ->Range(kMinN, kMaxN);

    for (const auto& algo : {kAlgorithms[0], kAlgorithms[2]}) {
        benchmark::RegisterBenchmark(std::string("sort_paused/") + algo.name + "/random",
                                     BM_SortPaused, algo.sort)
            ->Arg(1 << 6)
            ->Arg(1 << 10)
            ->Arg(1 << 14)
            ->Arg(1 << 18);
    }
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
