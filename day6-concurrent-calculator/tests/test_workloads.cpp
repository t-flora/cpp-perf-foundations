#include "check.hpp"

#include <concurrency/workloads.hpp>

#include <numeric>
#include <vector>

using namespace concurrency;
using test::check;

static_assert(is_prime(2) && is_prime(3) && is_prime(5) && is_prime(97) && is_prime(7919));
static_assert(!is_prime(0) && !is_prime(1) && !is_prime(4) && !is_prime(25) && !is_prime(7917));

int main() {
    constexpr std::uint64_t kN = 1'000'000;
    constexpr std::uint64_t kPrimesBelowN = 78'498; // pi(10^6)

    check(count_primes_serial(0, kN) == kPrimesBelowN, "serial prime count");
    for (std::size_t threads : {1u, 3u, 8u}) {
        check(count_primes_static(0, kN, threads) == kPrimesBelowN,
              "static partition, " + std::to_string(threads) + " threads");
    }
    ThreadPool pool(4);
    for (std::uint64_t grain : {1'000u, 77'777u, 2'000'000u}) {
        check(count_primes_dynamic(pool, 0, kN, grain) == kPrimesBelowN, "dynamic, grain " + std::to_string(grain));
    }

    std::vector<std::uint64_t> data(1'000'003);
    std::iota(data.begin(), data.end(), std::uint64_t{0});
    const std::uint64_t expected = data.size() * (data.size() - 1) / 2;
    check(sum_serial(data) == expected, "serial sum");
    for (std::size_t chunks : {1u, 7u, 64u}) {
        check(sum_parallel(pool, data, chunks) == expected, "parallel sum, " + std::to_string(chunks) + " chunks");
    }

    return test::finish("workloads");
}
