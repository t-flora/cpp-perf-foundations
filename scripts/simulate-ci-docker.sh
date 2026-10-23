#!/bin/bash
cd "$(dirname "$0")/.." || exit 1 # run from the repo root, wherever this is called from

echo "🐳 Simulating GitHub Actions CI with Docker..."
echo "============================================="

PROJECT="day1-cmake-ci"
PLATFORM=${1:-"ubuntu"}

case $PLATFORM in
    "ubuntu")
        IMAGE="ubuntu:22.04"
        INSTALL_CMD="apt-get update && apt-get install -y cmake build-essential"
        ;;
    "ubuntu20")
        IMAGE="ubuntu:20.04" 
        INSTALL_CMD="apt-get update && apt-get install -y cmake build-essential"
        ;;
    *)
        echo "Usage: $0 [ubuntu|ubuntu20]"
        echo "Simulates CI on different platforms"
        exit 1
        ;;
esac

echo "🎯 Testing on: $PLATFORM ($IMAGE)"
echo

# Check if Docker is available and running
echo "🔍 Checking Docker availability..."
if ! command -v docker >/dev/null 2>&1; then
    echo "❌ Docker is not installed!"
    echo "   Install Docker Desktop from: https://www.docker.com/products/docker-desktop"
    exit 1
fi

if ! docker info >/dev/null 2>&1; then
    echo "❌ Docker daemon is not running!"
    echo "   Start Docker Desktop and try again"
    exit 1
fi

echo "✅ Docker is available and running"
echo

# Create a script that will run inside the container
cat > ci-inside-container.sh << 'EOF'
#!/bin/bash
set -e

echo "=== Inside CI Container ==="
echo "Platform: $(cat /etc/os-release | grep PRETTY_NAME)"
echo "User: $(whoami)"
echo "Working directory: $(pwd)"
echo

# Install CMake and build tools
echo "📦 Installing dependencies..."
DEBIAN_FRONTEND=noninteractive apt-get update -qq
DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential

# Install CMake - try package first, then install newer version if needed
apt-get install -y cmake
CMAKE_VERSION=$(cmake --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
echo "📋 Installed CMake: $CMAKE_VERSION"

# Check if we need a newer CMake (basic check for common requirements)
if [ -f "WORKDIR/day1-cmake-ci/CMakeLists.txt" ]; then
    REQUIRED_VERSION=$(grep "cmake_minimum_required" WORKDIR/day1-cmake-ci/CMakeLists.txt | grep -o '[0-9]\+\.[0-9]\+' || echo "3.16")
    echo "📋 Project requires CMake: $REQUIRED_VERSION"
    
    # If system CMake is too old, install from Kitware's repository
    if dpkg --compare-versions "$CMAKE_VERSION" "lt" "$REQUIRED_VERSION"; then
        echo "⬆️  Installing newer CMake from Kitware..."
        apt-get install -y wget gpg
        wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
        echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null
        apt-get update -qq
        apt-get install -y cmake
    fi
fi

echo "✅ Dependencies installed"
echo "  CMake: $(cmake --version | head -n1)"
echo "  GCC: $(gcc --version | head -n1)"
echo

# Configure project
echo "🔧 Configuring project..."
cd WORKDIR/day1-cmake-ci

# Clean any existing build directory to avoid cache conflicts
if [ -d "build" ]; then
    echo "🧹 Cleaning previous build cache..."
    rm -rf build
fi

cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build project
echo "🔨 Building project..."
cmake --build build

# Run tests
echo "🧪 Running tests..."
cd build
ctest --verbose

echo
echo "🎉 Docker CI simulation completed successfully!"
EOF

chmod +x ci-inside-container.sh

# Run the simulation in Docker
echo "🚀 Launching Docker container..."
if docker run --rm \
    --platform linux/amd64 \
    -v "$(pwd):/workspace" \
    -w /workspace \
    $IMAGE \
    bash -c "
        # Copy our script and make it executable
        cp ci-inside-container.sh /tmp/
        sed -i 's|WORKDIR|/workspace|g' /tmp/ci-inside-container.sh
        chmod +x /tmp/ci-inside-container.sh
        /tmp/ci-inside-container.sh
    "; then
    DOCKER_EXIT_CODE=0
    STATUS="✅ PASSED"
else
    DOCKER_EXIT_CODE=$?
    STATUS="❌ FAILED"
fi

# Cleanup
rm -f ci-inside-container.sh

echo
echo "✨ Summary:"
echo "  Platform: $PLATFORM"
echo "  Image: $IMAGE"
echo "  Status: $STATUS"
echo
echo "💡 Try different platforms:"
echo "  scripts/simulate-ci-docker.sh ubuntu    # Ubuntu 22.04 (same as CI)"
echo "  scripts/simulate-ci-docker.sh ubuntu20  # Ubuntu 20.04"

# Exit with the same code as the Docker command
exit $DOCKER_EXIT_CODE 