// The fast and naive parsers must accept the same lines, produce the same values, and reject
// with the same reason.
#include "check.hpp"

#include <stream/trade.hpp>

#include <string>
#include <vector>

using stream::ParseError;
using test::check;

namespace {

struct Case {
    std::string line;
    ParseError expected;
};

const char* name(ParseError e) {
    switch (e) {
        case ParseError::None: return "ok";
        case ParseError::FieldCount: return "field_count";
        case ParseError::BadTimestamp: return "bad_timestamp";
        case ParseError::BadSymbol: return "bad_symbol";
        case ParseError::BadPrice: return "bad_price";
        case ParseError::BadQuantity: return "bad_quantity";
    }
    return "?";
}

} // namespace

int main() {
    // A valid line parses exactly, with prices in integer ticks
    stream::Trade t;
    check(stream::parse_trade("1700000000000000001,AAPL,187.25,300", t) == ParseError::None, "valid line");
    check(t.ts_ns == 1700000000000000001 && t.symbol == "AAPL" && t.price_ticks == 1872500 && t.qty == 300,
          "valid line fields");
    check(stream::parse_trade("1,BRK.B,0.0001,1\r", t) == ParseError::None && t.price_ticks == 1,
          "smallest price, dotted symbol, CRLF line ending");
    check(stream::parse_trade("1,X,999999.9999,1000000", t) == ParseError::None && t.price_ticks == 9999999999,
          "largest allowed price and quantity");

    const std::vector<Case> cases = {
        {"1,AAPL,100,1", ParseError::None},
        {"1,AAPL,100.5,1", ParseError::None},
        {"1,AAPL,100.1234,1", ParseError::None},
        {"1,AAPL,007.10,1", ParseError::None},
        {"", ParseError::FieldCount},
        {"1,AAPL,100", ParseError::FieldCount},
        {"1,AAPL,100,1,", ParseError::FieldCount},
        {"1,AAPL,100,1,extra", ParseError::FieldCount},
        {",AAPL,100,1", ParseError::BadTimestamp},
        {"-1,AAPL,100,1", ParseError::BadTimestamp},
        {" 1,AAPL,100,1", ParseError::BadTimestamp},
        {"1x,AAPL,100,1", ParseError::BadTimestamp},
        {"99999999999999999999,AAPL,100,1", ParseError::BadTimestamp}, // overflows int64
        {"1,,100,1", ParseError::BadSymbol},
        {"1,aapl,100,1", ParseError::BadSymbol},
        {"1,TOOLONGSYM,100,1", ParseError::BadSymbol},
        {"1,AA PL,100,1", ParseError::BadSymbol},
        {"1,AAPL,,1", ParseError::BadPrice},
        {"1,AAPL,0,1", ParseError::BadPrice},
        {"1,AAPL,0.0000,1", ParseError::BadPrice},
        {"1,AAPL,-5,1", ParseError::BadPrice},
        {"1,AAPL,+5,1", ParseError::BadPrice},
        {"1,AAPL,1e3,1", ParseError::BadPrice},  // std::stod would accept this
        {"1,AAPL,12.,1", ParseError::BadPrice},
        {"1,AAPL,.5,1", ParseError::BadPrice},
        {"1,AAPL,1.23456,1", ParseError::BadPrice},
        {"1,AAPL,1.2.3,1", ParseError::BadPrice},
        {"1,AAPL,1000000,1", ParseError::BadPrice}, // 7 integer digits
        {"1,AAPL,12abc,1", ParseError::BadPrice},   // std::stod would parse the "12"
        {"1,AAPL,100,", ParseError::BadQuantity},
        {"1,AAPL,100,0", ParseError::BadQuantity},
        {"1,AAPL,100,-3", ParseError::BadQuantity},
        {"1,AAPL,100,1.5", ParseError::BadQuantity},
        {"1,AAPL,100,1000001", ParseError::BadQuantity},
    };

    for (const auto& c : cases) {
        stream::Trade fast;
        stream::NaiveTrade naive;
        const ParseError f = stream::parse_trade(c.line, fast);
        const ParseError n = stream::parse_trade_naive(c.line, naive);
        check(f == c.expected, "fast: \"" + c.line + "\" -> " + name(f) + ", expected " + name(c.expected));
        check(n == c.expected, "naive: \"" + c.line + "\" -> " + name(n) + ", expected " + name(c.expected));
        if (f == ParseError::None && n == ParseError::None) {
            check(fast.ts_ns == naive.ts_ns && fast.symbol == naive.symbol &&
                      fast.price_ticks == naive.price_ticks && fast.qty == naive.qty,
                  "fast and naive values agree on \"" + c.line + "\"");
        }
    }

    // Exact decimal formatting of ticks
    check(stream::ticks_to_string(1872500) == "187.2500", "format 187.25");
    check(stream::ticks_to_string(1) == "0.0001", "format one tick");
    check(stream::ticks_to_string(9999999999) == "999999.9999", "format max price");

    return test::finish("parser");
}
