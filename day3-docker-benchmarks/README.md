# Day 3: Containerized Benchmark Runner
**Goal**: Measure performance correctly, and make the measurements reproducible with Docker

You'll learn Google Benchmark on code you already know (Day 2), use it to compare three sorting algorithms, then package the suite in a Docker image anyone can run with one command. Readings are listed under Day 3 in the [main README](../README.md#day-3-containerized-benchmark-runner).

## Deliverables

```
day3-docker-benchmarks/
├── CMakeLists.txt             # FetchContent for Google Benchmark; tests + benchmarks targets
├── Dockerfile                 # Multi-stage: build → runtime
├── .dockerignore
├── run_benchmarks.sh          # Build image, run it, write results/<timestamp>.json
├── include/sorting/sorting.hpp
├── tests/test_sorting.cpp
├── benchmarks/bench_sorting.cpp
└── results/
    └── ANALYSIS.md            # Your findings, with numbers
```

## Milestones

### 1. Warm-up: Google Benchmark on Day 2 (~1h)
Add Google Benchmark to your Day 2 project with `FetchContent` and port `simple_benchmark.cpp`, one benchmark per operation.
- Pin a release tag, not `main`.
- Set `BENCHMARK_ENABLE_TESTING` to `OFF` before `FetchContent_MakeAvailable`, or configuring will go looking for GoogleTest too.
- Build in Release. Read the header Google Benchmark prints before the numbers: `Library was built as DEBUG` means the numbers are meaningless.

**Question**: Your `chrono` loop and a naive port can both end up timing nothing. Why? (Hint: `v1` and `v2` never change inside the loop.) Confirm it on Compiler Explorer, then fix the benchmark so it measures the operation itself.

### 2. Sorting Algorithms, Tested First (~1.5h)
In `sorting.hpp`, implement for `std::span<std::uint32_t>`:
- `quicksort`: your pivot choice matters later, so note what you picked
- `radix_sort`: LSD, 8-bit digits (4 passes), with a scratch buffer

`test_sorting.cpp` checks both against `std::sort` on empty, single-element, all-equal, sorted, reversed, and seeded random input. Register it with CTest. A fast wrong sort is worthless.

### 3. Benchmark Suite (~1.5h)
`bench_sorting.cpp` benchmarks `std::sort`, `quicksort`, and `radix_sort` on:
- **Sizes** 2^4 to 2^20 (`RangeMultiplier` + `Range`)
- **Distributions**: random, sorted, reversed, few unique (e.g., values in `[0, 16)`)

Requirements:
- Generate inputs outside the timed loop, with fixed seeds.
- **Each iteration must sort unsorted data.** Sorting in place destroys the input, so from the second iteration on you'd be timing already-sorted input. You can either wrap the copy in `PauseTiming`/`ResumeTiming`, or time copy + sort and add a copy-only baseline. Try both at N = 64 and N = 1024, and explain the difference.
- Register one family per algorithm × distribution (`BENCHMARK_CAPTURE`, or `benchmark::RegisterBenchmark` in a loop), and add `->Complexity()` so Google Benchmark fits the big-O for each.
- Call `SetItemsProcessed` to get elements/second alongside time.

Run natively and keep the result:

```bash
./build/bench_sorting --benchmark_repetitions=10 --benchmark_min_time=0.1s --benchmark_display_aggregates_only=true \
    --benchmark_out=results/native.json --benchmark_out_format=json
```

Use `display_aggregates_only`, not `report_aggregates_only`: `compare.py` needs the individual repetitions in the JSON.

### 4. Containerize (~1.5h)
**Dockerfile**:
- **Build stage**: compiler, CMake, and git (`FetchContent` clones at configure time). Configure in Release, build, and run the tests, so a failing test fails the image build.
- **Runtime stage**: a slim base with only the benchmark binary copied in. Use an exec-form `ENTRYPOINT` so flags pass through: `docker run bench --benchmark_filter=Radix`.
- **`.dockerignore`**: exclude `build/` and `results/`.

**`run_benchmarks.sh`**: builds the image, mounts `results/` into the container, writes `results/<timestamp>.json`, and forwards extra arguments to the binary.

**Gotchas**:
- `GLIBCXX_3.4.xx not found` at runtime means the build and runtime stages ship different `libstdc++` versions. Use the same distro for both, or link with `-static-libstdc++`.
- On Apple Silicon, build native `arm64` images. `--platform linux/amd64` runs under emulation, and its timings are meaningless.
- On Linux, pass `--user "$(id -u):$(id -g)"` to `docker run`, or your result files will be owned by root.
- `docker build --target <build-stage-name>` gives you the single-stage image for free. Compare its size to the final image.

### 5. Analysis (~1h)
In `results/ANALYSIS.md`, answer each question with numbers from your runs:
1. On random input, at what N does radix sort overtake `std::sort`? Why does it lose below that?
2. How does your quicksort do on sorted and few-unique input? What would fix it?
3. Does the fitted complexity match theory for each algorithm?
4. Which benchmarks have the highest coefficient of variation (`_cv`)? What would you change to reduce it?
5. `PauseTiming` vs. copy baseline at N = 64 and N = 1024: which would you trust, and why?
6. Native vs. container: run `compare.py benchmarks results/native.json results/<docker>.json` (it lives in `build/_deps/benchmark-src/tools` if you named the dependency `benchmark`; run `pip install -r requirements.txt` there first). Is the difference significant? List every variable that differs between the two runs before you blame Docker. On macOS, the compiler, OS, and a VM all change at once.
7. Single-stage vs. multi-stage image size.
8. Your random input is probably the same array every iteration. Does that matter? At N ≤ 4096, compare it against cycling through many distinct arrays, for both a comparison sort and radix sort.

## Done When
- [ ] `ctest` passes natively and inside `docker build`
- [ ] On a fresh clone with only Docker installed, `./run_benchmarks.sh` produces a JSON file in `results/`
- [ ] The runtime image contains no compiler
- [ ] `ANALYSIS.md` answers every question with numbers

## If Time Permits
- Plot ns/element vs. N (log x-axis) for each distribution
- Add a CI job that builds the image and smoke-runs the suite with `--benchmark_min_time=0.01s`. Shared runners are noisy, so never fail CI on timings
- On Linux, pin the container to a single core (`--cpuset-cpus`) and see whether `_cv` drops
