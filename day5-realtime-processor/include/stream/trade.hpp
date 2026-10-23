#pragma once
// Trade records: CSV parsing (a fast and a naive version that must agree exactly) and the
// decimal formatting of prices stored as integer ticks.
//
// Input format, one trade per line:  timestamp_ns,symbol,price,quantity
//   timestamp_ns  non-negative integer, nanoseconds since the epoch
//   symbol        1-8 characters from [A-Z0-9.]
//   price         decimal > 0, at most 6 integer digits and 4 decimals: "12", "12.5", "12.3456"
//   quantity      integer in [1, 1'000'000]
//
// The limits keep one trade's notional (price_ticks * qty) under 1e16, so int64 window sums
// have room for ~900 trades even at the extremes, and far more for realistic data.
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace stream {

inline constexpr std::int64_t kTicksPerUnit = 10'000; // 1 tick = 0.0001
inline constexpr std::size_t kMaxPriceIntDigits = 6;
inline constexpr std::int64_t kMaxQuantity = 1'000'000;
inline constexpr std::size_t kMaxPriceDecimals = 4;
inline constexpr std::size_t kMaxSymbolLength = 8;

enum class ParseError { None, FieldCount, BadTimestamp, BadSymbol, BadPrice, BadQuantity };

// Rejection reasons, including the one the pipeline adds (late = timestamp went backwards)
enum class Reject { FieldCount, BadTimestamp, BadSymbol, BadPrice, BadQuantity, Late, Count };

inline constexpr std::array<const char*, static_cast<std::size_t>(Reject::Count)> kRejectNames = {
    "field_count", "bad_timestamp", "bad_symbol", "bad_price", "bad_quantity", "late"};

inline Reject to_reject(ParseError e) { return static_cast<Reject>(static_cast<int>(e) - 1); }

// symbol is a view into the parsed line: copy it before the line goes away
struct Trade {
    std::int64_t ts_ns = 0;
    std::string_view symbol;
    std::int64_t price_ticks = 0;
    std::int64_t qty = 0;
};

namespace detail {

inline bool is_symbol_char(char c) { return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.'; }

inline bool valid_symbol(std::string_view s) {
    if (s.empty() || s.size() > kMaxSymbolLength) return false;
    for (char c : s) {
        if (!is_symbol_char(c)) return false;
    }
    return true;
}

inline bool all_digits(std::string_view s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

// from_chars over the whole field: no sign, no whitespace, no trailing junk
inline bool parse_int(std::string_view s, std::int64_t& out) {
    if (!all_digits(s)) return false;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc{} && ptr == s.data() + s.size();
}

// "123.45" -> 1234500 ticks, exactly: integer and fractional parts are parsed as integers,
// so no binary floating point is involved
inline bool parse_price_ticks(std::string_view s, std::int64_t& ticks) {
    const auto dot = s.find('.');
    const std::string_view int_part = s.substr(0, dot);
    const std::string_view frac_part = dot == std::string_view::npos ? std::string_view{} : s.substr(dot + 1);
    if (int_part.size() > kMaxPriceIntDigits) return false;
    if (dot != std::string_view::npos && (frac_part.empty() || frac_part.size() > kMaxPriceDecimals)) {
        return false;
    }
    std::int64_t whole = 0;
    std::int64_t frac = 0;
    if (!parse_int(int_part, whole)) return false;
    if (!frac_part.empty() && !parse_int(frac_part, frac)) return false;
    for (std::size_t i = frac_part.size(); i < kMaxPriceDecimals; ++i) frac *= 10;
    ticks = whole * kTicksPerUnit + frac;
    return true;
}

} // namespace detail

// Fast path: string_view slicing + std::from_chars. No allocation, no exceptions, no locale
inline ParseError parse_trade(std::string_view line, Trade& out) {
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);

    std::array<std::string_view, 4> fields;
    std::size_t start = 0;
    for (std::size_t i = 0; i < 3; ++i) {
        const auto comma = line.find(',', start);
        if (comma == std::string_view::npos) return ParseError::FieldCount;
        fields[i] = line.substr(start, comma - start);
        start = comma + 1;
    }
    fields[3] = line.substr(start);
    if (fields[3].find(',') != std::string_view::npos) return ParseError::FieldCount;

    if (!detail::parse_int(fields[0], out.ts_ns)) return ParseError::BadTimestamp;
    if (!detail::valid_symbol(fields[1])) return ParseError::BadSymbol;
    out.symbol = fields[1];
    if (!detail::parse_price_ticks(fields[2], out.price_ticks) || out.price_ticks <= 0) {
        return ParseError::BadPrice;
    }
    if (!detail::parse_int(fields[3], out.qty) || out.qty <= 0 || out.qty > kMaxQuantity) {
        return ParseError::BadQuantity;
    }
    return ParseError::None;
}

// The idiomatic-but-slow version a first attempt usually looks like: a std::string per field,
// std::stoll/std::stod with exceptions. stod/stoll accept leading whitespace, signs, exponents,
// and trailing junk, so to reject exactly what parse_trade rejects it needs the same
// character checks up front.
struct NaiveTrade {
    std::int64_t ts_ns = 0;
    std::string symbol;
    std::int64_t price_ticks = 0;
    std::int64_t qty = 0;
};

inline ParseError parse_trade_naive(std::string line, NaiveTrade& out) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::string fields[4];
    std::size_t n = 0;
    std::string current;
    for (char c : line) {
        if (c == ',') {
            if (n == 3) return ParseError::FieldCount;
            fields[n++] = current;
            current.clear();
        } else {
            current += c;
        }
    }
    if (n != 3) return ParseError::FieldCount;
    fields[3] = current;

    try {
        if (!detail::all_digits(fields[0])) return ParseError::BadTimestamp;
        out.ts_ns = std::stoll(fields[0]);
    } catch (const std::out_of_range&) {
        return ParseError::BadTimestamp;
    }

    if (!detail::valid_symbol(fields[1])) return ParseError::BadSymbol;
    out.symbol = fields[1];

    const std::string& p = fields[2];
    const auto dot = p.find('.');
    const std::string int_part = p.substr(0, dot);
    const std::string frac_part = dot == std::string::npos ? "" : p.substr(dot + 1);
    if (!detail::all_digits(int_part) || int_part.size() > kMaxPriceIntDigits ||
        (dot != std::string::npos && (!detail::all_digits(frac_part) || frac_part.size() > kMaxPriceDecimals))) {
        return ParseError::BadPrice;
    }
    out.price_ticks = std::llround(std::stod(p) * kTicksPerUnit);
    if (out.price_ticks <= 0) return ParseError::BadPrice;

    try {
        if (!detail::all_digits(fields[3])) return ParseError::BadQuantity;
        out.qty = std::stoll(fields[3]);
    } catch (const std::out_of_range&) {
        return ParseError::BadQuantity;
    }
    if (out.qty <= 0 || out.qty > kMaxQuantity) return ParseError::BadQuantity;
    return ParseError::None;
}

// ---- Formatting ------------------------------------------------------------------------------

// Writes ticks as "I.FFFF" (always 4 decimals) at `out`; returns one past the last char.
// `out` needs room for 25 chars
inline char* format_ticks(char* out, std::int64_t ticks) {
    out = std::to_chars(out, out + 20, ticks / kTicksPerUnit).ptr;
    *out++ = '.';
    std::int64_t frac = ticks % kTicksPerUnit;
    for (std::int64_t div = kTicksPerUnit / 10; div > 0; div /= 10) {
        *out++ = static_cast<char>('0' + frac / div);
        frac %= div;
    }
    return out;
}

inline std::string ticks_to_string(std::int64_t ticks) {
    char buf[32];
    return std::string(buf, format_ticks(buf, ticks));
}

} // namespace stream
