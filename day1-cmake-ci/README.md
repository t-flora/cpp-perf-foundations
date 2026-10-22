# Day 1: Calculator with CMake + CI
A header-only, template-based calculator with an interactive demo, `<cassert>` tests, and a GitHub Actions pipeline.

## Project Structure

```
day1-cmake-ci/
├── src/
│   ├── calculator.hpp      # Template calculator functions + parsing helpers
│   ├── main.cpp            # Interactive calculator
│   └── CMakeLists.txt      # INTERFACE library target + demo executable
├── tests/
│   ├── test_calculator.cpp # Runtime tests using <cassert>
│   └── CMakeLists.txt      # Test executable
├── CMakeLists.txt          # Top-level config, registers the test with CTest
└── build_and_test.sh       # Configure, build, and test in one step

../.github/workflows/day1-ci.yml   # CI lives in the repo root's .github/
```

## Building

```bash
./build_and_test.sh   # configure (Debug), build, run tests
```

The script only saves you typing. You should know what it runs:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug   # always build out-of-source; CMake is case-sensitive here
cmake --build build
(cd build && ctest --verbose)             # runs every test registered with add_test()
./build/src/Calculator                    # interactive demo
```

| Target | Kind | Notes |
|---|---|---|
| `calculator` | `INTERFACE` library | Header-only; linking to it adds `src/` to the include path |
| `calculator_run` | executable | Output name is `Calculator` |
| `test_calculator` | executable | Registered with CTest as `calculator_tests` |

Build a single target with `cmake --build build --target calculator_run`.

```
Calculator> 5 + 3
8
Calculator> 7 / 2
3 (integer division)
Calculator> 7.0 / 2
3.5
Calculator> q
```

## Design Notes
- **`decltype` return types**: `add(int, double)` returns `double`, following the usual arithmetic conversions. Integer division truncates: `divide(7, 3) == 2`.
- **`<cassert>` tests**: no dependencies, and failures report file and line. Assertions compile away under `NDEBUG`, so the tests only check anything in Debug builds.
- **Floating point**: compare with `approx_equal(a, b, epsilon)`, never `==`.

The tests cover the four operations on positive, negative, and zero values; mixed-type results; the `isInteger()` and `parseExpression()` helpers; and integer vs. floating-point division.

## CI
`.github/workflows/day1-ci.yml` runs on pushes and PRs that touch `day1-cmake-ci/`. It runs on Ubuntu, macOS, and Windows: checkout → install CMake → configure (Debug) → build → `ctest`.

Windows uses the Ninja generator because the default Visual Studio generator is multi-config and ignores `CMAKE_BUILD_TYPE`. Most CI pain lives in cross-platform details like this one, so **get CI green on your own platform first** and add the others as a stretch goal. Also check your branch filters: by default, workflows run on pushes to every branch.

**Follow-up**: How would you run this workflow locally, without pushing?

## Troubleshooting
- **Tests "pass" but check nothing**: you built in Release, where `NDEBUG` disables `assert`.
- **Build errors**: you need CMake 3.16+ and a C++17 compiler.

---

**Next Steps**: Add modulo and power operations, with tests.
