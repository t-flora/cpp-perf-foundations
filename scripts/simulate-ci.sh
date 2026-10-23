#!/bin/bash
set -e
cd "$(dirname "$0")/.." # run from the repo root, wherever this is called from

echo "🤖 Simulating GitHub Actions CI locally..."
echo "========================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Simulate the exact CI steps
echo -e "${BLUE}Step 1: Checkout code${NC}"
echo "✅ Code already available locally (simulating actions/checkout@v4)"
echo

echo -e "${BLUE}Step 2: Install CMake${NC}"
if command -v cmake >/dev/null 2>&1; then
    echo "✅ CMake already installed: $(cmake --version | head -n1)"
else
    echo -e "${YELLOW}⚠️  CMake not found. Install with:${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "  brew install cmake"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "  sudo apt-get install cmake"
    fi
    exit 1
fi
echo

echo -e "${BLUE}Step 3: Configure project${NC}"
cd day1-cmake-ci
if [ -d "build" ]; then
    echo "🧹 Cleaning previous build (avoids Docker/local path conflicts)..."
    rm -rf build
fi

echo "🔧 Running: cmake -B build -DCMAKE_BUILD_TYPE=Debug"
cmake -B build -DCMAKE_BUILD_TYPE=Debug
echo "✅ Configuration complete"
echo

echo -e "${BLUE}Step 4: Build project${NC}"
echo "🔨 Running: cmake --build build"
cmake --build build
echo "✅ Build complete"
echo

echo -e "${BLUE}Step 5: Run tests${NC}"
cd build
echo "🧪 Running: ctest --verbose"
ctest --verbose
echo

if [ $? -eq 0 ]; then
    echo -e "${GREEN}🎉 LOCAL CI SIMULATION PASSED!${NC}"
    echo -e "${GREEN}Your code should pass GitHub Actions CI${NC}"
else
    echo -e "${RED}❌ LOCAL CI SIMULATION FAILED!${NC}"
    echo -e "${RED}Fix these issues before pushing to GitHub${NC}"
    exit 1
fi

echo
echo "📊 Summary:"
echo "  - Platform: $(uname -s) (vs CI: Ubuntu + macOS + Windows)"
echo "  - Compiler: $(c++ --version 2>/dev/null | head -n1 || echo 'Unknown')"
echo "  - CMake: $(cmake --version | head -n1)"
echo
echo "💡 To test on different platforms, consider using Docker or act" 