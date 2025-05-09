#!/bin/bash
# Build script for Raspberry Pi platform

# Exit on error
set -e

usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --cross-compile   - Build using cross-compilation toolchain"
    echo "  --host=ADDRESS    - Specify Raspberry Pi SSH host for deploy (requires SSH access)"
    echo "  --help            - Show this help message"
    exit 1
}

# Source directory
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SRC_DIR}/build/rpi"

# Default settings
CROSS_COMPILE=false
RPI_HOST=""

# Parse arguments
for arg in "$@"
do
    case $arg in
        --cross-compile)
        CROSS_COMPILE=true
        ;;
        --host=*)
        RPI_HOST="${arg#*=}"
        ;;
        --help)
        usage
        ;;
        *)
        echo "Unknown option: $arg"
        usage
        ;;
    esac
done

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ "$CROSS_COMPILE" = true ]; then
    echo "Building for Raspberry Pi using cross-compilation..."
    
    # Check for cross-compiler
    if ! command -v arm-linux-gnueabihf-gcc &> /dev/null; then
        echo "Error: Cross-compiler not found."
        echo "Please install the Raspberry Pi cross-compilation toolchain:"
        echo "  For Ubuntu/Debian: sudo apt-get install -y crossbuild-essential-armhf"
        echo "  For macOS: brew install arm-linux-gnueabihf-binutils"
        exit 1
    fi
    
    # Configure with CMake for cross-compilation
    echo "Configuring Raspberry Pi build with CMake for cross-compilation..."
    
    # Create toolchain file if it doesn't exist
    TOOLCHAIN_FILE="${BUILD_DIR}/toolchain-rpi.cmake"
    if [ ! -f "$TOOLCHAIN_FILE" ]; then
        cat > "$TOOLCHAIN_FILE" << 'EOL'
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
EOL
    fi
    
    # Configure with CMake
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DMCP_PLATFORM=RPI \
          -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
          "$SRC_DIR"
else
    echo "Building for Raspberry Pi using native build..."
    
    # Configure with CMake for native build
    echo "Configuring Raspberry Pi build with CMake..."
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DMCP_PLATFORM=RPI \
          "$SRC_DIR"
fi

# Build
echo "Building for Raspberry Pi platform..."
cmake --build . -j $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

# Deploy to Raspberry Pi if host is specified
if [ -n "$RPI_HOST" ]; then
    echo "Deploying to Raspberry Pi at $RPI_HOST..."
    
    # Create deploy directory on Raspberry Pi
    ssh "$RPI_HOST" "mkdir -p ~/mcp_embedded"
    
    # Copy binary to Raspberry Pi
    scp "$BUILD_DIR/mcp_embedded" "$RPI_HOST:~/mcp_embedded/"
    
    echo "Deployment completed successfully."
fi

echo "Raspberry Pi build completed successfully"