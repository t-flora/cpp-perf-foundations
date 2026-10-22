# Day 4: Data Structure Performance Explorer
**Goal**: Know which container to reach for, backed by your own measurements

Big-O tells you how cost grows. It says nothing about the constant, and on modern hardware the constant is mostly memory. You'll measure four ways to store key → value data, plus the classic vector vs. list experiment, from 16 to 1M elements, then turn the numbers into a decision guide. Readings are listed under Day 4 in the [main README](../README.md#day-4-data-structure-performance-explorer).

## Deliverables

```
day4-container-performance/
├── CMakeLists.txt                     # Same FetchContent setup as Day 3
├── include/containers/workloads.hpp   # Containers behind one interface + input generators
├── tests/test_workloads.cpp
├── benchmarks/bench_containers.cpp
├── scripts/plot_results.py            # JSON → CSV + PNG
└── results/
    ├── results.json, results.csv, containers.png
    └── DECISION_GUIDE.md
```

## Milestones

### 1. One Interface, Five Containers (~1.5h)
Wrap each container in a struct with `build(entries)`, `find(key) -> const Value*` (`nullptr` if missing), and `sum_values()`. Keys and values are `std::uint64_t`.

| Name | Storage | `find` |
|---|---|---|
| `map` | `std::map` | tree search |
| `unordered_map` | `std::unordered_map` | hash |
| `unordered_map_reserved` | the same, with `reserve(n)` before building | hash |
| `sorted_vector` | `std::vector<std::pair<K, V>>`, sorted by key once | `std::lower_bound` |
| `vector` | `std::vector<std::pair<K, V>>`, unsorted | linear `std::find_if` |

You also need deterministic input generators:
- `n` unique random keys. Hint: a bijective mixing function, like splitmix64's finalizer, gives unique keys without a dedup pass.
- Lookup sequences over existing keys, in random order and in sorted order.

Also write `insert_sorted(seq, values)`, which inserts each value at its sorted position, found by linear search. It needs to work for both `std::vector` and `std::list`.

**Tests**: for every container, every key returns its value, missing keys return `nullptr`, and `sum_values()` matches. `insert_sorted` must give the same sequence for vector and list. A benchmark comparison is only valid if every variant computes the same thing.

### 2. Benchmarks (~2h)
| Family | Each iteration |
|---|---|
| `lookup/<container>/<random\|sequential>/N` | A batch of lookups, e.g. 1024 |
| `build/<container>/N` | Build from N entries, then destroy |
| `iterate/<container>/N` | Sum all values |
| `insert_sorted/<vector\|list>/N` | Sorted insertion of N values (it's O(n²), so stop at 16K) |

Use sizes from 16 to 1M in steps of ×4. That covers the 100 / 10K / 1M points and every cache level in between.

Requirements:
- Call `SetItemsProcessed` everywhere, so every family plots as ns per item.
- Lookups must cycle through a long key sequence (≥ 64K keys). If you repeat the same few lookups, you're measuring the cache and the branch predictor, not the container. Day 3 showed what that does to your numbers.
- Cap the linear-search lookups. At 1M they'd take minutes.

```bash
./build/bench_containers --benchmark_out=results/results.json --benchmark_out_format=json
```

### 3. Plot (~1h)
`scripts/plot_results.py` reads the JSON and writes:
- `results.csv`, with one row per (family, container, pattern, N) and an `ns_per_item` column
- `containers.png`, with one panel per family/pattern, one line per container, and log-log axes. Give each container the same color in every panel, and a distinct marker as well, so the lines can be told apart without color.

### 4. Analysis and Decision Guide (~1.5h)
In `results/DECISION_GUIDE.md`, answer each question with numbers:
1. Up to what N does linear search in an unsorted vector beat `map` and `unordered_map` lookups?
2. `sorted_vector` and `map` lookups are both O(log n). Which is faster at 1M, by how much, and why?
3. Random vs. sequential lookups: which containers care, and why?
4. How much does `reserve()` save when building an `unordered_map`?
5. Iteration: compare ns per element for `map` and `vector` at 1M, and relate it to cache lines.
6. Vector vs. list sorted insertion at 16K: a list insert is O(1) and a vector insert is O(n). Which wins, and why?
7. Where do the curves bend? Match the bends to the cache sizes in the header Google Benchmark prints.

Then write the guide: a table of *if your workload is… → use… → because (measured number)*.

## Done When
- [ ] `ctest` passes
- [ ] One command reproduces `results.json`, and one more reproduces the CSV and PNG
- [ ] Every row of the decision guide cites a measurement

## If Time Permits
- Add `absl::flat_hash_map` or `boost::unordered_flat_map` via `FetchContent`: how much of `unordered_map`'s cost comes from its node-based design?
- Try `std::flat_map` (C++23) if your standard library ships it
- Back `std::map`'s nodes with `std::pmr::monotonic_buffer_resource`: how much of the build cost is `malloc`?
- Run the suite in your Day 3 container and compare
