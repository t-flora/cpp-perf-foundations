#!/usr/bin/env python3
"""Generate a deterministic synthetic trade stream as CSV: timestamp_ns,symbol,price,quantity

    python3 scripts/generate_trades.py --trades 1000000 > data/trades.csv
    python3 scripts/generate_trades.py --trades 1000000 | ./build/processor --output none

- Symbols are Zipf-weighted: a few are busy, most are quiet.
- Prices follow a per-symbol random walk, printed with 0-4 decimals ("101.5", "99.1234", "100").
- Arrivals are Poisson at --rate trades/s, with a 20x burst for 2 s out of every 60 s, which
  stresses window sizes and ring buffer capacity.
- About --bad-rate of the lines are malformed or late, one of each rejection reason in turn.
  Bad lines are inserted, so the output has --trades valid trades plus the bad lines.
"""
import argparse
import random
import sys

TICKERS = [
    "AAPL", "MSFT", "NVDA", "AMZN", "GOOG", "META", "TSLA", "JPM", "V", "XOM",
    "UNH", "MA", "HD", "PG", "COST", "AVGO", "LLY", "BRK.B", "WMT", "KO",
    "PEP", "MRK", "ABBV", "CVX", "BAC", "ORCL", "CRM", "ADBE", "NFLX", "AMD",
]
START_NS = 1_700_000_000_000_000_000
TICKS = 10_000  # 1 tick = 0.0001


def symbols(n):
    return [TICKERS[i] if i < len(TICKERS) else f"SYM{i}" for i in range(n)]


def format_price(ticks):
    """Ticks -> decimal string with trailing zeros dropped, so 0-4 decimals: 101.5, 99.1234, 100."""
    whole, frac = divmod(ticks, TICKS)
    return f"{whole}.{frac:04d}".rstrip("0").rstrip(".")


def bad_line(kind, ts, sym, price, qty):
    """One malformed or late line of the given kind."""
    return [
        f"{ts},{sym},{price}",                 # field_count: missing a field
        f"{ts},{sym},{price},{qty},extra",     # field_count: extra field
        f"{ts}x,{sym},{price},{qty}",          # bad_timestamp
        f"{ts},{sym.lower()},{price},{qty}",   # bad_symbol: lowercase
        f"{ts},,{price},{qty}",                # bad_symbol: empty
        f"{ts},{sym},{price}x,{qty}",          # bad_price: junk
        f"{ts},{sym},12.345678,{qty}",         # bad_price: too many decimals
        f"{ts},{sym},{price},-{qty}",          # bad_quantity: negative
        f"{ts},{sym},{price},0",               # bad_quantity: zero
        f"{ts - 5_000_000_000},{sym},{price},{qty}",  # late: 5 s in the past
    ][kind]


BAD_KINDS = 10


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--trades", type=int, default=1_000_000, help="valid trades to generate")
    parser.add_argument("--symbols", type=int, default=50)
    parser.add_argument("--rate", type=float, default=1000.0, help="mean trades per second outside bursts")
    parser.add_argument("--bad-rate", type=float, default=0.001, help="fraction of extra malformed/late lines")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out", help="output file (default: stdout)")
    args = parser.parse_args()

    rng = random.Random(args.seed)
    syms = symbols(args.symbols)
    weights = [1.0 / (rank + 1) for rank in range(len(syms))]  # Zipf
    prices = {s: rng.randint(10 * TICKS, 500 * TICKS) for s in syms}

    out = open(args.out, "w") if args.out else sys.stdout
    buf = []
    ts = START_NS
    bad_kind = 0
    for _ in range(args.trades):
        in_burst = (ts - START_NS) % 60_000_000_000 < 2_000_000_000
        rate = args.rate * (20 if in_burst else 1)
        ts += int(rng.expovariate(rate) * 1e9)

        sym = rng.choices(syms, weights)[0]
        prices[sym] = max(1, prices[sym] + rng.randint(-50, 50))
        price = format_price(prices[sym])
        qty = min(1_000_000, max(1, int(rng.lognormvariate(4.0, 1.2))))

        if rng.random() < args.bad_rate:
            buf.append(bad_line(bad_kind, ts, sym, price, qty))
            bad_kind = (bad_kind + 1) % BAD_KINDS
        buf.append(f"{ts},{sym},{price},{qty}")

        if len(buf) >= 10_000:
            out.write("\n".join(buf) + "\n")
            buf.clear()
    if buf:
        out.write("\n".join(buf) + "\n")
    if out is not sys.stdout:
        out.close()


if __name__ == "__main__":
    main()
