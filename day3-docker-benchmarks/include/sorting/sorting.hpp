#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace sorting {

namespace detail {

// Below this size, insertion sort beats partitioning: no recursion, and the data fits in cache
inline constexpr std::size_t kInsertionCutoff = 16;

inline void insertion_sort(std::span<std::uint32_t> a) {
    for (std::size_t i = 1; i < a.size(); ++i) {
        const std::uint32_t value = a[i];
        std::size_t j = i;
        while (j > 0 && a[j - 1] > value) {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = value;
    }
}

// Orders a[lo], a[mid], a[hi] and returns the median. Sorted and reversed inputs then get a
// perfect pivot, and the pivot value is never the unique maximum of the range, which Hoare
// partitioning needs to guarantee progress.
inline std::uint32_t median_of_three(std::span<std::uint32_t> a, std::size_t lo, std::size_t mid,
                                     std::size_t hi) {
    if (a[mid] < a[lo]) std::swap(a[mid], a[lo]);
    if (a[hi] < a[lo]) std::swap(a[hi], a[lo]);
    if (a[hi] < a[mid]) std::swap(a[hi], a[mid]);
    return a[mid];
}

// Hoare partition: returns j such that every element of [lo, j] <= every element of [j+1, hi].
// It stops on elements *equal* to the pivot and swaps them, so runs of duplicates split evenly
// instead of piling up on one side.
inline std::size_t hoare_partition(std::span<std::uint32_t> a, std::size_t lo, std::size_t hi) {
    const std::uint32_t pivot = median_of_three(a, lo, lo + (hi - lo) / 2, hi);
    std::size_t i = lo;
    std::size_t j = hi;
    while (true) {
        while (a[i] < pivot) ++i;
        while (a[j] > pivot) --j;
        if (i >= j) return j;
        std::swap(a[i], a[j]);
        ++i;
        --j;
    }
}

// Textbook Lomuto partition with the last element as pivot
inline std::size_t lomuto_partition(std::span<std::uint32_t> a, std::size_t lo, std::size_t hi) {
    const std::uint32_t pivot = a[hi];
    std::size_t store = lo;
    for (std::size_t k = lo; k < hi; ++k) {
        if (a[k] < pivot) std::swap(a[k], a[store++]);
    }
    std::swap(a[store], a[hi]);
    return store;
}

} // namespace detail

// Quicksort: median-of-three pivot, Hoare partition, insertion sort for small ranges.
// It recurses into the smaller half and loops on the larger, so stack depth stays O(log n)
// even on bad inputs.
inline void quicksort(std::span<std::uint32_t> a) {
    while (a.size() > detail::kInsertionCutoff) {
        const std::size_t split = detail::hoare_partition(a, 0, a.size() - 1) + 1;
        auto left = a.first(split);
        auto right = a.subspan(split);
        if (left.size() < right.size()) {
            quicksort(left);
            a = right;
        } else {
            quicksort(right);
            a = left;
        }
    }
    detail::insertion_sort(a);
}

// Textbook quicksort, kept as a cautionary baseline: last-element pivot + Lomuto partition is
// O(n^2) on sorted, reversed, and few-unique input, with O(n) recursion depth. Don't use it on
// large inputs.
inline void quicksort_naive(std::span<std::uint32_t> a) {
    if (a.size() < 2) return;
    const std::size_t p = detail::lomuto_partition(a, 0, a.size() - 1);
    quicksort_naive(a.first(p));
    quicksort_naive(a.subspan(p + 1));
}

// LSD radix sort on 8-bit digits: 4 stable counting-sort passes, O(n) time plus an O(n)
// scratch buffer. The buffer is allocated on every call, so its cost counts as part of the sort.
inline void radix_sort(std::span<std::uint32_t> a) {
    const std::size_t n = a.size();
    if (n < 2) return;

    // One pass over the input builds the histograms for all four digits
    std::array<std::array<std::size_t, 256>, 4> counts{};
    for (const std::uint32_t v : a) {
        for (int d = 0; d < 4; ++d) ++counts[d][(v >> (8 * d)) & 0xFF];
    }

    std::vector<std::uint32_t> scratch(n);
    std::uint32_t* src = a.data();
    std::uint32_t* dst = scratch.data();

    for (int d = 0; d < 4; ++d) {
        auto& count = counts[d];
        const int shift = 8 * d;

        // If every key has the same digit here, this pass would only copy: skip it.
        // That skips 3 of the 4 passes for small keys, e.g. values < 256
        if (count[(src[0] >> shift) & 0xFF] == n) continue;

        // Exclusive prefix sum: count[digit] becomes that digit's first output slot
        std::size_t offset = 0;
        for (auto& c : count) offset += std::exchange(c, offset);

        for (std::size_t i = 0; i < n; ++i) {
            const std::uint32_t v = src[i];
            dst[count[(v >> shift) & 0xFF]++] = v;
        }
        std::swap(src, dst);
    }

    // With an odd number of passes the sorted data ends in scratch
    if (src != a.data()) std::copy(src, src + n, a.data());
}

} // namespace sorting
