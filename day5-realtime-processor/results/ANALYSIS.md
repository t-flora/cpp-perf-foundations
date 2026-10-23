# Day 5 Analysis: Naive vs. Fast Stream Processing

**Setup**: Apple M2, macOS, Apple Clang 17, Release. Input: `generate_trades.py --trades 1000000` (seed 1), 1,001,005 lines, 37 MB, 50 symbols, ~1000 s of event time with 20× bursts. 60 s window, 1 s snapshot interval. Each configuration ran 3 times; the tables show the median run. The host was busy (a browser using ~2.5 cores), which matters for the max column (question 6). Component benchmarks: [`components.json`](components.json).

## 1. Throughput and Latency

Per-record latency covers parsing, validation, window update, and any snapshot emission triggered by that record. Throughput includes I/O.

| Pipeline | Output | Throughput | p50 | p99 | p99.9 | p99.99 | max (3 runs) |
|---|---|---|---|---|---|---|---|
| naive | to file | 2.09M lines/s | 251 ns | 463 ns | 1.4 µs | **94 µs** | 2.6–7.3 ms |
| naive | none | 2.38M lines/s | 251 ns | 423 ns | 1.1 µs | 5.8 µs | 5.0–14 ms |
| fast | to file | 6.79M lines/s | 125 ns | 211 ns | 423 ns | 7.2 µs | 0.07–5.9 ms |
| fast | none | 6.96M lines/s | 125 ns | 211 ns | 375 ns | 2.0 µs | 0.07–2.4 ms |

With output, the fast pipeline has **3.2× the throughput, 2× lower median latency, 3.3× lower p99.9, and 13× lower p99.99**. Both produce byte-identical snapshots (`cmp` on the 30,051-row output).

## 2. Where Does the Median Difference Come From?
Component benchmarks (Google Benchmark, median of 5):

| Component | Naive | Fast | Saved |
|---|---|---|---|
| Parse one line | 213 ns | 86 ns | 127 ns |
| Symbol → state | 33 ns (`std::map<std::string>`, builds a key) | 12.7 ns (heterogeneous hash lookup) | 20 ns |
| Window update | 8.0 ns (`std::deque`) | 3.9 ns (`RingBuffer`) | 4 ns |
| One `steady_clock::now()` | 16.7 ns | 16.7 ns | |

The parts account for the whole. Fast: 86 + 13 + 4 ≈ 103 ns, plus ~17 ns of clock overhead ≈ 120 ns, against a measured mean of 116–122 ns. Naive: 213 + 33 + 8 ≈ 254 ns, plus clock and `getline` into a `std::string`, against a measured mean of 275–314 ns. **Parsing is most of the gap.** A `std::string` per field plus `stod`/`stoll` costs 2.5× what `from_chars` over `string_view`s does. The ring buffer is twice as fast as the deque, but the window was never the bottleneck.

There's still room: 86 ns to parse a 40-byte line is slow. The fast parser scans each field twice (validate, then `from_chars`) and finds commas one at a time. A single-pass parser is the obvious next step.

## 3. What Creates the Tail?
Snapshots. Every 1 s of event time (~1000 records), one record triggers a snapshot of ~30 active symbols, so about 0.1% of records do far more work. That's exactly where p99.9 and p99.99 sit.

- **Naive with output, p99.99 = 94 µs**: `std::endl` flushes the stream after every row, so a snapshot makes ~30 `write` system calls at ~3 µs each. With output disabled, the same records cost 5.8 µs. **`std::endl` alone is a 16× tail-latency regression.** Writing `'\n'` instead would let the stream buffer it.
- **Fast with output, p99.99 = 7.2 µs**: rows are formatted with `to_chars` into a 64 KB buffer, which is written out every ~1,200 rows (64 KB at ~53 bytes per row). The unlucky record that triggers the flush pays for the system call, and that's the gap between 7.2 µs and 2.0 µs.
- **What would remove it**: move output off the hot path entirely. Hand rows to another thread through a queue, so the processing thread never makes a system call. That's Day 6.

## 4. The Clock Is Part of the Measurement
`steady_clock` on Apple silicon ticks at **24 MHz: one tick is 41.7 ns**. The clock-overhead histogram shows only 0, 42, 83, and 125 ns. So:
- Every per-record latency is rounded to a multiple of ~41.7 ns, and "p50 = 125 ns" means "3 ticks". Differences under ~40 ns per record can't be seen this way. That's why the component costs come from Google Benchmark, which times millions of iterations at once.
- Each measurement costs two clock reads, ~33 ns, which is a quarter of the fast pipeline's per-record time. Timing every record makes the program measurably slower. Real systems sample, or read the CPU's cycle counter directly (`rdtsc`, or `cntvct_el0` on ARM).

## 5. Does Ring-Buffer Sizing Matter?
At the default capacity (4096 per symbol), the ring buffers grew 8 times in the whole run: the busiest symbols during 20× bursts. Pre-sizing to 32768 eliminates growth, but the percentiles don't move (p99.99 5.1 µs vs. 2.0 µs, within run-to-run noise). Eight events in a million are too rare to show up even at p99.99, so they hide in the max. The cost of pre-sizing is memory: 32768 × 24 bytes × 50 symbols = 39 MB reserved, touched page by page on first use. The right capacity comes from the expected peak rate × window length, with growth as a monitored fallback, not a design.

## 6. Why Is the Max Not Reproducible?
Across identical runs, the max ranges from 71 µs to 14 ms, in every configuration. Those are not slow records. The OS preempted the process (for the browser, an interrupt, or a timer), or a fresh page was touched for the first time, in the middle of a measurement. On a busy laptop nothing in user space prevents this.

On a dedicated Linux machine you would isolate a core for the process (`isolcpus`, `taskset`), lock and pre-fault memory (`mlockall`, touching every page at startup), raise scheduling priority (`SCHED_FIFO`), and keep the core from sleeping. Measure the max only after doing that; until then, trust p99.9 and p99.99 more than the max.

## 7. File vs. Live Pipe
Piping the generator straight in (`generate_trades.py | processor`) works unchanged: `LineReader` reads whatever `fread` returns. Throughput drops to 0.30M lines/s because the Python generator is the bottleneck. Per-record latency *also* gets worse, though I/O isn't in the timed region: p99.9 rises to 847 ns from 375–423 ns. Between chunks the processor waits for input, and it comes back to colder caches and branch predictors, and possibly a lower CPU frequency. That's why latency-critical systems busy-poll instead of sleeping.

## 8. Why Integer Ticks?
Measured: the same window with `double` prices and running sums drifts from an exact recomputation by 2.8e-7 after 1M updates and 1.6e-6 after 100M. That's a millionth of a tick, nowhere near visible at 4 decimals. So drift alone isn't a strong argument at this scale.

What integers buy is **exactness**. Running sums equal the recomputation bit for bit (`test_window` checks it after 1M updates). Two pipelines can be compared with `==` on their output. Replaying a stream gives identical results. And there's never a value sitting on a rounding boundary that prints differently depending on summation order. Decimal prices like 0.1 have no exact binary representation in the first place, so `stod` already introduces error before any arithmetic happens.
