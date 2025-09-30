# Vector math header-only library
### Day 2: Header-Only Math Library
**Goal**: Understand how modern C++ libraries are distributed

**What You'll Build**: Vector/matrix operations library for 2D/3D vectors
- Single header file with templated vector operations
- Example usage demonstrating the API
- CMake export files for easy integration

*If time permits*:
- Documentation with usage examples
- A few unit tests with Google Test

**Skills Gained**:
- Template programming fundamentals
- Header-only library design patterns
- Package management and distribution
- API design principles

**Essential Reading** (2-3 hours before coding):
- [Effective Modern C++ - Item 30](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/) - Perfect forwarding for template performance
- [Expression Templates Explained (ModernesCpp)](https://www.modernescpp.com/index.php/expression-templates) - Eliminating temporary objects in mathematical operations
- [C++ Reference: constexpr specifier](https://en.cppreference.com/w/cpp/language/constexpr) - Compile-time computation fundamentals
- [Google Highway SIMD Library](https://github.com/google/highway) - Cross-platform vectorization basics
- [API Design Principles](https://wiki.qt.io/API_Design_Principles) - Creating clean interfaces

*Additional resources*:
- [Modern C++ Header-Only Libraries](https://github.com/p-ranav/awesome-hpp) - Examples and best practices
- [Google Test Primer](http://google.github.io/googletest/primer.html) - Testing fundamentals

**Tools to for quick performance scans**:
- [Compiler Explorer](https://godbolt.org/) - See assembly output from your optimizations
- [Quick Bench](https://quick-bench.com/) - Online benchmarking for code snippets
- [C++ Insights](https://cppinsights.io/) - Understand template instantiation

**Why it matters**: Most high-performance C++ libraries are header-only for optimal compilation and inlining. Understanding this pattern is essential for systems programming.