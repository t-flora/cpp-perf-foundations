# Day 5: Real-Time Data Processor
**Goal**: Process a stream record by record, with bounded memory and predictable latency

You'll build a trade-stream processor that keeps per-symbol sliding-window statistics, twice: a naive version from the obvious standard-library parts, and a fast one built for latency. They must produce byte-identical output, so every difference you measure comes from implementation choices, not behavior. Readings are listed under Day 5 in the [main README](../README.md#day-5-real-time-data-processor).

## Provided
- `scripts/generate_trades.py`: a deterministic synthetic stream with Zipf-weighted symbols, random-walk prices, 20× bursts, and ~0.1% malformed or late lines. `--trades 1000000` takes ~3 s.
- `data/sample_trades.csv`: 2,048 lines (2,000 valid, 48 bad, covering every rejection reason) for your tests.

## Spec
**Input**, one trade per line: `timestamp_ns,symbol,price,quantity`

| Field | Valid | Otherwise |
|---|---|---|
| (line) | exactly 4 comma-separated fields | `field_count` |
| `timestamp_ns` | digits only, fits in `int64` | `bad_timestamp` |
| `symbol` | 1–8 characters from `[A-Z0-9.]` | `bad_symbol` |
| `price` | > 0, digits with optional `.` + 1–4 decimals, ≤ 6 integer digits | `bad_price` |
| `quantity` | digits only, 1 to 1,000,000 | `bad_quantity` |
| (order) | timestamp ≥ the last *accepted* timestamp | `late` |

**State**: per symbol, a sliding window of the trades with `ts` in `(now − W, now]`, with count, volume, VWAP, and mean price.

**Output**: a snapshot at every multiple B of the interval, in event time. The snapshot for B covers trades with `ts ≤ B`, so emit it when the first trade with `ts > B` arrives, plus a final snapshot at the last trade's timestamp. One row per symbol whose window isn't empty, sorted by symbol: `snapshot_ns,symbol,trades,volume,vwap,mean_price`. Prices are printed with exactly 4 decimals, rounded half up.

## Milestones

### 1. Parsing (~1.5h)
- **Fast**: slice the line into `string_view`s and parse with `std::from_chars`. No allocation, no exceptions.
- **Prices as integer ticks** (1 tick = 0.0001): parse the integer and fractional parts as integers. Write down why before you start; question 8 checks your answer.
- **Naive**: a `std::string` per field, `std::stoll`/`std::stod`. It must reject *exactly* the lines the fast parser rejects. Hint: `stod` happily accepts `" 12"`, `"1e3"`, and `"12abc"`.
- **Tests**: a table of edge cases run through both parsers, checking that the reasons and values agree.

### 2. Ring Buffer and Window (~1.5h)
- `RingBuffer<T>`: power-of-two capacity, mask instead of modulo, `push_back`/`pop_front`/`front`/`size`. Decide what happens when it's full, and make that event visible.
- `Window<Container>`: one template that works with both `std::deque` and your `RingBuffer`, with running sums so every statistic is O(1).
- **Test**: after a million random adds and evictions, the running sums equal a from-scratch recomputation *exactly*.

### 3. Two Pipelines (~2h)
| Piece | Naive | Fast |
|---|---|---|
| Reading | `std::getline` | large `fread` chunks + `memchr`, `string_view` lines |
| Symbol → state | `std::map<std::string, …>` | intern to an id (C++20 heterogeneous `unordered_map::find(string_view)`), state in a `vector` |
| Window | `std::deque` | `RingBuffer` |
| Output | `std::ostream << … << std::endl` | format with `to_chars` into a buffer, write in large chunks |

**Test**: on the sample with a 10 s window and 1 s interval, both pipelines produce byte-identical output and identical counters, with 2,000 trades accepted and every rejection reason non-zero.

### 4. Measure (~1.5h)
- A `processor` CLI: `--pipeline fast|naive`, `--window-ms`, `--interval-ms`, `--output FILE|-|none`, input file or stdin. It prints throughput and latency percentiles to stderr.
- **Per-record latency**: read `steady_clock` before and after processing each record, and record it in a histogram that doesn't allocate on the hot path. Start with a pre-reserved `vector` + `nth_element` if you like. A log-linear histogram (see HdrHistogram) is constant-size.
- **Measure the measurement**: the cost and resolution of two back-to-back clock reads.
- **Component benchmarks** (Google Benchmark): parse fast vs. naive, symbol lookup map vs. hash, window deque vs. ring.

### 5. Analysis (~1h)
In `results/ANALYSIS.md`, answer each question with numbers:
1. Throughput, p50, p99, p99.9, p99.99, and max for both pipelines, with output on and off, over 3 runs each.
2. Do the component costs add up to the per-record medians?
3. Which records make up the tail? What would remove them?
4. What's your clock's resolution and cost? What can't you measure because of it?
5. Does ring buffer growth show up in the percentiles? What's the trade-off in pre-sizing?
6. Is the max reproducible across identical runs? Why, or why not?
7. Pipe the generator straight into the processor. Does per-record latency change, even though I/O isn't timed?
8. Measure how far `double` running sums drift from a recomputation. Is drift the real reason to use integers?

## Done When
- [ ] `ctest` passes, including the byte-identical pipelines test
- [ ] The reference tests pass too (`-DDAY5_REFERENCE_TESTS=ON`; see the main README). Write your own first
- [ ] Both pipelines produce identical output on a 1M-trade stream (check with `cmp`)
- [ ] `ANALYSIS.md` answers every question with numbers

## If Time Permits
- Sliding-window high/low in O(1) amortized, with a monotonic queue
- Accept records up to an allowed lateness and correct affected windows, instead of rejecting them
- A single-pass parser: how close can you get to the cost of `memchr` itself?
