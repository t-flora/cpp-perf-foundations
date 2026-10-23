# Day 6 Analysis: What Parallelism Buys

**Setup**: Apple M2 (4 performance + 4 efficiency cores, 128-byte cache lines), macOS, Apple Clang 17, Release. Google Benchmark v1.9.5, real time, 3 repetitions, medians. The host was busy (load average ~4, a browser using ~2.5 cores), so the 5–8 thread results in particular understate an idle machine. Raw data: [`scaling.json`](scaling.json), plot: [`scaling.png`](scaling.png).

## 1. Static vs. Dynamic Partitioning (Compute-Bound)
Counting primes below 3M by trial division takes 81.4 ms on one thread.

| Threads | Static slices | Dynamic chunks (10K numbers each) | Static ceiling (model) |
|---|---|---|---|
| 1 | 1.02× | 0.98× | 1.00× |
| 2 | 1.62× | 1.80× | 1.55× |
| 3 | 2.25× | 2.79× | 2.19× |
| 4 | **2.92×** | **3.68×** | **2.85×** |
| 8 | 3.60× | 4.33× | 5.51× |

Trial division costs roughly √n per number, so equal *ranges* are unequal *work*. With T equal slices of [0, N], the last slice holds a fraction 1 − ((T−1)/T)^1.5 of the total work, and everyone waits for it. Speedup is capped at the reciprocal:

- T = 4: the last slice is 1 − 0.75^1.5 = 35% of the work, so the ceiling is **2.85×**. Measured: **2.92×**.

The model predicts static scaling within ~4% at every thread count up to 4. Dynamic chunks sidestep the problem: idle workers take the next chunk, so the expensive numbers get spread out, reaching 92% of ideal at 4 threads. The pool isn't free, though: with 1 thread, dynamic is 2.5% slower than plain serial (300 tasks through a mutex-protected queue plus a future each).

## 2. Past the Performance Cores
From 4 to 8 threads, dynamic only improves from 22.1 to 18.8 ms: four extra cores add 0.65 cores' worth of throughput, about 0.16 of a performance core each. Efficiency cores run at a lower clock with narrower pipelines, and the browser was using about 2.5 cores of exactly the capacity these threads would get, so this understates them. Static is worse: from 4 to 5 threads it gains *nothing* (27.9 → 28.0 ms). Slices get smaller, but now an expensive slice can land on a slow core, and the whole job waits for it. **Uneven cores are one more reason to partition dynamically.**

## 3. Memory-Bound Work Barely Scales
Summing 256 MB of `uint64_t`:

| Threads | 1 | 2 | 3 | 4 | 8 |
|---|---|---|---|---|---|
| Bandwidth | 52.4 GiB/s | 55.8 | **57.4** | 57.3 | 57.0 |
| Speedup | 1.00× | 1.06× | 1.09× | 1.09× | 1.09× |

One add per 8 bytes loaded makes this a pure DRAM bandwidth test. A single core already pulls 52 GiB/s, and the whole chip tops out at 57 GiB/s, about 60% of the M2's theoretical 100 GB/s (93 GiB/s). Adding threads to a bandwidth-bound loop just adds threads waiting on the same memory controller. Before parallelizing, ask what the bottleneck is: cores, or bytes.

## 4. Contention: Sharing Is the Expensive Part
Total increments per second across all threads. Every variant except the mutex does the same relaxed `fetch_add`; only where the counters sit changes.

| Threads | mutex | one shared atomic | per-thread, same line | per-thread, padded 64 B | per-thread, padded 128 B |
|---|---|---|---|---|---|
| 1 | 172 M/s | 501 M/s | 501 M/s | 500 M/s | 500 M/s |
| 2 | 96 M/s | 244 M/s | 236 M/s | 979 M/s | 974 M/s |
| 4 | 55 M/s | 129 M/s | 129 M/s | 1.83 G/s | 1.88 G/s |
| 8 | 53 M/s | 52 M/s | 152 M/s | 2.58 G/s | **3.13 G/s** |

- **A shared atomic gets *slower* with every thread you add**: 501 M/s with one thread, 52 M/s with eight. Every increment has to take exclusive ownership of the cache line, so the line bounces between cores and the increments are serialized by coherence traffic. At 8 threads, an atomic is no better than a mutex.
- **False sharing is as bad as true sharing.** Per-thread counters packed into one cache line share *no data*, yet at 2–4 threads they perform exactly like the single shared counter (236 vs. 244 M/s, then 129 vs. 129). The hardware tracks ownership per line, not per variable.
- **Padding fixes it**: per-thread counters on separate lines scale almost linearly, reaching 3.13 G/s at 8 threads, **60× the shared atomic**. The general rule is to aggregate per thread and combine at the end (what `count_primes_static` and the futures in `count_primes_dynamic` do), rather than all writing one location.
- The mutex costs 5.8 ns uncontended (vs. 2.0 ns for an atomic) and flattens at ~55 M/s once contended.
- **Two points don't fit the trend, and they reproduce**: the mutex at 3 threads (22–24 M/s, half its 4-thread throughput) and the shared atomic at 6 threads (35–37 M/s). Both repeated with CV under 5% within a run, and again in two separate reruns. **I haven't explained them.** Likely suspects are how macOS spreads 3 or 6 threads across the performance and efficiency clusters, and the mutex's spin-then-sleep policy. Confirming either would need per-thread core placement, which macOS doesn't expose. Left as an open question.

## 5. How Much Padding Does This Machine Need?
`std::hardware_destructive_interference_size` is the standard's answer: **64** in Apple's libc++. GCC says 256 for AArch64. `sysctl hw.cachelinesize` says **128**. Measured:
- **Up to 4 threads, 64 and 128 are identical** (1.83 vs. 1.88 G/s at 4). Among those threads, 64-byte padding was enough.
- **At 5–8 threads, 64-byte padding falls behind**: 2.58 vs. 3.13 G/s at 8, 18% lower. Once threads span both core clusters, two counters 64 bytes apart (one 128-byte line) contend again.

So the constant the standard library provides is too small for this machine, and the right value depends on which cores your threads run on. Pad to the measured line size (128 here), or to 2× the reported constant if you have to pick one value for every platform. It's a few bytes per counter.

## 6. ThreadSanitizer
- All three test suites run clean under `-DDAY6_TSAN=ON`: exit code 0 and zero reports, including the 4-producer / 4-consumer stress test and the pool shutdown test.
- `race_demo` is caught immediately: `WARNING: ThreadSanitizer: data race ... Location is global 'racy'`.
- Without TSan, in Release, the racy counter came out at **exactly 1,000,000** instead of 4,000,000, run after run. That's not a few lost updates. Because a data race is undefined behavior, the compiler may assume no other thread touches `racy`: it turned each thread's million increments into one `racy += 1000000`, and the four threads overwrote each other's single store. The atomic counter is exact. A race doesn't just produce "slightly wrong" numbers. It licenses the optimizer to produce *any* number.
