# Day 3: Containerized Sorting Benchmarks
A Google Benchmark suite comparing `std::sort`, quicksort, a naive quicksort, and LSD radix sort on four input distributions from 16 to 1M elements, packaged in a multi-stage Docker image. Findings, with numbers, are in [`results/ANALYSIS.md`](results/ANALYSIS.md). The warm-up (porting the Day 2 `chrono` loop to Google Benchmark) lives in `day2-header-library/benchmarks/bench_vec3.cpp`.

## Project Structure

```
day3-docker-benchmarks/
├── include/sorting/
│   ├── sorting.hpp        # quicksort, quicksort_naive, radix_sort
│   └── inputs.hpp         # Deterministic random / sorted / reversed / few_unique inputs
├── tests/test_sorting.cpp # Every algorithm vs. std::sort on edge cases and every distribution
├── benchmarks/bench_sorting.cpp
├── Dockerfile             # build stage (toolchain + tests) → runtime stage (binary only)
├── run_benchmarks.sh      # Build image, run it, write results/docker-<timestamp>.json
└── results/               # native.json, docker-*.json, ANALYSIS.md
```

## Running

**Natively**:
```bash
cmake -B build             # Release by default; downloads Google Benchmark
cmake --build build
(cd build && ctest --output-on-failure)
./build/bench_sorting --benchmark_repetitions=10 --benchmark_min_time=0.1s \
    --benchmark_display_aggregates_only=true \
    --benchmark_out=results/native.json --benchmark_out_format=json
```

**In Docker** (only Docker needed):
```bash
./run_benchmarks.sh                                  # full suite, ~6 minutes
./run_benchmarks.sh --benchmark_filter=radix         # extra flags go to the binary
UBUNTU_VERSION=22.04 ./run_benchmarks.sh             # different base for both stages
```

**Comparing two runs** (Mann-Whitney U test per benchmark):
```bash
pip install -r build/_deps/benchmark-src/tools/requirements.txt
python3 build/_deps/benchmark-src/tools/compare.py benchmarks results/native.json results/docker-<timestamp>.json
```

## Benchmark Families

| Family | What it shows |
|---|---|
| `sort/<algorithm>/<distribution>/N` | The main comparison: copy unsorted input + sort, with a `->Complexity()` fit per family |
| `copy_baseline/random/N` | The copy alone, to subtract from `sort/` |
| `sort_paused/<algorithm>/random/N` | The same work with `PauseTiming` around the copy, to show its overhead |
| `sort_repeated/<algorithm>/random/N` | The *same* array every iteration, to show branch-predictor memorization |

## Design Notes
- **`quicksort`**: median-of-three pivot, Hoare partition (balanced splits on duplicates), and insertion sort below 16 elements. It recurses into the smaller half, so stack depth is O(log n).
- **`quicksort_naive`**: last-element pivot + Lomuto, kept to show O(n²) on sorted, reversed, and few-unique input. Benchmarked only up to 16K.
- **`radix_sort`**: one pass builds all four 8-bit histograms. A pass is skipped when every key shares that digit (3 of 4 for few-unique input). The scratch buffer is allocated on each call, so its cost counts.
- **Fresh inputs**: each `sort/` benchmark cycles through about 1M elements' worth of distinct arrays. Sorting the same small array every iteration lets the branch predictor learn it; see `sort_repeated/` and the analysis.
- **Tests in Release**: the project builds in Release, so the tests use a `check()` helper instead of `assert`, which `NDEBUG` would compile away.
- **Docker**: both stages use the same `ubuntu:${UBUNTU_VERSION}` base, so the runtime `libstdc++` matches the one the binary was built against. Tests run during `docker build`, so a broken sort can't produce an image. The exec-form `ENTRYPOINT` passes flags through. `run_benchmarks.sh` runs as `--user $(id -u):$(id -g)` so result files aren't owned by root on Linux.

Not done from the stretch list: plots (Day 4 covers plotting), a CI smoke job, and `--cpuset-cpus` pinning, which needs a Linux host.
