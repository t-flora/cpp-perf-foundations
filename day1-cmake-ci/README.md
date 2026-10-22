# Day 1: CMake + CI Hello World
**Goal**: Master the build system every C++ project uses

You'll build a small calculator library, a demo program, and tests, then get GitHub Actions to build and test it on every push. The code is deliberately simple: today is about the build and the pipeline. Readings are listed under Day 1 in the [main README](../README.md#day-1-cmake--ci-hello-world).

## Deliverables

```
day1-cmake-ci/
├── CMakeLists.txt        # Starter skeleton provided: fill in the TODOs
├── src/                  # calculator library + main.cpp demo
└── tests/                # test executable, registered with CTest

.github/workflows/day1-ci.yml   # at the repo root, not inside day1-cmake-ci/
```

## Milestones

### 1. Library and Demo (~2h)
- A `calculator` library with `add`, `subtract`, `multiply`, and `divide`. Header-only or `.h`/`.cpp` is your call; know the trade-off.
- A `main.cpp` demo that links against it. An interactive `5 + 3` prompt is a nice touch.
- Decide, and document: what does `divide(1, 0)` do? What type does `add(1, 2.5)` return?

Use targets throughout: `target_include_directories` and `target_link_libraries`, never global `include_directories`.

### 2. Tests (~1h)
- A test executable that covers positive, negative, and zero operands, integer vs. floating-point division, and float comparisons with a tolerance.
- Register it with `add_test`, so `ctest` runs it.
- Plain `assert` is fine, but find out what `NDEBUG` does to it in a Release build.

### 3. CI (~2h)
A GitHub Actions workflow that checks out the code, configures, builds, and runs `ctest`.
- Trigger only on changes under `day1-cmake-ci/` (`paths:` filter), and watch your branch filters.
- `.github/` must sit at the repo root; use `working-directory:` to reach the project.
- Get it green on *your* platform first. Multi-config generators (Visual Studio, Xcode) ignore `CMAKE_BUILD_TYPE`, which is one reason Windows tends to break first.

## Done When
- [ ] From a clean checkout: `cmake -B build && cmake --build build && (cd build && ctest)` passes
- [ ] CI goes green on push, and red when you break a test on purpose

## If Time Permits
- Extend the CI matrix to Linux, macOS, and Windows
- Run the workflow locally with [act](https://github.com/nektos/act)
- Add a `.clang-format` and check formatting in CI
