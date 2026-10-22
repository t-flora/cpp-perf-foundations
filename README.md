# C++ for High-Performance Engineering: 6-Day Foundational Project Series

A progressive series of one-day C++ projects designed to build the foundational skills needed for high-performance systems development. Each project introduces essential concepts while building toward the competence required for high-performance engineering. Some of these projects may take longer than a day, and that's fine!
> **Complete Solutions**: This is the **main branch** containing project implementations.
> 
> **For learners**: Clone the template branch to get started:
> ```bash
> git clone -b template https://github.com/t-flora/cpp-perf-foundations.git
> ```
> 
> **For instructors/reference**: You're in the right place! This branch contains implementations.

## Branch Structure

This repository uses a two-branch approach:

- **`template` branch**: Empty project templates with starter code and TODO comments for learners
- **`main` branch**: Complete implementations and solutions for reference

**Workflow for learners**:
```bash
# Clone template branch to start learning
git clone -b template https://github.com/t-flora/cpp-perf-foundations.git
cd cpp-perf-foundations

# Work on your solutions...

# Optional: Compare with complete solutions  
git remote add upstream https://github.com/t-flora/cpp-perf-foundations.git
git fetch upstream main
git checkout -b solutions upstream/main
```

## Repository Structure

```
cpp-foundations/
├── README.md                 # This file
├── scripts/
│   └── test_all.sh          # Build and test every day
├── .github/
│   └── workflows/
│       └── day1-ci.yml      # GitHub Actions CI for Day 1
├── day1-cmake-ci/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── build_and_test.sh
│   ├── src/
│   │   ├── CMakeLists.txt
│   │   ├── calculator.hpp
│   │   └── main.cpp
│   └── tests/
│       ├── CMakeLists.txt
│       └── test_calculator.cpp
├── day2-header-library/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── cmake/
│   │   └── vec3Config.cmake.in
│   ├── include/
│   │   └── vec3/
│   │       └── vec3.hpp
│   ├── examples/
│   │   └── consumer/        # Uses the installed library via find_package
│   ├── benchmarks/
│   │   ├── simple_benchmark.cpp
│   │   └── bench_vec3.cpp
│   └── tests/
│       ├── test_vec3_basics.cpp
│       └── test_vec3_constexpr.cpp
├── day3-docker-benchmarks/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── Dockerfile
│   ├── run_benchmarks.sh
│   ├── include/
│   │   └── sorting/
│   │       ├── sorting.hpp
│   │       └── inputs.hpp
│   ├── benchmarks/
│   │   └── bench_sorting.cpp
│   ├── tests/
│   │   └── test_sorting.cpp
│   └── results/
│       └── ANALYSIS.md
├── day4-container-performance/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── containers/
│   │       └── workloads.hpp
│   ├── benchmarks/
│   │   └── bench_containers.cpp
│   ├── tests/
│   │   └── test_workloads.cpp
│   ├── scripts/
│   │   └── plot_results.py
│   └── results/
│       └── DECISION_GUIDE.md
├── day5-realtime-processor/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── src/
│   │   └── market_data_processor.cpp
│   ├── data/
│   │   └── sample_trades.csv
│   └── tests/
│       └── test_vwap.cpp
└── day6-concurrent-calculator/
    ├── README.md
    ├── CMakeLists.txt
    ├── src/
    │   ├── thread_pool.h
    │   ├── concurrent_calculator.cpp
    │   └── benchmark_concurrency.cpp
    ├── tests/
    │   └── test_thread_safety.cpp
    └── scripts/
        └── plot_scaling.py
```

## Project Overview

### Day 1: CMake + CI Hello World
**Goal**: Master the build system every C++ project uses

**What You'll Build**: A small calculator library, a demo executable, and a CI pipeline that builds and tests both
- `calculator` library with add/subtract/multiply/divide (header-only or `.h`/`.cpp`, your call)
- `main.cpp` demo, plus a test executable registered with CTest
- Target-based CMake: one library target that the executables link against
- GitHub Actions workflow that builds and tests on your platform

*If time permits*:
- Extend the CI matrix to Linux, macOS, and Windows
- Run the workflow locally with [act](https://github.com/nektos/act)
- Add a `.clang-format` and check formatting in CI

**Skills Gained**:
- CMake targets, linking, and usage requirements
- CI with GitHub Actions
- Project layout and consistent code style

**Essential Reading** (2-3 hours before coding):
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/) - Official tutorial, steps 1-4
- [Modern CMake: The Basics](https://cliutils.gitlab.io/modern-cmake/basics) - Target-based CMake
- [GitHub Actions CMake Workflow](https://github.com/actions/starter-workflows/blob/main/ci/cmake-single-platform.yml) - Starter template
- [CMake Template Project](https://github.com/cpp-best-practices/cmake_template) - Complete example with CI

**Why it matters**: Every real C++ project starts here. Without a reliable build and CI, you can't collaborate on or ship code.

---

### Day 2: Header-Only Math Library
**Goal**: Understand how modern C++ libraries are distributed

**What You'll Build**: A templated 3D vector type, `Vec3<T>`, in a single header
- Arithmetic operators, `dot`, `cross`, `length`, `normalized`, `constexpr` wherever possible
- Tests, including `static_assert`s that prove operations evaluate at compile time
- An `INTERFACE` CMake target with install/export rules, and a small consumer project that uses it through `find_package(vec3)`
- A naive `std::chrono` timing loop over your operations. Note what makes it unreliable; Day 3 replaces it

*If time permits*:
- Expression templates for chains like `a + b + c`; compare the assembly on Compiler Explorer
- Switch the tests to Google Test

**Skills Gained**:
- Templates and `constexpr`
- Header-only library design and CMake packaging
- API design principles

**Essential Reading** (2-3 hours before coding):
- [Effective Modern C++ - Item 30](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/) - Perfect forwarding and where it breaks
- [Expression Templates Explained (ModernesCpp)](https://www.modernescpp.com/index.php/expression-templates) - Removing temporaries from math expressions
- [C++ Reference: constexpr specifier](https://en.cppreference.com/w/cpp/language/constexpr) - Compile-time computation
- [Google Highway SIMD Library](https://github.com/google/highway) - Cross-platform vectorization basics
- [API Design Principles](https://wiki.qt.io/API_Design_Principles) - Designing clean interfaces

*Additional resources*:
- [Modern C++ Header-Only Libraries](https://github.com/p-ranav/awesome-hpp) - Examples to learn from
- [Google Test Primer](http://google.github.io/googletest/primer.html) - Testing fundamentals
- [Importing and Exporting Guide](https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html) - Making `find_package` work for your library

**Tools for quick performance checks**:
- [Compiler Explorer](https://godbolt.org/) - See the assembly your code produces
- [Quick Bench](https://quick-bench.com/) - Online benchmarking for snippets
- [C++ Insights](https://cppinsights.io/) - See what templates instantiate to

**Why it matters**: Many high-performance libraries (Eigen, GLM, much of Boost) are header-only: the compiler sees every definition, so it can inline and optimize across calls. You'll use libraries like this constantly, and eventually write them.

---

### Day 3: Containerized Benchmark Runner
**Goal**: Measure performance correctly, and make the measurements reproducible with Docker

**What You'll Build**: A Google Benchmark suite comparing sorting algorithms, packaged in a multi-stage Docker image
- Warm-up: port your Day 2 `chrono` loop to Google Benchmark and explain why the numbers differ
- Your own quicksort and LSD radix sort, tested for correctness against `std::sort`
- Benchmarks across sizes (16 to 1M elements) and input distributions (random, sorted, reversed, few unique), with repetitions and JSON output
- Multi-stage Dockerfile: toolchain in the build stage, only the binary in the runtime stage
- `run_benchmarks.sh`, which builds the image, runs it, and writes timestamped results to `results/`
- A short `ANALYSIS.md` comparing the algorithms, and native vs. containerized runs

*If time permits*:
- Plot ns/element vs. size for each distribution
- A CI job that builds the image and smoke-runs the benchmarks (never gate CI on timings)

**Skills Gained**:
- Benchmark design: defeating the optimizer, controlling timers, repetitions
- Reading results statistically instead of anecdotally
- Multi-stage Docker builds for C++
- Fetching dependencies with CMake `FetchContent`

**Essential Reading** (2-3 hours before coding):
- [Google Benchmark User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md) - Passing arguments, asymptotic complexity, controlling timers, preventing optimization, repetitions
- [Reducing Variance](https://github.com/google/benchmark/blob/main/docs/reducing_variance.md) - Why your numbers jump around
- [Tuning C++: Benchmarks, and CPUs, and Compilers! Oh My!](https://www.youtube.com/watch?v=nXaxk27zwlk) - Chandler Carruth, CppCon 2015; live benchmarking pitfalls
- [Docker Multi-Stage Builds](https://docs.docker.com/build/building/multi-stage/) - Separating build and runtime images

*Additional resources*:
- [Google Benchmark Tools](https://github.com/google/benchmark/blob/main/docs/tools.md) - `compare.py` for statistically comparing two runs
- [Radix Sort Revisited](http://www.codercorner.com/RadixSortRevisited.htm) - Practical LSD radix sort for 32-bit keys
- [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) - Pulling dependencies at configure time
- [Docker C++ Guide](https://docs.docker.com/guides/cpp/) - Containerizing a C++ project end to end

**Why it matters**: Optimization decisions are only as good as the measurements behind them. A number you can't reproduce on another machine, in CI, or next month is an anecdote. Benchmark discipline plus a pinned environment turns it into evidence.

---

### Day 4: Data Structure Performance Explorer
**Goal**: Know which container to reach for, backed by your own measurements

**What You'll Build**: A benchmark suite comparing key-value containers and sequences, plus plots and a decision guide
- `std::map`, `std::unordered_map` (with and without `reserve`), sorted `std::vector` + binary search, and unsorted `std::vector` + linear search, behind one interface and tested to give identical answers
- Lookup (random vs. sequential keys), build, and iterate benchmarks from 16 to 1M elements
- The classic vector vs. list sorted-insertion experiment
- A Python script that turns the JSON results into a CSV and log-log plots
- A decision guide where every recommendation cites a measurement

*If time permits*:
- Add an open-addressing hash map (`absl::flat_hash_map`, `boost::unordered_flat_map`) or C++23 `std::flat_map`
- Back `std::map` with a `std::pmr` arena and measure how much of its cost is allocation

**Skills Gained**:
- How caches turn the same Big-O into very different running times
- Container internals: nodes vs. contiguous storage, hashing, rehashing
- Benchmarking across sizes and access patterns
- Data analysis and visualization with Python

**Essential Reading** (2-3 hours before coding):
- [Efficiency with Algorithms, Performance with Data Structures](https://www.youtube.com/watch?v=fHNmRkzxHWs) - Chandler Carruth, CppCon 2014; why contiguous memory wins
- [Gallery of Processor Cache Effects](http://igoro.com/archive/gallery-of-processor-cache-effects/) - Seven short experiments to reproduce in your head
- [Containers Library (cppreference)](https://en.cppreference.com/w/cpp/container) - Complexity guarantees for every container
- [What Is Cache-Friendly Code?](https://stackoverflow.com/questions/16699247/what-is-a-cache-friendly-code) - Memory layout and performance

*Additional resources*:
- [Designing a Fast, Efficient, Cache-friendly Hash Table](https://www.youtube.com/watch?v=ncHmEUmJZf4) - Matt Kulukundis, CppCon 2017; what `std::unordered_map` leaves on the table
- [Abseil Containers Guide](https://abseil.io/docs/cpp/guides/container) - Flat vs. node hash maps in practice
- [What Every Programmer Should Know About Memory](https://people.freebsd.org/~lstewart/articles/cpumemory.pdf) - Ulrich Drepper; sections 3 and 6
- [When to Use Which Container](https://stackoverflow.com/questions/471432/in-which-scenario-do-i-use-a-particular-stl-container) - Practical decision guide

**Why it matters**: Big-O hides the constant, and on modern hardware the constant is mostly memory: one cache miss costs as much as hundreds of arithmetic operations. The wrong container can cost an order of magnitude, and only measurement shows where.

---

### Day 5: Real-Time Data Processor
**Goal**: Handle streaming data efficiently

**What You'll Build**: Mock data processor with real-time analytics
- Parse CSV stream of data records (timestamp, identifier, value, quantity)
- Calculate moving averages and other windowed statistics per identifier
- Implement sliding window calculations using ring buffers
- Output real-time results with configurable update frequency
- Handle data validation and error cases

**Skills Gained**:
- Stream processing patterns
- Circular buffers and windowed calculations
- Real-time analytics concepts
- Real-time system design principles

**Essential Reading** (2-3 hours before coding):
- [Circular Buffer Implementation](https://www.embedded.com/ring-buffer-basics/) - Lock-free ring buffer concepts
- [VWAP Calculation](https://www.investopedia.com/terms/v/vwap.asp) - Understanding volume-weighted average price
- [Stream Processing Patterns](https://www.oreilly.com/radar/the-world-beyond-batch-streaming-101/) - Real-time data processing fundamentals
- [CSV Parsing in C++](https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c) - Efficient file processing techniques

### Day 6: Concurrent Task Processor
**Goal**: Learn thread safety and parallel processing fundamentals

**What You'll Build**: Multi-threaded mathematical computation system
- Thread pool for distributing CPU-intensive calculations
- Thread-safe queue using std::mutex and std::condition_variable
- Parallel processing of mathematical operations (matrix multiplication, prime finding)
- Benchmark comparing single-threaded vs multi-threaded performance
- Proper synchronization and resource management

**Skills Gained**:
- std::thread, std::mutex, std::condition_variable
- Thread pool design patterns
- Race condition prevention
- Lock-free data structures (std::atomic basics)
- Scalability measurement and analysis

**Essential Reading** (2-3 hours before coding):
- (Paid textbook) [C++ Concurrency in Action - Chapter 2](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition) - Managing threads
- [Thread Pool Tutorial](https://en.cppreference.com/w/cpp/thread) - Basic threading primitives
- [Condition Variables in C++](https://www.geeksforgeeks.org/cpp/cpp-multithreading-condition-variables/) - Synchronization fundamentals
- [std::atomic Basics](https://en.cppreference.com/w/cpp/atomic/atomic) - Lock-free programming introduction
- [Core Guidelines - CP](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-concurrency) - Best practices from Core Guidelines

**Why it matters**: High-performance systems are inherently concurrent - data processing, task scheduling, and resource management all happen in parallel. Understanding thread safety is essential for scalable systems.

---

### Prerequisites
- A C++20 compiler (GCC 10+, Clang 12+, or MSVC 2019 16.10+); Day 1 only needs C++17
- CMake 3.16+
- Git
- Docker (Day 3)
- Python 3.8+ with numpy and scipy (Day 3 `compare.py`) and matplotlib (Day 4 plots)

### Building a Project
Each day is a standalone CMake project:
```bash
cd day1-cmake-ci
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
(cd build && ctest --verbose)
```

`scripts/test_all.sh` builds and tests every day (or only the ones you name) in `build/<day>/`.

## Learning Progression

Each day builds on the previous:

1. **Build Systems** → You can compile and distribute C++ code professionally
2. **Header Libraries** → You understand modern C++ distribution patterns
3. **Containers + Benchmarks** → You can measure performance and trust the numbers
4. **Data Structures** → You make informed choices about algorithms and containers
5. **Stream Processing** → You can handle real-time data efficiently
6. **Concurrency** → You understand thread safety and parallel processing fundamentals

## Success Criteria

After completing all 6 projects, you should be able to:
- Set up a new C++ project with CMake and CI from scratch
- Design and implement header-only libraries
- Write and interpret performance benchmarks
- Choose appropriate data structures for different use cases
- Process streaming data efficiently
- Write thread-safe concurrent code
- Containerize C++ applications for deployment

## Next Steps

With these foundational skills mastered, you'll be ready to tackle more ambitious projects like:
- High-performance order book implementation
- Low-latency data processing systems
- Distributed system components
- Real-time analytics engines

## Daily Study Schedule
Each day should follow this pattern:
1. **Morning (2-3 hours)**: Read assigned materials for the day
2. **Afternoon (4-5 hours)**: Implement the project
3. **Evening (1 hour)**: Document learnings and prepare for next day

**Time Management**: Don't spend more than 3 hours on reading per day. The goal is practical competence, not theoretical mastery.

## Versioning and Releases

This project follows [semantic versioning](https://semver.org/) adapted for educational content:
- **Major**: Breaking changes to project structure or prerequisites
- **Minor**: New projects or significant enhancements  
- **Patch**: Bug fixes and documentation updates

## Contributing

This is a learning repository. Each project should be:
- Completable in one focused day (6-8 hours)
- Well-documented with clear learning objectives
- Professionally structured with proper testing
- Incrementally building toward real-world skills

---

**Remember**: The goal isn't to build the most sophisticated systems possible, but to develop the foundational competence that makes sophisticated systems _achievable_ in the first place.