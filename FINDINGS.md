# Findings

What the measurements showed, day by day, and the lessons that kept coming back. Each day's analysis has the full tables and methodology; this is the summary. All numbers are from one Apple M2 laptop (4 performance + 4 efficiency cores, 128-byte cache lines), macOS, Apple Clang 17, Release, usually with a browser running. Treat them as this machine's numbers, not universal ones.

## Lessons That Recurred

1. **The compiler deletes work you don't protect.** Day 2's naive benchmark timed a hoisted result, not the operation (add looked 4× slower than it is). Day 6's data race compiled into one store per thread, so the "counter" ended at exactly 1,000,000 of 4,000,000. Use `DoNotOptimize`, and treat a data race as undefined behaviour, not a small error.
2. **The branch predictor memorizes repeated inputs.** Re-sorting the same 1024-element array was 4.7× faster than sorting fresh ones (Day 3), and Day 4's builds came out 2–4× too cheap the same way. Every benchmark that consumes its input since then cycles through ~1M elements of distinct inputs.
3. **Memory layout beats asymptotic complexity.** `vector` iteration is 217× faster than `map` at 1M (Day 4). Sorted insertion into a `vector` beats `list` by 16× despite the O(n) shift. A sorted vector beats `map` with the same O(log n) comparisons. Contention is a layout problem too: counters 8 bytes apart are as slow as one shared counter (Day 6).
4. **Attribute a difference before explaining it.** "Docker is 68% slower" (Day 3) blended three causes: the compiler (quicksort), the allocator returning memory to a VM (radix sort at 256K), and something unresolved (radix at 1M). Day 5's end-to-end speedup was split into component benchmarks that add up to the measured per-record cost. Day 7's loopback round trips were slow on both systems for unrelated reasons, found by comparing TCP against pipes and Unix sockets, blocking against spinning.
5. **Check the model, then find what's left over.** A √n cost model predicted static partitioning at 2.85× on 4 threads; measured 2.92× (Day 6). Fitted complexity matched theory except where a fixed per-call cost bent the curve (Day 3).
6. **Measurement has a resolution and a cost.** `steady_clock` ticks every 41.7 ns on Apple silicon, and two reads cost a quarter of a fast record's processing (Day 5). `PauseTiming` adds ~1 µs, more than the 10 ns it excludes at small N (Day 3).
7. **Tails come from rare, expensive events, and the max is the OS.** p99.99 is where flushes, snapshots, and buffer growth show up. The max varied from 71 µs to 14 ms across identical runs (Day 5).
8. **Benchmark on the platform you deploy on.** The Nagle/delayed-ACK stall costs 41 ms per request on Linux and doesn't happen on macOS (Day 7). The 64-byte padding the standard library suggests is too small for an M2 (Day 6). Docker on a Mac is a VM, with its own page-fault and wakeup costs (Days 3 and 7).
9. **Report what you can't explain.** Two reproducible dips in Day 6 (mutex at 3 threads, atomic at 6), radix sort at 1M under Docker (Day 3), and the source of macOS's TCP cost (Day 7) are open. Calling them noise was wrong once already.

## By Day

### Day 2: Header-Only Library ([README](day2-header-library/README.md#what-the-naive-benchmark-gets-wrong))
- The `volatile`-store benchmark reported add at 3.37 ns; with `DoNotOptimize` on the inputs it's 0.77 ns. The naive loop was timing three stores of a precomputed result.

### Day 3: Sorting, Native vs. Docker ([ANALYSIS](day3-docker-benchmarks/results/ANALYSIS.md))
- Radix sort overtakes `std::sort` between 16 and 64 elements and is 16× faster at 1M. A ~0.85 µs fixed cost per call dominates below that.
- A last-element-pivot quicksort is O(n²) on sorted input (fit: 0.28·N²); libc++'s `std::sort` is *linear* there because it detects sorted runs.
- Native median CV 1.4%, Docker 6.5%. Tiny inputs and core migration produce the worst outliers.
- Native vs. Docker: quicksort +22% is GCC vs. Clang (reproduces natively); radix sort at 256K is glibc returning memory to the kernel, fixed by malloc tunables; radix at 1M (5.9 → 21.5 ms) is unresolved.
- Runtime image 101 MB vs. 542 MB for the build stage.

### Day 4: Containers ([DECISION_GUIDE](day4-container-performance/results/DECISION_GUIDE.md))
- Point lookups at 1M: `unordered_map` 28 ns, `sorted_vector` 156 ns, `map` 266 ns.
- Linear search in a vector never beat a hash map, even at 16 entries (14.1 vs. 8.0 ns): the loop exit is an unpredictable branch.
- `reserve(n)` saves 35–40% of a hash map's build at every size.
- Sequential lookups speed up `map` 2.5× (shared search paths stay cached) and do nothing for a hash map.
- Curves bend where the working set leaves L1 (~128–256 KB) and again past L2 + SLC into DRAM (node containers, 256K → 1M).

### Day 5: Stream Processor ([ANALYSIS](day5-realtime-processor/results/ANALYSIS.md))
- Fast vs. naive: 6.79M vs. 2.09M lines/s, p50 125 vs. 251 ns, p99.99 7.2 vs. 94 µs. Parsing (86 vs. 213 ns per line) is most of the gap.
- `std::endl` alone is a 16× p99.99 regression: ~30 `write` calls per snapshot.
- A live pipe raises p99.9 from ~400 to 847 ns, though I/O isn't timed: waiting leaves caches and predictors cold.
- `double` running sums drift only 1.6e-6 after 1e8 updates. The case for integer ticks is exactness and reproducibility, not drift.

### Day 6: Concurrency ([ANALYSIS](day6-concurrent-calculator/results/ANALYSIS.md))
- Prime counting: static slices 2.92× at 4 threads (model: 2.85×), dynamic chunks 3.68×. From 4 to 8 threads dynamic only reaches 4.33×, and static gains nothing from 4 to 5: efficiency cores are slow, and a slice stuck on one holds up the job.
- Summing 256 MB: one core pulls 52.4 GiB/s, the chip peaks at 57.4 GiB/s. 1.09× with 8 threads.
- One shared atomic drops from 501 M/s (1 thread) to 52 M/s (8 threads), no better than a mutex. Per-thread counters padded apart reach 3.13 G/s, 60× more.
- `hardware_destructive_interference_size` is 64 in libc++, but 64-byte padding loses 18% to 128-byte padding once threads span both clusters.
- All tests clean under ThreadSanitizer; the race demo is caught.
- Open: mutex throughput halves at 3 threads, and the atomic dips at 6. Both reproduce.

### Day 7: TCP Echo ([ANALYSIS](day7-tcp-echo/results/ANALYSIS.md))
- A loopback round trip costs 75–84 µs on macOS but 4.6 µs over a Unix socket. On macOS, TCP `send()` itself takes 17.7 µs (0.54 µs for a Unix socket), and delivery afterwards is shared system-wide: ~30K round trips/s total, whatever the server design or pipelining depth. Suspect: the Cisco Secure Client socket filter installed on this machine; unconfirmed.
- In the Linux VM, TCP costs only ~2 µs over a Unix socket, but every *blocking* wakeup costs ~40 µs, even through a pipe. Latency *drops* under load (50 → 8 µs with 8 connections) because busy vCPUs never sleep.
- Nagle + header and payload in two writes: 41 ms per round trip on Linux (850×, delayed-ACK timer), no stall on macOS. Fix: `TCP_NODELAY`, or one write per message.
- Little's law held exactly for pipelining: 128 in flight / 371K round trips/s = 345 µs predicted, 339 µs measured. Past the throughput plateau, extra depth only adds queueing.
- Linux: a single-threaded `poll()` server plateaus at ~90K round trips/s (one core); thread per connection reaches 711K. Event loops buy connection count, not per-connection speed.
- Payload size is free up to 4 KB on both systems. A 1 MB echo is 6× faster on Linux (2.6 GiB/s vs. 441 MiB/s).
