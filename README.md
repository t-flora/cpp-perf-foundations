# C++ Foundations: 6-Day Project Series

A progressive series of one-day C++ projects designed to build the foundational skills needed for high-performance financial systems development. Each project introduces essential concepts while building toward the competence required for trading systems engineering.

## Repository Structure

```
cpp-foundations/
├── README.md                 # This file
├── CMakeLists.txt           # Root CMake file
├── .github/
│   └── workflows/
│       └── ci.yml           # GitHub Actions CI
├── common/
│   ├── CMakeLists.txt       # Shared utilities
│   └── include/
│       └── common/
│           └── timer.h      # Shared timing utilities
├── day1-cmake-ci/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── calculator.h
│   │   ├── calculator.cpp
│   │   └── main.cpp
│   └── tests/
│       └── test_calculator.cpp
├── day2-header-library/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── vector_math/
│   │       └── vector_math.h
│   ├── examples/
│   │   └── example_usage.cpp
│   └── tests/
│       └── test_vector_math.cpp
├── day3-docker-benchmarks/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── Dockerfile
│   ├── run_benchmarks.sh
│   ├── src/
│   │   └── sorting_benchmarks.cpp
│   └── results/
│       └── .gitkeep
├── day4-container-performance/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── src/
│   │   └── container_comparison.cpp
│   ├── scripts/
│   │   └── plot_results.py
│   └── results/
│       └── .gitkeep
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
**Goal**: Master the build system that every C++ project uses

**What You'll Build**: Simple calculator library with multiple source files
- `calculator.h/cpp` with add/subtract/multiply/divide functions
- `main.cpp` that demonstrates library usage
- CMake configuration for library and executable
- GitHub Actions CI pipeline for Linux/macOS
- clang-format integration for code style

**Skills Gained**: 
- CMake basics (targets, linking, dependencies)
- CI/CD setup with GitHub Actions
- Professional C++ project structure
- Code formatting and style consistency

**Essential Reading** (2-3 hours before coding):
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/) - Official tutorial, focus on steps 1-4
- [Modern CMake Practices](https://cliutils.gitlab.io/modern-cmake/chapters/basics.html) - Modern target-based approach
- [GitHub Actions for C++](https://docs.github.com/en/actions/automating-builds-and-tests/building-and-testing-cpp) - Basic CI setup

**Why It Matters**: Every real C++ project needs this foundation. Without solid build systems, you can't collaborate or deploy code professionally.

---

### Day 2: Header-Only Math Library
**Goal**: Understand how modern C++ libraries are distributed

**What You'll Build**: Vector/matrix operations library for 2D/3D vectors
- Single header file with templated vector operations
- Comprehensive unit tests with Google Test
- Example usage demonstrating the API
- CMake export files for easy integration
- Documentation with usage examples

**Skills Gained**:
- Template programming fundamentals
- Header-only library design patterns
- Package management and distribution
- API design principles

**Essential Reading** (2-3 hours before coding):
- [C++ Templates: The Complete Guide](https://www.josuttis.com/tmplbook/) - Chapter 1 (Function Templates)
- [Header-Only Libraries](https://vector-of-bool.github.io/2018/10/05/header-only.html) - Best practices and pitfalls
- [Google Test Primer](https://google.github.io/googletest/primer.html) - Testing fundamentals
- [API Design Guidelines](https://www.qt.io/blog/2009/08/12/designing-qt-style-c-apis) - Creating clean interfaces

**Why It Matters**: Most high-performance C++ libraries are header-only for optimal compilation and inlining. This pattern is essential for high-performance systems.

---

### Day 3: Containerized Benchmark Runner
**Goal**: Learn Docker + performance measurement

**What You'll Build**: Benchmark suite comparing sorting algorithms
- Performance comparison of std::sort vs quicksort vs radix sort
- Google Benchmark integration with proper statistical analysis
- Multi-stage Dockerfile (separate build and runtime stages)
- Automated script to run containers and export results
- Performance graphs and analysis

**Skills Gained**:
- Docker containerization for C++ projects
- Scientific performance measurement
- Benchmark design and interpretation
- CI integration with performance testing

**Essential Reading** (2-3 hours before coding):
- [Google Benchmark User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md) - Focus on basic usage and avoiding optimization
- [Docker Best Practices for C++](https://docs.docker.com/language/cpp/) - Multi-stage builds and optimization
- [Performance Analysis Basics](https://easyperf.net/blog/2019/02/09/Top-Down-performance-analysis-methodology) - Understanding bottlenecks
- [Sorting Algorithms Overview](https://www.toptal.com/developers/sorting-algorithms) - Visual comparison of different approaches

**Why It Matters**: Modern systems run in containers, and performance measurement is critical for optimization decisions.

---

### Day 4: Data Structure Performance Explorer
**Goal**: Understand when to use different containers

**What You'll Build**: Systematic performance comparison of STL containers
- Compare std::vector, std::unordered_map, std::map, std::set for different operations
- Vary data sizes (100, 10K, 1M elements) and access patterns
- Generate CSV results with timing data
- Python script to create performance visualizations
- Decision guide for container selection

**Skills Gained**:
- Deep understanding of STL container performance characteristics
- Systematic benchmarking methodology
- Data analysis and visualization
- Performance-driven decision making

**Essential Reading** (2-3 hours before coding):
- [STL Container Performance](https://www.cppreference.com/w/cpp/container) - Time complexity tables for all containers
- [Cache-Friendly Data Structures](https://stackoverflow.com/questions/16699247/what-is-a-cache-friendly-code) - Memory layout impact on performance
- [Big O Cheat Sheet](https://www.bigocheatsheet.com/) - Quick reference for algorithmic complexity
- [When to Use Which Container](https://stackoverflow.com/questions/471432/in-which-scenario-do-i-use-a-particular-stl-container) - Practical decision guide
- [Python matplotlib Tutorial](https://matplotlib.org/stable/tutorials/introductory/pyplot.html) - For data visualization

**Why It Matters**: High-performance systems live or die on container choice. A wrong data structure can cost microseconds per operation, which can add up very quickly.

---

### Day 5: Real-Time Data Processor
**Goal**: Handle streaming data efficiently

**What You'll Build**: Mock market data processor with real-time analytics
- Parse CSV stream of trades (timestamp, symbol, price, quantity)
- Calculate Volume-Weighted Average Price (VWAP) per symbol
- Implement sliding window calculations using ring buffers
- Output real-time results with configurable update frequency
- Handle data validation and error cases

**Skills Gained**:
- Stream processing patterns
- Circular buffers and windowed calculations
- Basic financial market concepts (VWAP)
- Real-time system design principles

**Essential Reading** (2-3 hours before coding):
- [Circular Buffer Implementation](https://www.embedded.com/ring-buffer-basics/) - Lock-free ring buffer concepts
- [VWAP Calculation](https://www.investopedia.com/terms/v/vwap.asp) - Understanding volume-weighted average price
- [Stream Processing Patterns](https://www.oreilly.com/radar/the-world-beyond-batch-streaming-101/) - Real-time data processing fundamentals
- [CSV Parsing in C++](https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c) - Efficient file processing techniques
- [Market Data Basics](https://www.cmegroup.com/education/courses/introduction-to-market-data.html) - Understanding trading data formats

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
- [C++ Concurrency in Action - Chapter 2](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition) - Managing threads
- [Thread Pool Tutorial](https://en.cppreference.com/w/cpp/thread) - Basic threading primitives
- [Mutex and Condition Variables](https://thispointer.com/c11-multithreading-part-6-condition-variable-example/) - Synchronization fundamentals
- [std::atomic Basics](https://en.cppreference.com/w/cpp/atomic/atomic) - Lock-free programming introduction
- [Thread Safety Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-concurrency) - Best practices from Core Guidelines

**Why It Matters**: Many modern systems are inherently concurrent - e.g. in trading, market data processing, order management, and risk calculations all happen in parallel. Understanding thread safety is essential.

---

### Prerequisites
- C++17 or later compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.16+
- Git
- Docker (for Day 3)
- Python 3.8+ with matplotlib (for Day 4 visualization)

### Building All Projects
```bash
git clone <your-repo-url>
cd cpp-foundations
mkdir build && cd build
cmake ..
make -j$(nproc)
ctest  # Run all tests
```

### Building Individual Projects
```bash
cd day1-cmake-ci
mkdir build && cd build
cmake ..
make
./calculator_demo
```

## Learning Progression

Each day builds on the previous:

1. **Build Systems** → You can compile and distribute C++ code professionally
2. **Header Libraries** → You understand modern C++ distribution patterns
3. **Containers + Benchmarks** → You can measure and optimize performance
4. **Data Structures** → You make informed choices about algorithms and containers
5. **Stream Processing** → You can handle real-time data like trading systems
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
- Low-latency market data processing systems
- Distributed trading system components
- Real-time risk management systems

## Resources

### Essential Reading
- [Modern CMake Tutorial](https://cliutils.gitlab.io/modern-cmake/)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [Docker Best Practices for C++](https://docs.docker.com/language/cpp/)

### Financial Context
- [Market Microstructure in Practice](https://www.amazon.com/Market-Microstructure-Practice-Jean-Philippe-Bouchaud/dp/9813231122)
- [Algorithmic Trading: Winning Strategies](https://www.amazon.com/Algorithmic-Trading-Winning-Strategies-Rationale/dp/1118460146)

### Daily Study Schedule
Each day should follow this pattern:
1. **Morning (2-3 hours)**: Read assigned materials for the day
2. **Afternoon (4-5 hours)**: Implement the project
3. **Evening (1 hour)**: Document learnings and prepare for next day

**Time Management**: Don't spend more than 3 hours on reading per day. The goal is practical competence, not theoretical mastery.

## Contributing

This is a learning repository. Each project should be:
- Completable in one focused day (6-8 hours)
- Well-documented with clear learning objectives
- Professionally structured with proper testing
- Incrementally building toward real-world skills

---

**Remember**: The goal isn't to build the most sophisticated system possible, but to develop the foundational competence that makes sophisticated systems achievable.