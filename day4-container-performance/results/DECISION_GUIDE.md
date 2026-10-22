# Day 4: Container Decision Guide

**Setup**: Apple M2, macOS, Apple Clang 17 + libc++, Release. `std::uint64_t` keys and values (16-byte entries). Google Benchmark v1.9.5, 3 repetitions × 0.1 s, medians. Median CV 1.7%; the host was busy (load average ~5 on 8 cores). Raw data: [`results.json`](results.json), [`results.csv`](results.csv), plot: [`containers.png`](containers.png).

All numbers are **ns per item**: per lookup, per element built, per element iterated, or per element inserted.

## The Guide

| If your workload is… | Use | Because (measured) |
|---|---|---|
| Point lookups by key, at any size | `unordered_map` | 8–14 ns per random lookup up to 256K entries and 28 ns at 1M, against 266 ns for `map` and 156 ns for `sorted_vector` at 1M |
| Building a hash map whose size you know | `unordered_map` + `reserve(n)` | 40% cheaper build at 1M (95 vs. 159 ns/element), and 38% at 16 |
| Build once, then read: lookups plus ordered iteration | `sorted_vector` | At 1M: lookups 1.7× faster than `map` (156 vs. 266), iteration 217× faster (0.33 vs. 71.6), build 6× cheaper (90 vs. 534) |
| Ordered data with inserts and erases interleaved with reads | `map` | The only option here with O(log n) ordered insertion. You pay for it: 266 ns lookups and 72 ns/element iteration at 1M |
| Lookups that arrive in key order (merges, range scans) | `sorted_vector` or `map` | Sorted access cuts `map` lookups 2.5× at 1M (266 → 105). A hash map gains nothing (28 → 29) |
| Full scans and aggregation | a `vector` (sorted or not) | 0.33 ns/element at every size: 93× faster than `unordered_map` and 217× faster than `map` at 1M |
| Tiny collections (≤ 64 entries) | `unordered_map`, or anything contiguous | Linear search only *ties* `map` at 16 entries (14.1 vs. 14.6 ns) and loses to `unordered_map` (8.0 ns). "Vectors always win when small" didn't hold here |
| A sequence kept sorted with inserts in the middle | `vector`, never `list` | 16× faster at 16K elements (1.75 vs. 28.8 µs per insert), even though each insert shifts O(n) elements |

## Questions

### 1. When does linear search in an unsorted vector beat `map` and `unordered_map`?
In this setup, never outright. With random keys at n = 16, the vector ties `map` (14.1 vs. 14.6 ns) and loses to `unordered_map` (8.0 ns). At n = 64 it's already 2× slower than `map`, and it reaches 17 µs per lookup at 64K. Linear search loses at small n because the match position is random, so the loop exit is an unpredictable branch: roughly one misprediction per lookup, on top of eight comparisons on average. Meanwhile libc++ hashes a `uint64_t` to itself, so `unordered_map::find` is a modulo and one pointer dereference into a table that fits in L1. The folklore about linear search holds for tiny *sets of small keys* searched with SIMD or branchless code, not for a generic `std::find_if` over 16-byte pairs.

### 2. `sorted_vector` vs. `map`: both O(log n), so why the gap?
At 1M, `sorted_vector` lookups are 1.7× faster (156 vs. 266 ns); at 4K, 2.3× (23.7 vs. 55.4 ns). Same number of comparisons, very different memory traffic:
- A `map` node is 48 bytes (three pointers, a color flag, and the pair), allocated separately. Every level of the tree is a dependent load from an unpredictable address.
- A sorted vector is 16 bytes per entry, packed. The first few probes of every binary search hit the same handful of cache lines, which stay hot, so only the last few levels miss.

### 3. Random vs. sequential lookups: who cares?

| at 1M | random | sequential | change |
|---|---|---|---|
| `map` | 266 | 105 | 2.5× |
| `sorted_vector` | 156 | 98 | 1.6× |
| `unordered_map` | 27.8 | 29.3 | none |

Ordered containers benefit because successive keys share most of their search path, so those nodes are already cached. A hash function deliberately scatters neighbouring keys, so access order is irrelevant at large n. At small n (16–1K), `unordered_map` does get about 3.7× faster with sequential lookups (8.0 → 2.2 ns), but that's the branch predictor learning a short repeating key cycle, not locality.

### 4. What does `reserve()` save?
Without it, the table rehashes about log₂(n) times as it grows, reallocating the bucket array and relinking every node each time. Build cost per element:

| n | `unordered_map` | `+ reserve(n)` | saved |
|---|---|---|---|
| 16 | 68.6 | 42.3 | 38% |
| 64K | 75.6 | 48.9 | 35% |
| 1M | 159 | 95.3 | 40% |

### 5. Iteration: `map` vs. `vector` at 1M
`vector` takes 0.33 ns/element, about one cycle; `map` takes 71.6 ns, **217× slower**. A `vector` of 16-byte pairs packs eight entries into each 128-byte cache line on Apple silicon, and the hardware prefetcher streams the lines in ahead of use. A `map`'s in-order traversal follows pointers to nodes allocated in insertion order, which here is random key order. Each step depends on the previous load, so nothing can be prefetched, and once the tree outgrows the caches nearly every step goes to DRAM. `unordered_map` (30.6 ns) sits in between: still pointer chasing, but its node list is closer to allocation order.

### 6. Vector vs. list sorted insertion at 16K
The vector wins by **16×** (1.75 vs. 28.8 µs per inserted element), and by 1.6× even at 64 elements. Both have to *find* the insertion point by linear search, which is O(n) for both. For the vector that search is a sequential scan, and the O(n) shift is one `memmove`, both bandwidth-bound and prefetch-friendly. For the list the search is a pointer chase through nodes scattered in memory. The list's O(1) splice saves nothing, because the O(n) search dominates and costs far more per step.

### 7. Where do the curves bend?
Google Benchmark reports a 64 KB L1D and a 4 MB L2. That's an approximation on Apple silicon: an M2 performance cluster shares 16 MB of L2, plus an 8 MB system-level cache.

| Curve | Bend | Working set at the bend |
|---|---|---|
| `map` iterate | 1K → 4K (3.6 → 7.8) | 4K nodes × 48 B = 192 KB, past L1 |
| `sorted_vector` lookup | 4K → 16K (23.7 → 49.1) | 16K × 16 B = 256 KB, past L1 |
| `unordered_map` iterate | 4K → 16K (1.6 → 6.0) | 16K × ~40 B = 640 KB, past L1 |
| everything node-based | 256K → 1M (2–4×) | 1M × 40–48 B = 40–48 MB, past L2 and SLC, into DRAM |

The contiguous containers bend late and gently, if at all (`vector` iteration is flat from 64 to 1M). The node-based ones bend at every cache level.

## Methodology Notes
- Inputs consumed by an iteration (build, sorted insertion) cycle through about 1M elements of distinct inputs. The first run reused one input, and `sorted_vector`'s build at n ≤ 1024 came out 2–4× too cheap because the branch predictor memorized `std::sort`'s comparisons. That's the Day 3 lesson, seen again.
- Lookups cycle through at least 64K keys for the same reason.
- `vector` lookups stop at 64K (17 µs each). At 1M, each batch of 1024 lookups would take about a quarter of a second, and that one family would dominate the run.
