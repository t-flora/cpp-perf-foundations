# Day 2: Header-Only Vec3 Library
A header-only, `constexpr` 3D vector type with CMake install/export rules, so other projects can `find_package(vec3)` it.

## Project Structure

```
day2-header-library/
├── include/vec3/vec3.hpp        # The whole library
├── cmake/vec3Config.cmake.in    # Package config template for find_package
├── tests/
│   ├── test_vec3_basics.cpp     # Runtime tests
│   ├── test_vec3_constexpr.cpp  # static_assert tests: compiling is passing
│   └── check_find_package.cmake # Installs vec3 and builds examples/consumer against it
├── examples/consumer/           # Standalone project using the *installed* library
├── benchmarks/
│   ├── simple_benchmark.cpp     # Naive std::chrono loop (Day 2)
│   └── bench_vec3.cpp           # Google Benchmark port (Day 3 warm-up)
└── CMakeLists.txt
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
(cd build && ctest --output-on-failure)
```

Builds default to Release when no build type is given, because that's what benchmarks need. The tests `#undef NDEBUG`, so their asserts still run in Release.

**Using it from another project**:
```bash
cmake --install build --prefix ~/.local
# in the other project's CMakeLists.txt:
#   find_package(vec3 0.1 REQUIRED)
#   target_link_libraries(app PRIVATE vec3::vec3)
cmake -S my-app -B my-app/build -DCMAKE_PREFIX_PATH=~/.local
```
The `vec3_find_package` test runs exactly this against `examples/consumer`, so broken packaging fails CTest.

**Benchmarks**:
```bash
./build/benchmarks/simple_benchmark                              # naive loop
cmake -B build -DVEC3_GOOGLE_BENCHMARK=ON && cmake --build build # downloads Google Benchmark
./build/benchmarks/bench_vec3
```

## Design Notes
- **Rule of zero**: no user-written copy assignment or destructor. Vec3 stays trivially copyable and usable in constant expressions; `test_vec3_constexpr.cpp` checks both.
- **Reference accessors** (`v.x() = 3`) replace setters. A `const` overload keeps const-correctness.
- **`length()` is not `constexpr`**: `std::sqrt` only becomes `constexpr` in C++26. `length_squared()` is, and it's often all you need, e.g. for comparing distances.
- **`2 * v`**: the free `operator*` takes `std::type_identity_t<T>` for the scalar, so `T` is deduced from the vector alone and `2 * Vec3<double>{}` compiles.
- **Division by zero throws**: this checks every division, which hot-path libraries like GLM skip. A `throw` is also not a constant expression, so `a / 0` fails at compile time when used in a `static_assert`.
- **`namespace vec3`**: headers never `using namespace std;`. Doing so would leak into every file that includes them.
- **AppleClang**: `$<CXX_COMPILER_ID:Clang>` doesn't match Apple's compiler, so the warning flags list `AppleClang` explicitly.

## What the Naive Benchmark Gets Wrong
Representative numbers (Apple M2, Release):

| Operation | `simple_benchmark` | `bench_vec3` |
|---|---|---|
| add | 3.37 ns | 0.77 ns |
| dot | 0.81 ns | 1.02 ns |
| length | 0.58 ns | 1.03 ns |

`v1` and `v2` never change inside the loop, so the compiler computes the result once and the loop only repeats the `volatile` store. For `Vec3` that's three stores, which is why "add" looks four times slower than it is. `bench_vec3` calls `benchmark::DoNotOptimize` on the inputs every iteration, so the work can't be hoisted out of the loop. Day 3 builds on this.
