#pragma once
// Day 5: parsing `timestamp_ns,symbol,price,quantity`. See the spec in README.md.
#include <cstdint>
#include <string>
#include <string_view>

namespace stream {

inline constexpr std::int64_t kTicksPerUnit = 10'000; // 1 tick = 0.0001

enum class ParseError { None, FieldCount, BadTimestamp, BadSymbol, BadPrice, BadQuantity };

// symbol is a view into the parsed line
struct Trade {
    std::int64_t ts_ns = 0;
    std::string_view symbol;
    std::int64_t price_ticks = 0;
    std::int64_t qty = 0;
};

// TODO: fast path. string_view slicing + std::from_chars; no allocation, no exceptions
inline ParseError parse_trade(std::string_view line, Trade& out) {
    (void)line;
    (void)out;
    return ParseError::FieldCount;
}

struct NaiveTrade {
    std::int64_t ts_ns = 0;
    std::string symbol;
    std::int64_t price_ticks = 0;
    std::int64_t qty = 0;
};

// TODO: naive path. std::string per field, std::stoll / std::stod.
// Must accept and reject exactly the same lines as parse_trade
inline ParseError parse_trade_naive(std::string line, NaiveTrade& out) {
    (void)line;
    (void)out;
    return ParseError::FieldCount;
}

// TODO: a price in ticks with exactly 4 decimals: 1872500 -> "187.2500". Exact, so no double
inline std::string ticks_to_string(std::int64_t ticks) {
    (void)ticks;
    return {};
}

} // namespace stream
