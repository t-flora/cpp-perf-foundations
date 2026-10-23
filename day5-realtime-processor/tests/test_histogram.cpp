#include "check.hpp"

#include <stream/histogram.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

using stream::LatencyHistogram;
using test::check;

int main() {
    // Small values are exact
    LatencyHistogram small;
    for (std::uint64_t v = 1; v <= 50; ++v) small.record(v);
    check(small.percentile(50) == 25 && small.percentile(100) == 50 && small.min() == 1, "exact below 64");

    // Every bucket's bounds are consistent with index()
    bool consistent = true;
    for (std::size_t i = 0; i + 1 < LatencyHistogram::kBuckets; ++i) {
        const auto ub = LatencyHistogram::upper_bound(i);
        consistent &= LatencyHistogram::index(ub) == i && LatencyHistogram::index(ub + 1) == i + 1;
    }
    check(consistent, "bucket upper bounds map back to their own bucket");

    // Percentiles of a heavy-tailed sample stay within ~3% of the exact answer
    std::mt19937_64 rng(3);
    std::lognormal_distribution<double> dist(5.0, 1.5);
    std::vector<std::uint64_t> values(200000);
    LatencyHistogram h;
    for (auto& v : values) {
        v = static_cast<std::uint64_t>(dist(rng)) + 1;
        h.record(v);
    }
    std::sort(values.begin(), values.end());
    for (double p : {50.0, 90.0, 99.0, 99.9}) {
        const auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * values.size())) - 1;
        const double exact = static_cast<double>(values[rank]);
        const double approx = static_cast<double>(h.percentile(p));
        check(approx >= exact && approx <= exact * 1.035, "p" + std::to_string(p) + " within 3.5%");
    }
    check(h.max() == values.back() && h.count() == values.size(), "max and count are exact");
    check(h.percentile(100) == values.back(), "p100 is the max");

    return test::finish("histogram");
}
