#pragma once
// The containers under test, behind one small interface, plus deterministic input generators.
// Tests and benchmarks share this header, so the code that gets timed is the code that gets
// checked.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <numeric>
#include <random>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

namespace containers {

using Key = std::uint64_t;
using Value = std::uint64_t;
using Entry = std::pair<Key, Value>;

// ---- Inputs -------------------------------------------------------------------------------

// splitmix64's finalizer is a bijection on 64-bit integers: distinct inputs give distinct,
// random-looking outputs, so mix(0..n-1) yields n unique keys without a dedup pass
constexpr std::uint64_t mix(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// n entries with unique random keys, in random (insertion) order; value i for the i-th entry.
// Different `first` values give disjoint key sets: entries for first=0 and first=n never collide
inline std::vector<Entry> make_entries(std::size_t n, std::uint64_t first = 0) {
    std::vector<Entry> entries(n);
    for (std::size_t i = 0; i < n; ++i) entries[i] = {mix(first + i), i};
    return entries;
}

// A key that make_entries never produces, as long as first + n stays below 2^63
constexpr Key missing_key(std::size_t i) { return mix((1ULL << 63) + i); }

enum class Pattern { Random, Sequential };

// `count` keys that all exist in `entries`. Random: uniformly drawn. Sequential: ascending
// key order, wrapping around, so successive lookups land next to each other
inline std::vector<Key> make_lookups(std::span<const Entry> entries, Pattern pattern,
                                     std::size_t count, std::uint64_t seed = 7) {
    std::vector<Key> keys;
    keys.reserve(entries.size());
    for (const auto& e : entries) keys.push_back(e.first);
    std::vector<Key> out(count);
    if (pattern == Pattern::Sequential) {
        std::sort(keys.begin(), keys.end());
        for (std::size_t i = 0; i < count; ++i) out[i] = keys[i % keys.size()];
    } else {
        std::mt19937_64 rng(seed);
        std::uniform_int_distribution<std::size_t> pick(0, keys.size() - 1);
        for (auto& k : out) k = keys[pick(rng)];
    }
    return out;
}

// ---- Map-like containers ------------------------------------------------------------------
// Each one is built from a batch of entries using that container's idiomatic method, then
// queried with find(), which returns nullptr for a missing key, and sum_values().

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

struct HashMap {
    static constexpr const char* name = "unordered_map";
    std::unordered_map<Key, Value> m;

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

// Same table, but reserve() first: no rehashing while it grows
struct HashMapReserved : HashMap {
    static constexpr const char* name = "unordered_map_reserved";

    void build(std::span<const Entry> entries) {
        m.reserve(entries.size());
        HashMap::build(entries);
    }
};

// A "flat map": sorted contiguous storage + binary search. Built in one batch (copy + sort),
// which is how you'd use it for read-mostly data
struct SortedVector {
    static constexpr const char* name = "sorted_vector";
    std::vector<Entry> v;

    void build(std::span<const Entry> entries) {
        v.assign(entries.begin(), entries.end());
        std::sort(v.begin(), v.end(), [](const Entry& a, const Entry& b) { return a.first < b.first; });
    }
    const Value* find(Key k) const {
        auto it = std::lower_bound(v.begin(), v.end(), k,
                                   [](const Entry& e, Key key) { return e.first < key; });
        return (it == v.end() || it->first != k) ? nullptr : &it->second;
    }
    Value sum_values() const {
        Value s = 0;
        for (const auto& [k, val] : v) s += val;
        return s;
    }
};

// No ordering at all: append, then linear search. O(n) lookups, but no pointers to chase
struct UnsortedVector {
    static constexpr const char* name = "vector";
    std::vector<Entry> v;

    void build(std::span<const Entry> entries) { v.assign(entries.begin(), entries.end()); }
    const Value* find(Key k) const {
        auto it = std::find_if(v.begin(), v.end(), [k](const Entry& e) { return e.first == k; });
        return it == v.end() ? nullptr : &it->second;
    }
    Value sum_values() const {
        Value s = 0;
        for (const auto& [k, val] : v) s += val;
        return s;
    }
};

// ---- Sequences: sorted insertion (the classic vector vs. list experiment) -------------------

// Insert each value at its sorted position, finding the position by linear search. The list
// insert itself is O(1), the vector's is O(n), and the search is O(n) for both.
template <typename Sequence>
void insert_sorted(Sequence& seq, std::span<const std::uint32_t> values) {
    for (const auto v : values) {
        auto it = std::find_if(seq.begin(), seq.end(), [v](std::uint32_t x) { return x >= v; });
        seq.insert(it, v);
    }
}

inline std::vector<std::uint32_t> make_values(std::size_t n, std::uint32_t seed = 3) {
    std::mt19937 rng(seed);
    std::vector<std::uint32_t> v(n);
    for (auto& x : v) x = static_cast<std::uint32_t>(rng());
    return v;
}

} // namespace containers
