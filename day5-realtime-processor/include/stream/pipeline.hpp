#pragma once
// Two trade-stream pipelines with identical behavior and output, built from different parts:
//
//   NaivePipeline: std::string per line and field, std::stod/stoll, std::map<std::string, ...>,
//                  a std::deque window, and std::ostream with std::endl (a flush per row)
//   FastPipeline:  std::string_view + std::from_chars, interned symbol ids indexing a vector,
//                  a RingBuffer window, and output batched into 64 KB writes
//
// Both: reject malformed and late records (counting each reason), keep a time-based sliding
// window per symbol, and emit a snapshot row per active symbol at every event-time boundary.
#include "ring_buffer.hpp"
#include "trade.hpp"
#include "window.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace stream {

inline constexpr const char* kHeader = "snapshot_ns,symbol,trades,volume,vwap,mean_price\n";

struct Config {
    std::int64_t window_ns = 60'000'000'000;   // 60 s of event time
    std::int64_t interval_ns = 1'000'000'000;  // a snapshot every 1 s of event time
    std::size_t ring_capacity = 4096;          // per-symbol RingBuffer capacity (FastPipeline)
};

struct Counters {
    std::uint64_t lines = 0;
    std::uint64_t accepted = 0;
    std::uint64_t snapshot_rows = 0;
    std::array<std::uint64_t, static_cast<std::size_t>(Reject::Count)> rejected{};

    std::uint64_t total_rejected() const { return std::accumulate(rejected.begin(), rejected.end(), std::uint64_t{0}); }
    bool operator==(const Counters&) const = default;
};

// Event-time snapshot schedule. Boundaries fall on multiples of the interval. The snapshot
// for boundary B covers trades with ts <= B, so it can only be emitted once a trade with
// ts > B arrives. The stream's own timestamps drive it, never the wall clock, so replaying
// a file gives the same output at any speed.
class SnapshotSchedule {
public:
    explicit SnapshotSchedule(std::int64_t interval_ns) : interval_(interval_ns) {}

    template <typename Emit>
    void before_trade(std::int64_t ts_ns, Emit&& emit) {
        if (!started_) {
            next_ = (ts_ns / interval_ + 1) * interval_;
            started_ = true;
            return;
        }
        while (ts_ns > next_) {
            emit(next_);
            next_ += interval_;
        }
    }

private:
    std::int64_t interval_;
    std::int64_t next_ = 0;
    bool started_ = false;
};

// ---- Naive ----------------------------------------------------------------------------------

class NaivePipeline {
public:
    // out == nullptr disables output, but rows are still computed and counted
    NaivePipeline(Config cfg, std::ostream* out) : cfg_(cfg), out_(out), schedule_(cfg.interval_ns) {
        if (out_) *out_ << kHeader;
    }

    void process(const std::string& line) {
        ++counters_.lines;
        NaiveTrade t;
        if (const ParseError e = parse_trade_naive(line, t); e != ParseError::None) {
            ++counters_.rejected[static_cast<std::size_t>(to_reject(e))];
            return;
        }
        if (t.ts_ns < last_ts_) {
            ++counters_.rejected[static_cast<std::size_t>(Reject::Late)];
            return;
        }
        schedule_.before_trade(t.ts_ns, [this](std::int64_t b) { snapshot(b); });
        last_ts_ = t.ts_ns;
        windows_[t.symbol].add({t.ts_ns, t.price_ticks, t.qty}, cfg_.window_ns);
        ++counters_.accepted;
    }

    // Final snapshot at the last trade's timestamp
    void finish() {
        if (counters_.accepted > 0) snapshot(last_ts_);
    }

    const Counters& counters() const { return counters_; }

private:
    void snapshot(std::int64_t boundary) {
        for (auto& [symbol, w] : windows_) {
            w.evict_through(boundary - cfg_.window_ns);
            if (w.count() == 0) continue;
            ++counters_.snapshot_rows;
            if (out_) {
                *out_ << boundary << ',' << symbol << ',' << w.count() << ',' << w.volume() << ','
                      << ticks_to_string(w.vwap_ticks()) << ',' << ticks_to_string(w.mean_price_ticks())
                      << std::endl; // flushes every row: one write syscall each
            }
        }
    }

    Config cfg_;
    std::ostream* out_;
    SnapshotSchedule schedule_;
    std::int64_t last_ts_ = std::numeric_limits<std::int64_t>::min();
    std::map<std::string, Window<std::deque<Entry>>> windows_;
    Counters counters_;
};

// ---- Fast -----------------------------------------------------------------------------------

// Collects output and writes it in large chunks: one write per 64 KB instead of one per row.
// Without a file it keeps everything in memory (used by tests).
class OutputBuffer {
public:
    explicit OutputBuffer(std::FILE* file = nullptr, std::size_t flush_at = 64 * 1024)
        : file_(file), flush_at_(flush_at) {
        buf_.reserve(2 * flush_at_);
    }
    OutputBuffer(const OutputBuffer&) = delete;
    OutputBuffer& operator=(const OutputBuffer&) = delete;
    ~OutputBuffer() { flush(); }

    void append(std::string_view s) {
        buf_.append(s);
        if (file_ && buf_.size() >= flush_at_) flush();
    }
    void flush() {
        if (!file_ || buf_.empty()) return;
        std::fwrite(buf_.data(), 1, buf_.size(), file_);
        std::fflush(file_);
        buf_.clear();
    }
    const std::string& str() const { return buf_; }

private:
    std::FILE* file_;
    std::size_t flush_at_;
    std::string buf_;
};

class FastPipeline {
public:
    // out == nullptr disables output, but rows are still computed and counted
    FastPipeline(Config cfg, OutputBuffer* out) : cfg_(cfg), out_(out), schedule_(cfg.interval_ns) {
        if (out_) out_->append(kHeader);
    }

    void process(std::string_view line) {
        ++counters_.lines;
        Trade t;
        if (const ParseError e = parse_trade(line, t); e != ParseError::None) {
            ++counters_.rejected[static_cast<std::size_t>(to_reject(e))];
            return;
        }
        if (t.ts_ns < last_ts_) {
            ++counters_.rejected[static_cast<std::size_t>(Reject::Late)];
            return;
        }
        schedule_.before_trade(t.ts_ns, [this](std::int64_t b) { snapshot(b); });
        last_ts_ = t.ts_ns;
        windows_[intern(t.symbol)].add({t.ts_ns, t.price_ticks, t.qty}, cfg_.window_ns);
        ++counters_.accepted;
    }

    void finish() {
        if (counters_.accepted > 0) snapshot(last_ts_);
    }

    const Counters& counters() const { return counters_; }

    // Times any symbol's ring buffer had to grow: each one is an allocation + copy on the
    // hot path. Zero means ring_capacity was big enough
    std::size_t ring_growths() const {
        std::size_t n = 0;
        for (const auto& w : windows_) n += w.entries().growths();
        return n;
    }

private:
    // Heterogeneous lookup (C++20): find() takes the string_view directly, so looking up a
    // known symbol doesn't construct a std::string
    struct StringHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view s) const { return std::hash<std::string_view>{}(s); }
    };

    std::uint32_t intern(std::string_view symbol) {
        if (auto it = ids_.find(symbol); it != ids_.end()) return it->second;
        const auto id = static_cast<std::uint32_t>(names_.size());
        ids_.emplace(std::string(symbol), id);
        names_.emplace_back(symbol);
        windows_.emplace_back(RingBuffer<Entry>(cfg_.ring_capacity));
        // Keep ids sorted by name so snapshots come out in the same order as std::map's
        const auto pos = std::lower_bound(order_.begin(), order_.end(), id,
                                          [this](std::uint32_t a, std::uint32_t b) { return names_[a] < names_[b]; });
        order_.insert(pos, id);
        return id;
    }

    void snapshot(std::int64_t boundary) {
        for (const std::uint32_t id : order_) {
            auto& w = windows_[id];
            w.evict_through(boundary - cfg_.window_ns);
            if (w.count() == 0) continue;
            ++counters_.snapshot_rows;
            if (!out_) continue;

            char row[128];
            char* p = row;
            char* const end = row + sizeof(row);
            p = std::to_chars(p, end, boundary).ptr;
            *p++ = ',';
            std::memcpy(p, names_[id].data(), names_[id].size());
            p += names_[id].size();
            *p++ = ',';
            p = std::to_chars(p, end, w.count()).ptr;
            *p++ = ',';
            p = std::to_chars(p, end, w.volume()).ptr;
            *p++ = ',';
            p = format_ticks(p, w.vwap_ticks());
            *p++ = ',';
            p = format_ticks(p, w.mean_price_ticks());
            *p++ = '\n';
            out_->append({row, static_cast<std::size_t>(p - row)});
        }
    }

    Config cfg_;
    OutputBuffer* out_;
    SnapshotSchedule schedule_;
    std::int64_t last_ts_ = std::numeric_limits<std::int64_t>::min();
    std::unordered_map<std::string, std::uint32_t, StringHash, std::equal_to<>> ids_;
    std::vector<std::string> names_;
    std::vector<Window<RingBuffer<Entry>>> windows_;
    std::vector<std::uint32_t> order_; // symbol ids sorted by name
    Counters counters_;
};

} // namespace stream
