// Component benchmarks: each family isolates one of the swaps FastPipeline makes over
// NaivePipeline, so the end-to-end difference can be attributed.
#include <stream/pipeline.hpp>
#include <stream/ring_buffer.hpp>
#include <stream/trade.hpp>
#include <stream/window.hpp>

#include <benchmark/benchmark.h>

#include <chrono>
#include <deque>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

const std::vector<std::string> kSymbols = {"AAPL", "MSFT", "NVDA", "AMZN", "GOOG", "META", "TSLA", "JPM",
                                           "V",    "XOM",  "UNH",  "MA",   "HD",   "PG",   "COST", "BRK.B"};

// Realistic lines, cycled so the branch predictor can't memorize one (see Day 3)
std::vector<std::string> make_lines(std::size_t n) {
    std::mt19937_64 rng(5);
    std::vector<std::string> lines;
    std::int64_t ts = 1'700'000'000'000'000'000;
    for (std::size_t i = 0; i < n; ++i) {
        ts += static_cast<std::int64_t>(rng() % 2'000'000);
        const auto ticks = static_cast<std::int64_t>(100'000 + rng() % 5'000'000);
        std::string price = stream::ticks_to_string(ticks);
        while (price.back() == '0') price.pop_back(); // vary the number of decimals
        if (price.back() == '.') price.pop_back();
        lines.push_back(std::to_string(ts) + "," + kSymbols[rng() % kSymbols.size()] + "," + price + "," +
                        std::to_string(1 + rng() % 5000));
    }
    return lines;
}

void BM_ParseFast(benchmark::State& state) {
    const auto lines = make_lines(4096);
    std::size_t i = 0;
    for (auto _ : state) {
        stream::Trade t;
        benchmark::DoNotOptimize(stream::parse_trade(lines[i], t));
        benchmark::DoNotOptimize(t);
        i = (i + 1) & 4095;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ParseFast)->Name("parse/fast");

void BM_ParseNaive(benchmark::State& state) {
    const auto lines = make_lines(4096);
    std::size_t i = 0;
    for (auto _ : state) {
        stream::NaiveTrade t;
        benchmark::DoNotOptimize(stream::parse_trade_naive(lines[i], t));
        benchmark::DoNotOptimize(t);
        i = (i + 1) & 4095;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ParseNaive)->Name("parse/naive");

// Steady state: one add per iteration, which evicts one entry, keeping N entries in the window
template <typename Container>
void BM_Window(benchmark::State& state, Container c) {
    const auto n = state.range(0);
    constexpr std::int64_t step = 1000;
    stream::Window<Container> w(std::move(c));
    std::int64_t ts = 0;
    for (std::int64_t i = 0; i < n; ++i) w.add({ts += step, 1'000'000, 10}, n * step);
    for (auto _ : state) {
        w.add({ts += step, 1'000'000, 10}, n * step);
        benchmark::DoNotOptimize(w.volume());
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_CAPTURE(BM_Window, deque, std::deque<stream::Entry>{})->Name("window/deque")->RangeMultiplier(16)->Range(64, 16384);
BENCHMARK_CAPTURE(BM_Window, ring, stream::RingBuffer<stream::Entry>(32768))->Name("window/ring")->RangeMultiplier(16)->Range(64, 16384);

// Symbol -> state lookup. The naive pipeline builds a std::string per line and searches a
// std::map; the fast one hashes the string_view directly (heterogeneous lookup)
void BM_LookupMap(benchmark::State& state) {
    std::map<std::string, int> m;
    for (std::size_t i = 0; i < kSymbols.size(); ++i) m[kSymbols[i]] = static_cast<int>(i);
    const auto lines = make_lines(4096);
    std::vector<std::string_view> syms;
    for (const auto& l : lines) syms.push_back(std::string_view(l).substr(l.find(',') + 1, l.find(',', l.find(',') + 1) - l.find(',') - 1));
    std::size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(m.find(std::string(syms[i]))->second);
        i = (i + 1) & 4095;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LookupMap)->Name("symbol_lookup/map_string");

void BM_LookupHash(benchmark::State& state) {
    struct Hash {
        using is_transparent = void;
        std::size_t operator()(std::string_view s) const { return std::hash<std::string_view>{}(s); }
    };
    std::unordered_map<std::string, int, Hash, std::equal_to<>> m;
    for (std::size_t i = 0; i < kSymbols.size(); ++i) m[kSymbols[i]] = static_cast<int>(i);
    const auto lines = make_lines(4096);
    std::vector<std::string_view> syms;
    for (const auto& l : lines) syms.push_back(std::string_view(l).substr(l.find(',') + 1, l.find(',', l.find(',') + 1) - l.find(',') - 1));
    std::size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(m.find(syms[i])->second);
        i = (i + 1) & 4095;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LookupHash)->Name("symbol_lookup/unordered_string_view");

// What one latency measurement costs: processor reads the clock twice per record
void BM_SteadyClock(benchmark::State& state) {
    for (auto _ : state) benchmark::DoNotOptimize(std::chrono::steady_clock::now());
}
BENCHMARK(BM_SteadyClock)->Name("clock/steady_clock_now");

} // namespace
