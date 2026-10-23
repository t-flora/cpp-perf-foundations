# Day 6: Concurrent Task Processor
**Goal**: Write thread-safe building blocks, and measure what parallelism actually buys

You'll build a bounded blocking queue and a thread pool on top of it, prove them thread-safe (with tests and ThreadSanitizer), then use them to find out when more threads help and when they don't. Readings are listed under Day 6 in the [main README](../README.md#day-6-concurrent-task-processor).

## Deliverables

```
day6-concurrent-calculator/
├── CMakeLists.txt                       # Starter skeleton provided; add a TSan option
├── include/concurrency/
│   ├── bounded_queue.hpp                # Starter skeleton provided
│   ├── thread_pool.hpp                  # Starter skeleton provided
│   └── workloads.hpp
├── tests/
├── examples/race_demo.cpp               # A deliberate data race
├── benchmarks/bench_scaling.cpp
├── scripts/plot_scaling.py
└── results/ANALYSIS.md
```

## Milestones

### 1. Bounded Queue (~1.5h)
A multi-producer, multi-consumer queue with a fixed capacity, built on a `std::mutex` and `std::condition_variable`s:
- `push` blocks while full (backpressure), and `pop` blocks while empty.
- `close()`: blocked threads wake up, later pushes fail, and pops drain the remaining items and then signal "done" (`std::optional` works well), so consumers can loop on `while (auto item = q.pop())`.
- `try_push` / `try_pop` that never block.

How do you handle spurious wakeups? Do you notify before or after unlocking, and why?

### 2. Thread Pool (~1.5h)
- `submit(f, args...)` returns a `std::future` of `f`'s result. An exception thrown by `f` must reach the caller through `future::get()`. Hint: `std::packaged_task`, and why it doesn't fit in a `std::function`.
- The destructor finishes every queued task, then joins the workers.
- `parallel_for(pool, begin, end, grain, body)`: split, submit, wait. If a chunk throws, what must happen before you rethrow?

### 3. Prove It (~1.5h)
Tests, each targeting one guarantee:
- FIFO order; a producer really blocks on a full queue and resumes when there's room
- `close()` wakes a blocked producer *and* a blocked consumer; queued items still drain
- 4 producers × 4 consumers × 50K items: every item delivered exactly once
- Pool: results, forwarded arguments, move-only results, exceptions, draining on destruction
- Add a CMake option that builds everything with `-fsanitize=thread`, and run the tests under it. Then write `race_demo.cpp` (an unsynchronized counter incremented from 4 threads) and watch TSan catch it.

### 4. Scaling Experiments (~1.5h)
Google Benchmark with `UseRealTime()` (why not CPU time?), for 1 to `hardware_concurrency()` threads:
- **Compute-bound**: count primes below ~3M by trial division. Use *static* partitioning (T threads, T equal ranges) and *dynamic* partitioning (many small chunks through the pool).
- **Memory-bound**: sum a 256 MB array of `uint64_t` on the pool.
- **Contention**: T threads incrementing (a) one counter under a mutex, (b) one shared atomic, (c) per-thread atomics packed next to each other, (d) per-thread atomics padded apart. Use `->DenseThreadRange()` and `state.thread_index()`.

Plot speedup vs. threads (with the ideal line), and total counter throughput vs. threads.

### 5. Analysis (~1h)
In `results/ANALYSIS.md`, answer each question with numbers:
1. Trial division costs ~√n per number. Derive the maximum speedup of static partitioning with T equal ranges, and compare it with your measurements.
2. What happens once you have more threads than performance cores? (On hybrid CPUs, check how many of your cores are "efficiency" cores.)
3. How much does the memory-bound sum speed up? Compare the bandwidth you reach with your machine's theoretical peak.
4. Shared atomic vs. per-thread counters on the same cache line vs. padded: throughput at 2 threads and at max threads. What does this say about false sharing?
5. What is `std::hardware_destructive_interference_size` on your platform, and what is your actual cache line size? Measure the padding you need.
6. What does `race_demo` print without TSan in a Release build? Explain the exact number.

## Done When
- [ ] `ctest` passes, and passes again under ThreadSanitizer with zero reports
- [ ] TSan reports the race in `race_demo`
- [ ] `ANALYSIS.md` answers every question with numbers

## If Time Permits
- Sweep the `parallel_for` grain from 1 to 1M: where does queue overhead start to dominate?
- Move Day 5's snapshot output to a writer thread through your queue, and re-measure p99.99
- A work-stealing pool (a queue per worker): when does it beat one shared queue?
