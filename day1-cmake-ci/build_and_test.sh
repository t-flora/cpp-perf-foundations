#!/bin/bash

set -e  # Exit on any error

echo "🔨 Building Calculator Project..."
echo "================================="

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "📋 Configuring project..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build the project
echo "🏗️  Building project..."
cmake --build .

echo ""
echo "✅ Build completed successfully!"

# Run tests
echo ""
echo "🧪 Running tests..."
echo "=================="
./tests/test_calculator

echo ""
echo "🎉 Done! You can also run:"
echo "   📱 Calculator app: ./src/Calculator"
echo "   🧪 Tests directly: ./tests/test_calculator"
echo "   🏃 Tests via CTest: ctest --verbose" 