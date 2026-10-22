// Correctness before speed: every algorithm must agree with std::sort on every input shape.
#include <sorting/inputs.hpp>
#include <sorting/sorting.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using Input = std::vector<std::uint32_t>;
using SortFn = void (*)(std::span<std::uint32_t>);

struct Algorithm {
    const char* name;
    SortFn sort;
    std::size_t max_n; // quicksort_naive is O(n^2) on some inputs
};

int failures = 0;

// Not assert(): this project builds in Release, where NDEBUG would compile asserts away
void check_case(const Algorithm& algo, const std::string& label, const Input& input) {
    if (input.size() > algo.max_n) return;
    Input expected = input;
    std::sort(expected.begin(), expected.end());
    Input actual = input;
    algo.sort(actual);
    if (actual != expected) {
        ++failures;
        std::cerr << "FAIL " << algo.name << " on " << label << " (n=" << input.size() << ")\n";
    }
}

std::vector<std::pair<std::string, Input>> test_cases() {
    constexpr auto kMax = std::numeric_limits<std::uint32_t>::max();
    std::vector<std::pair<std::string, Input>> cases = {
        {"empty", {}},
        {"single", {7}},
        {"two sorted", {1, 2}},
        {"two reversed", {2, 1}},
        {"all equal", Input(1000, 5)},
        {"extremes", {kMax, 0, kMax, 1, 0, kMax - 1}},
        {"one digit differs", {0x01020304, 0x01020303, 0x01020305}},
        {"organ pipe", {1, 3, 5, 7, 9, 8, 6, 4, 2, 0, 1, 3, 5, 7, 9, 8, 6, 4, 2, 0}},
    };
    // Sizes around the insertion-sort cutoff and a few larger ones
    for (std::size_t n : {3u, 15u, 16u, 17u, 33u, 100u, 1000u, 4096u, 100000u}) {
        for (auto d : sorting::kAllDistributions) {
            for (std::uint32_t seed : {1u, 2u, 3u}) {
                cases.emplace_back(std::string(sorting::name(d)) + " seed " + std::to_string(seed),
                                   sorting::make_input(d, n, seed));
            }
        }
    }
    return cases;
}

} // namespace

int main() {
    const Algorithm algorithms[] = {
        {"quicksort", sorting::quicksort, std::numeric_limits<std::size_t>::max()},
        {"quicksort_naive", sorting::quicksort_naive, 4096},
        {"radix_sort", sorting::radix_sort, std::numeric_limits<std::size_t>::max()},
    };

    // The generators must actually produce the shapes they claim
    const auto sorted = sorting::make_input(sorting::Distribution::Sorted, 1000);
    const auto reversed = sorting::make_input(sorting::Distribution::Reversed, 1000);
    const auto few = sorting::make_input(sorting::Distribution::FewUnique, 1000);
    if (!std::is_sorted(sorted.begin(), sorted.end()) ||
        !std::is_sorted(reversed.rbegin(), reversed.rend()) ||
        *std::max_element(few.begin(), few.end()) >= 16 ||
        sorting::make_input(sorting::Distribution::Random, 100) !=
            sorting::make_input(sorting::Distribution::Random, 100)) {
        std::cerr << "FAIL input generators\n";
        ++failures;
    }

    const auto cases = test_cases();
    for (const auto& algo : algorithms) {
        for (const auto& [label, input] : cases) check_case(algo, label, input);
    }

    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "All sorting tests passed (" << cases.size() << " inputs x "
              << std::size(algorithms) << " algorithms)\n";
    return EXIT_SUCCESS;
}
