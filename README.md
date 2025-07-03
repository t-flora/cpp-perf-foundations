# C++ Foundations: 5-Day Project Series

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
└── day5-realtime-processor/
    ├── README.md
    ├── CMakeLists.txt
    ├── src/
    │   └── market_data_processor.cpp
    ├── data/
    │   └── sample_trades.csv
    └── tests/
        └── test_vwap.cpp
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

**Why It Matters**: Most high-performance C++ libraries are header-only for optimal compilation and inlining. Understanding this pattern is essential for financial systems.

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

**Why It Matters**: All modern trading systems run in containers, and performance measurement is critical for optimization decisions.

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

**Why It Matters**: Trading systems live or die on container choice. A wrong data structure can cost microseconds per operation, which translates to lost profits.

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

**Why It Matters**: This is fundamentally what trading systems do - process streaming market data and calculate analytics in real-time.

## Getting Started

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

## Success Criteria

After completing all 5 projects, you should be able to:
- Set up a new C++ project with CMake and CI from scratch
- Design and implement header-only libraries
- Write and interpret performance benchmarks
- Choose appropriate data structures for different use cases
- Process streaming data efficiently
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

## Contributing

This is a learning repository. Each project should be:
- Completable in one focused day (6-8 hours)
- Well-documented with clear learning objectives
- Professionally structured with proper testing
- Incrementally building toward real-world skills

---

**Remember**: The goal isn't to build the most sophisticated system possible, but to develop the foundational competence that makes sophisticated systems achievable.