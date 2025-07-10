# Calculator Project with Simple Testing
A template-based calculator library demonstrating modern C++ build practices with simple runtime testing using `<cassert>`.

## Features

- **Type-flexible calculator** using `decltype` (C++11) for automatic type deduction
- **Header-only library** with template functions for add, subtract, multiply, divide
- **Interactive calculator** with input parsing
- **Simple runtime tests** using `<cassert>` for verification
- **CMake build system** with integrated testing support
- **Github Actions CI** for testing builds upon pushing code/opening PRs

## Project Structure

```
day1-cmake-ci/
├── src/
├── src/
│   ├── calculator.hpp      # Template calculator functions + helpers
│   └── main.cpp            # Interactive calculator application
│   └── CMakeLists.txt      # Build config for header-only interface library target and executable target for interactive calculator
├── tests/
│   └── test_calculator.cpp # Runtime tests using <cassert>
│   └── CMakeLists.txt      # Build configuration for testing as a separate executable target in Debug builds
├── CMakeLists.txt         # Build configuration with testing
├── build_and_test.sh      # Build and test script
└── README.md             # This file

../.github/workflows/day1-ci.yml # Github Actions YAML file for building this project - .github folder in root directory
```

## Building the Project
### The easy way
The script in `build_and_test.sh` configures and runs the build process and runs tests.
You should be acquainted with how CMake works! This script is just to save you a few commands.

```bash
# Make script executable (first time only)
chmod +x build_and_test.sh

# Build and test everything
./build_and_test.sh
```

### Manual Build

```bash
# Create build directory
mkdir -p day1-build && cd day1-build # always go for out-of-source builds - it'll save you pain

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug # notice that CMake is case-sensitive

# Build
cmake --build .

# Run tests
./tests/test_calculator
```

## Configuring Github Actions CI
The GitHub Actions workflow for this project is defined in `../.github/workflows/day1-ci.yml`.  
It provides automated CI (Continuous Integration) for every push or pull request that affects the `day1-cmake-ci` directory.

**Key features:**
- **Triggers:**  
  - Runs on any push to `main` or `template` branches, or on pull requests, that modify files in `day1-cmake-ci/`.
- **Platforms:**  
  - The workflow tests your code on three operating systems: **Ubuntu**, **macOS**, and **Windows** (using a matrix strategy).
- **Steps:**  
  1. **Checkout code** using `actions/checkout@v4`.
  2. **Install CMake** on each platform (using `apt`, `brew`, or `choco` as appropriate).
  3. **Configure the project** with CMake in Debug mode.
  4. **Build the project** using CMake.
  5. **Run tests** using CTest with verbose output.

**What this means for you:**  
Every time you push changes or open a pull request, your code will be automatically built and tested on all three major platforms.  
If any step fails (e.g., build error, test failure, or missing dependency), the workflow will mark the CI run as failed, so you can catch issues before merging. There are a _lot_ of ways these steps can go wrong, especially if you're trying to do cross-platform development. **Consider setting up CI for the platform you are using, and adding other platforms as a stretch goal.**

**File location:**  
`cpp-perf-foundations/.github/workflows/day1-ci.yml`

**Follow-up:**  
If you wanted to simulate the CI run locally, what should you do?

## Extra: Running simple tests

### Direct Execution
```bash
cd day1-build
./tests/test_calculator
```

### Via CTest
```bash
cd day1-build
ctest --verbose # use CTest for running all your REGISTERED tests!
```

## Test Coverage

The simple test suite covers:
### Basic Operations
- Addition, subtraction, multiplication, division
- Positive, negative, and zero values
- Integer and floating-point arithmetic

### Type System (`decltype` functionality)
- **int + int** → `int`
- **int + double** → `double`  
- **float + int** → `float`
- **double + int** → `double`

### Helper Functions
- `isInteger()` string validation
- `parseExpression()` input parsing
- Whitespace handling and negative number support

### Edge Cases
- Integer vs. floating-point division behavior
- Floating-point precision issues

## Calculator Usage

### Interactive Mode
```bash
cd day1-build
./src/calculator

Calculator> 5 + 3
8
Calculator> 2.5 * 4.0
10
Calculator> q  # Exit
```

### Supported Operations
- **Addition**: `5 + 3`, `2.5 + 1.5`
- **Subtraction**: `10 - 7`, `-5 + 3`
- **Multiplication**: `4 * 6`, `2.5 * 2.0`
- **Division**: `8 / 2`, `7.0 / 3` (preserves precision)

## Testing Philosophy

This project uses **simple runtime assertions** with `<cassert>`:

### Why `<cassert>`?
- ✅ **Lightweight** - No external dependencies
- ✅ **Built-in** - Part of standard C++ library
- ✅ **Clear** - Fails fast with clear error location
- ✅ **Debug-friendly** - Can be disabled with `NDEBUG`

### Test Structure
```cpp
void test_basic_operations() {
    assert(calculator::add(2, 3) == 5);
    assert(approx_equal(calculator::add(2.5, 3.7), 6.2));
    std::cout << "✅ Basic operations passed!\n";
}
```

### Floating-Point Testing
```cpp
bool approx_equal(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;
}
```

## CMake Features

### Simple Configuration
- **No external dependencies** - Just standard C++
- **CTest integration** - Built-in test runner
- **Debug builds** - Compiler warnings and debug symbols

### Available Targets
```bash
# Build everything
cmake --build .

# Build only main app
cmake --build . --target calculator

# Build only tests
cmake --build . --target test_calculator

# Run tests via CMake
cmake --build . --target run_tests
```

## Troubleshooting

### Build Issues
- Ensure CMake 3.16+ is installed
- Check C++17 compiler support

### Test Failures
- Look for assertion failures with file/line numbers
- Check floating-point comparisons use `approx_equal()`
- Ensure debug build (assertions disabled in release with `NDEBUG`)

---

**Next Steps**: Try adding new operations (modulo, power) and corresponding tests!

