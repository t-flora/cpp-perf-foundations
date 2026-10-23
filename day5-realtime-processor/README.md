# Day 5: Real-Time Trade Stream Processor
Reads a CSV trade stream (from a file or a live pipe), keeps per-symbol sliding-window statistics (trade count, volume, VWAP, mean price), and emits a snapshot row per active symbol at every event-time interval. It comes in two pipelines with byte-identical output: a *naive* one built from the obvious standard-library pieces, and a *fast* one built for predictable per-record latency. Findings, with numbers, are in [`results/ANALYSIS.md`](results/ANALYSIS.md).

## Project Structure

```
day5-realtime-processor/
├── include/stream/
│   ├── trade.hpp          # CSV parsing (fast + naive, must agree), integer-tick prices
│   ├── ring_buffer.hpp    # Power-of-two FIFO, allocates up front, counts growths
│   ├── window.hpp         # Time-based sliding window with exact integer running sums
│   ├── histogram.hpp      # Allocation-free log-linear latency histogram (~3% precision)
│   ├── line_reader.hpp    # Chunked fread + memchr line splitting, string_view lines
│   └── pipeline.hpp       # NaivePipeline and FastPipeline
├── src/processor.cpp      # CLI: run a pipeline, print throughput + latency percentiles
├── scripts/generate_trades.py
├── data/sample_trades.csv # 2,048 lines (48 bad), used by the tests
├── tests/                 # ring buffer, parser, window, histogram, pipelines agree
├── benchmarks/bench_components.cpp
└── results/               # ANALYSIS.md, components.json
```

## Running

```bash
cmake -B build && cmake --build build          # Release by default
(cd build && ctest --output-on-failure)

python3 scripts/generate_trades.py --trades 1000000 > data/trades.csv
./build/processor --pipeline fast  --output none data/trades.csv   # summary goes to stderr
./build/processor --pipeline naive --output none data/trades.csv

# As a live stream
python3 scripts/generate_trades.py --trades 1000000 | ./build/processor --output - > snapshots.csv
```

Options: `--window-ms` (60000), `--interval-ms` (1000), `--capacity` (per-symbol ring buffer, 4096), `--output FILE|-|none` (`-` is stdout, the default).

**Input** is `timestamp_ns,symbol,price,quantity` per line. Each line is accepted, or rejected with a reason: `field_count`, `bad_timestamp`, `bad_symbol` (1–8 chars from `[A-Z0-9.]`), `bad_price` (> 0, ≤ 6 integer digits, ≤ 4 decimals), `bad_quantity` (1 to 1,000,000), or `late` (timestamp earlier than the last accepted one). **Output** is `snapshot_ns,symbol,trades,volume,vwap,mean_price`.

## Design Notes
- **Prices are integer ticks** (1 tick = 0.0001). A sliding window adds every trade and later subtracts it. With integers the running sums stay *exactly* equal to a from-scratch recomputation (`test_window` checks this after 1M updates). That's what makes byte-identical output across the two pipelines, and across replays, possible. With `double`s the residue is tiny but never zero (see the analysis), and decimal prices like 0.1 have no exact binary representation anyway. The input limits keep `price × quantity` well inside `int64`.
- **Event time, not wall-clock time**: the snapshot for boundary B covers trades with `ts ≤ B` and is emitted when the first trade past B arrives. Replaying a file gives the same output at any speed.
- **The window is (now − W, now]**: an entry exactly W old is evicted. Snapshots evict too, so a symbol that stops trading drops out once its window empties.
- **Fast vs. naive, one swap at a time**:

  | Piece | Naive | Fast |
  |---|---|---|
  | Reading | `std::getline` into a `std::string` | 64 KB `fread` chunks, `memchr`, `string_view` lines |
  | Parsing | `std::string` per field, `std::stod`/`stoll` + exceptions | `from_chars`, exact decimal → ticks |
  | Symbol → state | `std::map<std::string, …>` (allocates a key per line) | interned ids via heterogeneous `unordered_map::find(string_view)`, state in a `vector` |
  | Window | `std::deque` | `RingBuffer` |
  | Output | `std::ostream << … << std::endl` (a flush per row) | `to_chars` into a 64 KB batch |

- **Naive must reject exactly what fast rejects**: `stod`/`stoll` accept whitespace, signs, exponents (`1e3`), and trailing junk (`12abc`), so the naive parser checks characters first. `test_parser` runs 30+ edge cases through both.
- **Latency is measured per record**: two `steady_clock::now()` calls around `process()`, recorded in the histogram. The summary also reports the cost of the measurement itself.
