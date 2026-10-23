#pragma once
// Workloads for the scaling experiments.
//
//   count_primes_*: compute-bound. Trial division costs ~sqrt(n) per number, so equal-sized
//                   ranges are NOT equal work: later ranges are more expensive.
//   sum_*:          memory-bound. One add per 8 bytes loaded, so DRAM bandwidth is the limit.
#include "thread_pool.hpp"

#include <cstddef>
#include <cstdint>
#include <future>
#include <numeric>
#include <span>
#include <thread>
#include <vector>

namespace concurrency {

// Trial division by 2, 3, then 6k +/- 1
constexpr bool is_prime(std::uint64_t n) {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (std::uint64_t d = 5; d * d <= n; d += 6) {
        if (n % d == 0 || n % (d + 2) == 0) return false;
    }
    return true;
}

// Primes in [lo, hi)
inline std::uint64_t count_primes_serial(std::uint64_t lo, std::uint64_t hi) {
    std::uint64_t count = 0;
    for (std::uint64_t n = lo; n < hi; ++n) count += is_prime(n);
    return count;
}

// Static partitioning: one thread per equal-sized slice of the range. Simple, but the thread
// with the most expensive slice (the highest numbers) finishes last, and everyone waits for it
inline std::uint64_t count_primes_static(std::uint64_t lo, std::uint64_t hi, std::size_t threads) {
    std::vector<std::uint64_t> counts(threads);
    std::vector<std::thread> workers;
    const std::uint64_t len = hi - lo;
    for (std::size_t t = 0; t < threads; ++t) {
        const std::uint64_t a = lo + len * t / threads;
        const std::uint64_t b = lo + len * (t + 1) / threads;
        // Each thread writes only its own slot. Written once at the end, so no false
        // sharing problem; see the contention benchmarks for what happens otherwise
        workers.emplace_back([&counts, t, a, b] { counts[t] = count_primes_serial(a, b); });
    }
    for (auto& w : workers) w.join();
    return std::accumulate(counts.begin(), counts.end(), std::uint64_t{0});
}

// Dynamic partitioning: many small chunks through the pool. Idle workers take the next chunk,
// so uneven costs (and uneven cores) even out
inline std::uint64_t count_primes_dynamic(ThreadPool& pool, std::uint64_t lo, std::uint64_t hi, std::uint64_t grain) {
    std::vector<std::future<std::uint64_t>> parts;
    for (std::uint64_t a = lo; a < hi; a += grain) {
        const std::uint64_t b = std::min(hi, a + grain);
        parts.push_back(pool.submit(count_primes_serial, a, b));
    }
    std::uint64_t total = 0;
    for (auto& p : parts) total += p.get();
    return total;
}

inline std::uint64_t sum_serial(std::span<const std::uint64_t> data) {
    return std::accumulate(data.begin(), data.end(), std::uint64_t{0});
}

// Splits the array into `chunks` contiguous pieces summed on the pool
inline std::uint64_t sum_parallel(ThreadPool& pool, std::span<const std::uint64_t> data, std::size_t chunks) {
    std::vector<std::future<std::uint64_t>> parts;
    for (std::size_t c = 0; c < chunks; ++c) {
        const std::size_t a = data.size() * c / chunks;
        const std::size_t b = data.size() * (c + 1) / chunks;
        parts.push_back(pool.submit([data, a, b] { return sum_serial(data.subspan(a, b - a)); }));
    }
    std::uint64_t total = 0;
    for (auto& p : parts) total += p.get();
    return total;
}

} // namespace concurrency
