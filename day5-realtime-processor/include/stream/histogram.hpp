#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace stream {

// Fixed-size log-linear latency histogram, a minimal HdrHistogram: values below 64 are exact,
// and above that each power of two is split into 32 buckets, so any recorded value is
// reported within ~3%. record() is a few instructions and never allocates, so it can sit on
// the hot path it measures.
class LatencyHistogram {
public:
    static constexpr int kSubBits = 5;                       // 32 sub-buckets per power of two
    static constexpr std::uint64_t kExactLimit = 64;         // 2^(kSubBits + 1)
    static constexpr std::size_t kBuckets = 64 + (63 - 6 + 1) * 32;

    void record(std::uint64_t v) {
        ++counts_[index(v)];
        ++count_;
        sum_ += v;
        max_ = std::max(max_, v);
        min_ = std::min(min_, v);
    }

    // Smallest bucket bound such that at least p% of values are <= it (p in [0, 100])
    std::uint64_t percentile(double p) const {
        if (count_ == 0) return 0;
        const auto target = std::max<std::uint64_t>(
            1, static_cast<std::uint64_t>(std::ceil(p / 100.0 * static_cast<double>(count_))));
        std::uint64_t seen = 0;
        for (std::size_t i = 0; i < kBuckets; ++i) {
            seen += counts_[i];
            if (seen >= target) return std::min(upper_bound(i), max_);
        }
        return max_;
    }

    std::uint64_t count() const { return count_; }
    std::uint64_t max() const { return max_; }
    std::uint64_t min() const { return count_ == 0 ? 0 : min_; }
    double mean() const { return count_ == 0 ? 0.0 : static_cast<double>(sum_) / static_cast<double>(count_); }

    static std::size_t index(std::uint64_t v) {
        if (v < kExactLimit) return static_cast<std::size_t>(v);
        const int e = std::bit_width(v) - 1;           // >= 6
        const int shift = e - kSubBits;                // >= 1
        const auto mantissa = v >> shift;              // in [32, 63]
        return static_cast<std::size_t>(64 + (e - 6) * 32 + static_cast<int>(mantissa - 32));
    }

    // Largest value that maps to bucket i
    static std::uint64_t upper_bound(std::size_t i) {
        if (i < kExactLimit) return i;
        const auto e = static_cast<int>((i - 64) / 32) + 6;
        const auto mantissa = static_cast<std::uint64_t>((i - 64) % 32 + 32);
        const int shift = e - kSubBits;
        return ((mantissa + 1) << shift) - 1;
    }

private:
    std::array<std::uint64_t, kBuckets> counts_{};
    std::uint64_t count_ = 0;
    std::uint64_t sum_ = 0;
    std::uint64_t max_ = 0;
    std::uint64_t min_ = UINT64_MAX;
};

} // namespace stream
