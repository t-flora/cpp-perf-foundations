// A benchmark comparison is only valid if every container computes the same answers.
#include <containers/workloads.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace containers;

namespace {

int failures = 0;

// Not assert(): this project builds in Release, where NDEBUG would compile asserts away
void check(bool ok, const std::string& what) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL " << what << "\n";
    }
}

template <typename C>
void check_map(std::size_t n) {
    const std::string label = std::string(C::name) + " n=" + std::to_string(n);
    const auto entries = make_entries(n);
    C c;
    c.build(entries);

    for (const auto& [k, v] : entries) {
        const Value* found = c.find(k);
        if (!found || *found != v) {
            check(false, label + ": wrong value for an existing key");
            return;
        }
    }
    for (std::size_t i = 0; i < 100; ++i) {
        check(c.find(missing_key(i)) == nullptr, label + ": found a missing key");
    }
    const Value expected_sum = n == 0 ? 0 : n * (n - 1) / 2; // values are 0..n-1
    check(c.sum_values() == expected_sum, label + ": sum_values");
}

void check_inputs() {
    auto entries = make_entries(100000);
    std::vector<Key> keys;
    for (const auto& e : entries) keys.push_back(e.first);
    std::sort(keys.begin(), keys.end());
    check(std::adjacent_find(keys.begin(), keys.end()) == keys.end(), "make_entries: unique keys");
    for (std::size_t i = 0; i < 1000; ++i) {
        check(!std::binary_search(keys.begin(), keys.end(), missing_key(i)),
              "missing_key collides with a real key");
    }

    // Entry sets with different offsets must not share keys
    for (const auto& e : make_entries(1000, 100000)) {
        check(!std::binary_search(keys.begin(), keys.end(), e.first), "offset entry sets overlap");
    }

    const auto seq = make_lookups(entries, Pattern::Sequential, 1000);
    check(std::is_sorted(seq.begin(), seq.end()), "sequential lookups are ascending");
    const auto rnd = make_lookups(entries, Pattern::Random, 1000);
    check(std::all_of(rnd.begin(), rnd.end(),
                      [&](Key k) { return std::binary_search(keys.begin(), keys.end(), k); }),
          "random lookups only use existing keys");
    check(!std::is_sorted(rnd.begin(), rnd.end()), "random lookups are not sorted");
}

void check_insert_sorted() {
    for (std::size_t n : {0u, 1u, 2u, 100u, 2000u}) {
        const auto values = make_values(n);
        auto expected = values;
        std::sort(expected.begin(), expected.end());

        std::vector<std::uint32_t> vec;
        insert_sorted(vec, values);
        std::list<std::uint32_t> lst;
        insert_sorted(lst, values);

        const std::string label = "insert_sorted n=" + std::to_string(n);
        check(vec == expected, label + " (vector)");
        check(std::equal(lst.begin(), lst.end(), expected.begin(), expected.end()),
              label + " (list)");
    }
}

} // namespace

int main() {
    check_inputs();
    for (std::size_t n : {0u, 1u, 2u, 17u, 1000u, 50000u}) {
        check_map<StdMap>(n);
        check_map<HashMap>(n);
        check_map<HashMapReserved>(n);
        check_map<SortedVector>(n);
        check_map<UnsortedVector>(n);
    }
    check_insert_sorted();

    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "All container workload tests passed\n";
    return EXIT_SUCCESS;
}
