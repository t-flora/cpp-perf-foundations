#pragma once
// Day 4: the containers under test behind one interface, plus input generators.
// Share this header between tests and benchmarks, so the code you time is the code you test.
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <utility>
#include <vector>

namespace containers {

using Key = std::uint64_t;
using Value = std::uint64_t;
using Entry = std::pair<Key, Value>;

// TODO: make_entries(n): n entries with *unique* random keys, deterministic across runs
// TODO: make_lookups(entries, pattern, count): existing keys in random or sorted order

// One container, done, to show the interface. Write the other four the same way:
// unordered_map, unordered_map_reserved, sorted_vector, vector
struct StdMap {
    static constexpr const char* name = "map";
    std::map<Key, Value> m;

    void build(std::span<const Entry> entries) {
        for (const auto& [k, v] : entries) m.emplace(k, v);
    }
    const Value* find(Key k) const {
        auto it = m.find(k);
        return it == m.end() ? nullptr : &it->second;
    }
    Value sum_values() const {
        Value s = 0;
        for (const auto& [k, v] : m) s += v;
        return s;
    }
};

// TODO: template <typename Sequence> void insert_sorted(Sequence&, std::span<const std::uint32_t>)
// Insert each value at its sorted position, found by linear search; works for vector and list

} // namespace containers
