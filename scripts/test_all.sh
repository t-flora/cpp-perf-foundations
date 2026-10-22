#!/usr/bin/env bash
# Configure, build, and test every day's project, each in build/<day>/.
# Usage: scripts/test_all.sh [day-dir ...]   (default: every dayN-* directory with a CMakeLists.txt)
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

if [ $# -gt 0 ]; then
    days=("$@")
else
    days=()
    for dir in day*/; do
        [ -f "$dir/CMakeLists.txt" ] && days+=("${dir%/}")
    done
fi

failed=()
for day in "${days[@]}"; do
    echo "==> $day"
    # Day 1's tests use plain assert(), which Release builds compile away
    build_type=Release
    [ "$day" = "day1-cmake-ci" ] && build_type=Debug

    if cmake -S "$day" -B "build/$day" -DCMAKE_BUILD_TYPE="$build_type" > /dev/null &&
        cmake --build "build/$day" --parallel &&
        (cd "build/$day" && ctest --output-on-failure); then
        echo "==> $day: OK"
    else
        echo "==> $day: FAILED"
        failed+=("$day")
    fi
done

if [ ${#failed[@]} -gt 0 ]; then
    echo "Failed: ${failed[*]}"
    exit 1
fi
echo "All ${#days[@]} projects passed"
