# Day 3 Analysis: Sorting Benchmarks, Native vs. Docker

**Setup**: Apple M2 (4 performance + 4 efficiency cores), macOS. Native build: Apple Clang 17 + libc++. Container: Ubuntu 24.04 arm64 + GCC 13 + libstdc++, in Docker Desktop's Linux VM. Google Benchmark v1.9.5, Release builds, 10 repetitions × 0.1 s, medians unless noted. Raw data: [`native.json`](native.json), [`docker-20261022-164753.json`](docker-20261022-164753.json).

**Caveat**: the host was not idle. A browser was using about 2.5 cores, and the native run's load average was 7.7 on 8 cores. Treat differences under ~5% with suspicion; question 4 quantifies the noise.

## 1. Where Does Radix Sort Overtake `std::sort`?
Between 16 and 64 elements, on random input:

| N | `std::sort` | `radix_sort` | radix speedup |
|---|---|---|---|
| 16 | 0.20 µs | 0.88 µs | 0.23× (slower) |
| 64 | 1.47 µs | 1.12 µs | 1.3× |
| 1024 | 42.9 µs | 5.56 µs | 7.7× |
| 1M | 91.1 ms | 5.69 ms | 16× |

Radix sort has a fixed cost of about 0.85 µs per call, whatever the input size: zeroing four 256-entry histograms, four 256-entry prefix sums, and allocating the scratch buffer. At 16 elements that overhead *is* the sort. After that, radix costs a flat ~5.5 ns/element, while `std::sort` grows from 23 to 87 ns/element as `log n` and the branch-misprediction rate on random data rise.

## 2. Quicksort on Sorted and Few-Unique Input
- **`quicksort`** (median-of-three, Hoare): sorted and reversed input give a perfect pivot every time: 11.6 and 10.9 ns/element at 1M, about 8× faster than random input (89.9). Few-unique input costs 34.6 ns/element at 1M against `std::sort`'s 19.1. Hoare partitioning keeps duplicate-heavy splits balanced, but still partitions runs of equal keys over and over. A **3-way (Dutch national flag) partition** would put all keys equal to the pivot in their final place in one pass, making few-unique input O(n·k) for k distinct values.
- **`quicksort_naive`** (last-element pivot, Lomuto) is O(n²) where you'd expect. Google Benchmark's complexity fit reports `0.28 N^2` for sorted input, which comes to 318 µs at 1024 and 75 ms at 16K (4.6 µs/element). Reversed input is the same story. Few-unique input degrades more slowly, because only the duplicates pile up on one side. The fix is the one `quicksort` uses: a better pivot, and a partition scheme that splits equal keys.
- For contrast, libc++'s `std::sort` is *linear* on sorted (1.7 ns/element) and reversed (2.0 ns/element) input: it detects already-ordered runs.

## 3. Does the Fitted Complexity Match Theory?
Mostly, and the misses are instructive.

| Family | Fit | Theory |
|---|---|---|
| `std_sort/random`, `quicksort/random` | 4.3 / 4.5 · N lg N | N log N ✓ |
| `quicksort_naive/sorted`, `/reversed` | 0.28 / 0.21 · N² | N² ✓ |
| `std_sort/sorted`, `/reversed` | 1.7 / 2.1 · N | libc++ fast path ✓ |
| `radix_sort/few_unique`, `/reversed` | 5.2 / 7.4 · N | N ✓ |
| `radix_sort/random`, `/sorted` | 0.27 / 0.37 · N lg N | N ✗ |

The radix "N lg N" fits come from the small sizes: the ~0.85 µs fixed cost makes per-element cost *fall* from 55 ns at N=16 to 5.4 ns at N=1024, and the fitter has no constant term to absorb it. Above 1024, ns/element is flat. A fit is a summary, not a proof; always check it against ns/element.

## 4. Noise: Which Benchmarks Vary Most?
- **Native**: median CV 1.4%, but 26% of benchmarks exceed 5%. The worst are `quicksort/reversed/16` (136%), `quicksort/sorted/262144` (119%), and `std_sort/sorted/16` (112%).
- **Docker**: median CV 6.5%, and 57% exceed 5%.

Two patterns stand out. Tiny inputs (N=16) on fast paths run in about 30 ns, where any interruption is a large fraction of the time. And bimodal runs, like `quicksort/sorted/262144`, are consistent with the scheduler moving the thread between a performance and an efficiency core mid-run. macOS doesn't allow pinning a thread to a core. The fixes, in order of impact: an idle machine, more repetitions with the median rather than the mean, and on Linux, `taskset` or `--cpuset-cpus` plus a fixed CPU frequency (see *Reducing Variance*).

## 5. `PauseTiming` vs. a Copy Baseline

| N | copy + sort | paused copy, sort timed | copy alone |
|---|---|---|---|
| 64 (`std::sort`) | 1.47 µs | 2.43 µs | 0.01 µs |
| 64 (`radix_sort`) | 1.12 µs | 2.08 µs | 0.01 µs |
| 16384 (`std::sort`) | 985 µs | 990 µs | 2.4 µs |

At N=64, "pausing" the 10 ns copy *adds* about 1 µs: the pause/resume pair costs far more than the work it excludes. At 16K the overhead disappears into the noise. Trust the copy baseline: it costs 0.1–0.2 ns/element (1–2% of the cheapest sort), and you can subtract it explicitly. Use `PauseTiming` only when the excluded work is large compared with a microsecond.

## 6. Native vs. Docker
`compare.py` (Mann-Whitney U test, 10 vs. 10 repetitions) finds most differences significant (p = 0.0002), with a geometric-mean slowdown of +68%. But that average blends three different effects, and only one of them is "Docker".

At least five variables differ between the two runs: **compiler** (Clang 17 vs. GCC 13), **standard library** (libc++ vs. libstdc++, and so a different `std::sort`), **allocator** (macOS malloc vs. glibc), **OS and page size** (16 KB vs. 4 KB pages), and **virtualization** (Docker Desktop runs a Linux VM). To separate them, I rebuilt natively with Homebrew GCC 16 and reran the radix cases inside the container with different glibc malloc settings.

| Benchmark | Native Clang | Native GCC | Docker | Attribution |
|---|---|---|---|---|
| `quicksort/random/1M` | 94.7 ms | 115.9 ms | 119.1 ms | **Compiler**: GCC is 22% slower natively too |
| `std_sort/random/1M` | 91.6 ms | 93.5 ms | 94.1 ms | Library/compiler, ~2–3% |
| `radix_sort/random/1M` | 5.86 ms | 6.06 ms | 21.5 ms | **Environment**: not the compiler |
| `radix_sort/random/256K` | 1.32 ms | 1.35 ms | 2.30 ms | **Allocator + VM page faults** (below) |

- **Radix sort at 256K**: the 1 MB scratch buffer is freed and reallocated on every call. With default settings, glibc hands that memory back to the kernel, so every call page-faults fresh 4 KB pages, and page faults cost more in a VM. With `GLIBC_TUNABLES=glibc.malloc.mmap_threshold=268435456:glibc.malloc.trim_threshold=1073741824`, which keeps the memory, it drops from 2.04 ms to 1.40 ms, back to native speed.
- **Radix sort at 1M** stays at 13–14 ms with the same settings, and with transparent huge pages forced on. Its 256-way scatter over 8 MB is the most memory-system-sensitive code here. Plausible causes are nested paging under the VM and where the hypervisor schedules the VM's CPUs; confirming would take hardware counters (`perf stat`) on a Linux host. **Unresolved.**
- **Run order matters in the container**: in the full suite, `radix_sort/random/1024` took 26 µs, but run on its own in the same image it took 5.9 µs, matching native. Earlier benchmarks leave the allocator and page state different. Another reason to run each comparison more than once.
- On a Linux host, a container is just namespaces and cgroups on the same kernel, with no VM. CPU-bound code should then match native once the compiler, library, and allocator are held equal.

**Takeaway**: "Docker is 68% slower" would be the wrong conclusion. The compiler accounts for quicksort's gap; the allocator and the VM account for radix sort's. Change one variable at a time.

## 7. Image Size

| Image | Size |
|---|---|
| Build stage (`docker build --target build`) | 542 MB |
| Runtime stage (final image) | 101 MB = 100.6 MB `ubuntu:24.04` + 0.6 MB binary |

The final image has no compiler (`command -v g++ cc cmake` finds nothing). Going smaller would mean a distroless or `scratch` base, plus static linking or a matching glibc.

## 8. Same Input Every Iteration
This one was a surprise. With one fixed random array per size, re-sorted on every iteration:

| N | `std::sort`, same array | `std::sort`, 1M elements of distinct arrays | radix, same | radix, distinct |
|---|---|---|---|---|
| 256 | 1.9 µs | 8.4 µs | | |
| 1024 | 9.2 µs | 42.9 µs | 5.57 µs | 5.56 µs |
| 4096 | 146 µs | 210 µs | | |

At 1024, the repeated array sorts **4.7× faster**. The M2's branch predictor learns the entire sequence of comparison outcomes for an input it keeps seeing. Radix sort doesn't branch on the data, so it's unaffected, and that control is what pins the effect on branch prediction rather than caching. By 4096 the pattern is too long to memorize and the gap closes. A quick prototype cycling through 1, 2, 8, and 64 distinct arrays at N=1024 gave 9.5, 20.8, 35.2, and 41.8 µs.

Every `sort/` benchmark therefore cycles through about 1M elements' worth of distinct arrays. Without that, the first version of this suite reported `std::sort` at 1024 as "111M items/s" instead of the real ~24M.
