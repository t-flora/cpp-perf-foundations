#pragma once
// Day 3: sorting algorithms to benchmark against std::sort. Fill in the TODOs.
#include <cstdint>
#include <span>

namespace sorting {

// TODO: quicksort. Your pivot choice will matter for sorted and few-unique input
inline void quicksort(std::span<std::uint32_t> a) {
    (void)a;
}

// TODO: LSD radix sort, 8-bit digits (4 passes), with a scratch buffer
inline void radix_sort(std::span<std::uint32_t> a) {
    (void)a;
}

} // namespace sorting
