#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>
#include <vector>

namespace sorting {

enum class Distribution { Random, Sorted, Reversed, FewUnique };

inline constexpr Distribution kAllDistributions[] = {
    Distribution::Random, Distribution::Sorted, Distribution::Reversed, Distribution::FewUnique};

constexpr std::string_view name(Distribution d) {
    switch (d) {
        case Distribution::Random: return "random";
        case Distribution::Sorted: return "sorted";
        case Distribution::Reversed: return "reversed";
        case Distribution::FewUnique: return "few_unique";
    }
    return "unknown";
}

// Deterministic inputs: the same (distribution, n, seed) always gives the same data, so runs
// are comparable across machines and containers
inline std::vector<std::uint32_t> make_input(Distribution d, std::size_t n,
                                             std::uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::vector<std::uint32_t> v(n);
    if (d == Distribution::FewUnique) {
        std::uniform_int_distribution<std::uint32_t> few(0, 15); // values in [0, 16)
        for (auto& x : v) x = few(rng);
        return v;
    }
    for (auto& x : v) x = static_cast<std::uint32_t>(rng()); // full 32-bit range
    if (d == Distribution::Sorted) std::sort(v.begin(), v.end());
    if (d == Distribution::Reversed) std::sort(v.begin(), v.end(), std::greater<>{});
    return v;
}

} // namespace sorting
