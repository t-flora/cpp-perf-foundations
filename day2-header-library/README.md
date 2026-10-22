# Day 2: Header-Only Math Library
**Goal**: Understand how modern C++ libraries are distributed

You'll write `Vec3<T>`, a templated 3D vector, as a single header, then package it the way real libraries are packaged: another project should be able to `find_package(vec3)` it. Readings and tools are listed under Day 2 in the [main README](../README.md#day-2-header-only-math-library).

## Deliverables

```
day2-header-library/
├── CMakeLists.txt             # Starter skeleton provided
├── include/vec3/vec3.hpp      # Starter skeleton provided
├── cmake/vec3Config.cmake.in  # Package config template
├── tests/                     # Runtime + compile-time tests
├── examples/consumer/         # A separate project using the *installed* library
└── benchmarks/                # A naive std::chrono timing loop
```

## Milestones

### 1. The Header (~2.5h)
- Arithmetic (`+`, `-`, unary `-`, `* scalar`, `/ scalar`, and the compound versions), `==`, `dot`, `cross`, `length`, `normalized`
- `constexpr` wherever the language allows it. Which of these *can't* be `constexpr` in C++20, and why?
- Make `2 * v` compile, as well as `v * 2`, including `2 * Vec3<double>{}`.
- No `using namespace std;` in a header. Think about why.

### 2. Tests (~1.5h)
- Runtime tests for every operation, including what happens when you divide by zero or normalize a zero vector
- `static_assert`s that prove operations evaluate at compile time. If the file compiles, the tests passed.
- Register both with CTest.

### 3. Packaging (~1.5h)
- An `INTERFACE` target with `$<BUILD_INTERFACE:...>` / `$<INSTALL_INTERFACE:...>` include paths, and a `vec3::vec3` alias
- `install(TARGETS ... EXPORT ...)`, `install(EXPORT ...)`, plus a config and a version file (`CMakePackageConfigHelpers`)
- `examples/consumer/`: its own `CMakeLists.txt` that calls `find_package(vec3 REQUIRED)` and links `vec3::vec3`. Build it against `cmake --install build --prefix <somewhere>`.
- Stretch: make CTest do the install-and-build-consumer round trip, so broken packaging fails a test.

### 4. A First Benchmark (~0.5h)
Time each operation with a `std::chrono` loop over a million iterations, in a Release build. Write down the numbers, and anything that makes you doubt them. Day 3 revisits this loop.

## Checks Before Moving On
- Does the build pass with `-Wall -Wextra` and no warnings? A non-`void` function that falls off the end without a `return` is undefined behavior, and the compiler will tell you. On macOS, note that `$<CXX_COMPILER_ID:Clang>` does *not* match Apple's compiler (`AppleClang`).
- Can a consumer project that is *not* in this directory build against your installed library?
- Is `Vec3<float>` trivially copyable? (`static_assert(std::is_trivially_copyable_v<...>)`)

## If Time Permits
- Expression templates for chains like `a + b + c`; compare the assembly on Compiler Explorer
- Switch the tests to Google Test
