#pragma once
// Minimal test helper. Not assert(): this project builds in Release, where NDEBUG would
// compile asserts away.
#include <cstdlib>
#include <iostream>
#include <string>

namespace test {

inline int failures = 0;

inline void check(bool ok, const std::string& what) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL " << what << "\n";
    }
}

inline int finish(const char* suite) {
    if (failures != 0) {
        std::cerr << suite << ": " << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << suite << ": all checks passed\n";
    return EXIT_SUCCESS;
}

} // namespace test
