#pragma once
#include <cstdint>
#include <utility>

namespace stream {

struct Entry {
    std::int64_t ts_ns;
    std::int64_t price_ticks;
    std::int64_t qty;
};

// Time-based sliding window over one symbol's trades: it holds the trades with
// ts in (now - length, now] and keeps running sums, so every statistic is O(1).
//
// All sums are integers (prices in ticks), so subtracting an evicted trade restores the
// previous sum *exactly*: results are deterministic and can be compared with ==. With doubles,
// every add/subtract pair can leave a rounding residue. The drift is small but never zero,
// and it rules out exact comparisons and bit-identical replays.
//
// Container is std::deque<Entry> or RingBuffer<Entry>: anything with push_back,
// pop_front, front, empty, and size.
template <typename Container>
class Window {
public:
    explicit Window(Container c = Container{}) : entries_(std::move(c)) {}

    void add(const Entry& e, std::int64_t length_ns) {
        entries_.push_back(e);
        volume_ += e.qty;
        notional_ += e.price_ticks * e.qty;
        price_sum_ += e.price_ticks;
        evict_through(e.ts_ns - length_ns);
    }

    // Drop every entry with ts <= cutoff
    void evict_through(std::int64_t cutoff_ns) {
        while (!entries_.empty() && entries_.front().ts_ns <= cutoff_ns) {
            const Entry& old = entries_.front();
            volume_ -= old.qty;
            notional_ -= old.price_ticks * old.qty;
            price_sum_ -= old.price_ticks;
            entries_.pop_front();
        }
    }

    std::int64_t count() const { return static_cast<std::int64_t>(entries_.size()); }
    std::int64_t volume() const { return volume_; }
    std::int64_t notional() const { return notional_; }
    std::int64_t price_sum() const { return price_sum_; }

    // Volume-weighted average price, in ticks, rounded half up. Requires count() > 0
    std::int64_t vwap_ticks() const { return (notional_ + volume_ / 2) / volume_; }
    // Simple moving average of trade prices, in ticks, rounded half up. Requires count() > 0
    std::int64_t mean_price_ticks() const { return (price_sum_ + count() / 2) / count(); }

    const Container& entries() const { return entries_; }

private:
    Container entries_;
    std::int64_t volume_ = 0;
    std::int64_t notional_ = 0;
    std::int64_t price_sum_ = 0;
};

} // namespace stream
