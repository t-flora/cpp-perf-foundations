#include "check.hpp"

#include <stream/ring_buffer.hpp>
#include <stream/window.hpp>

#include <deque>
#include <random>

using stream::Entry;
using test::check;

namespace {

template <typename Container>
void check_window(const char* label, Container c) {
    const std::string l = label;
    constexpr std::int64_t W = 10; // window length

    // Hand-computed values
    stream::Window<Container> w(std::move(c));
    w.add({1, 100'0000, 10}, W);  // price 100.0000 x 10
    w.add({2, 102'0000, 30}, W);  // price 102.0000 x 30
    check(w.count() == 2 && w.volume() == 40, l + ": count and volume");
    // VWAP = (100*10 + 102*30) / 40 = 101.5
    check(w.vwap_ticks() == 101'5000, l + ": vwap");
    check(w.mean_price_ticks() == 101'0000, l + ": mean price");

    // The window is (now - W, now]: a trade exactly W old is evicted
    w.add({11, 104'0000, 10}, W);  // cutoff 1: evicts ts=1
    check(w.count() == 2 && w.volume() == 40, l + ": entry exactly one window old is evicted");
    w.add({12, 104'0000, 10}, W);  // cutoff 2: evicts ts=2
    check(w.count() == 2 && w.vwap_ticks() == 104'0000, l + ": second eviction");

    // Evicting at snapshot time, with no new trade
    w.evict_through(11);
    check(w.count() == 1, l + ": evict_through without a new trade");
    w.evict_through(100);
    check(w.count() == 0 && w.volume() == 0 && w.notional() == 0 && w.price_sum() == 0,
          l + ": empty window has all sums back at exactly zero");

    // Rounding is half up: (1 + 2) / 2 = 1.5 ticks -> 2
    stream::Window<Container> r;
    r.add({1, 1, 1}, W);
    r.add({2, 2, 1}, W);
    check(r.mean_price_ticks() == 2 && r.vwap_ticks() == 2, l + ": half-up rounding");
}

// Running sums stay *exactly* equal to a from-scratch recomputation, after a million
// add/evict cycles. That's the point of integer ticks: with doubles this drifts
void check_exact_sums() {
    stream::Window<stream::RingBuffer<Entry>> w(stream::RingBuffer<Entry>(64));
    std::mt19937_64 rng(42);
    std::int64_t ts = 0;
    for (int i = 0; i < 1'000'000; ++i) {
        ts += static_cast<std::int64_t>(rng() % 100);
        w.add({ts, static_cast<std::int64_t>(1 + rng() % 10'000'000), static_cast<std::int64_t>(1 + rng() % 1000)}, 5'000);
    }
    std::int64_t volume = 0, notional = 0, price_sum = 0;
    for (std::size_t i = 0; i < w.entries().size(); ++i) {
        const Entry& e = w.entries()[i];
        volume += e.qty;
        notional += e.price_ticks * e.qty;
        price_sum += e.price_ticks;
    }
    check(w.volume() == volume && w.notional() == notional && w.price_sum() == price_sum,
          "running sums equal a recomputation after 1M updates");
    check(w.entries().front().ts_ns > ts - 5'000, "only in-window entries remain");
}

} // namespace

int main() {
    check_window("deque", std::deque<Entry>{});
    check_window("ring", stream::RingBuffer<Entry>(4));
    check_exact_sums();
    return test::finish("window");
}
