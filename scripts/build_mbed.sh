#!/bin/bash
# Build script for MBED platform

# Exit on error
set -e

usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --target=NAME  - Specify the MBED target (e.g. NUCLEO_F446RE, DISCO_L475VG)"
    echo "  --port=PATH    - Specify the port for upload (e.g. --port=/dev/ttyACM0)"
    echo "  --help         - Show this help message"
    exit 1
}

# Source directory
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SRC_DIR}/build/mbed"

# Default target and port
TARGET="NUCLEO_F446RE"
PORT=""

# Parse arguments
for arg in "$@"
do
    case $arg in
        --target=*)
        TARGET="${arg#*=}"
        ;;
        --port=*)
        PORT="${arg#*=}"
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

echo "Using MBED target: $TARGET"
if [ -n "$PORT" ]; then
    echo "Using port: $PORT"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Check if mbed-cli is installed
if ! command -v mbed &> /dev/null; then
    echo "Error: mbed-cli is not installed or not in PATH."
    echo "Please install mbed-cli first with:"
    echo "  pip install mbed-cli"
    exit 1
fi

# Configure with mbed-cli
echo "Configuring MBED build..."

# If project is not initialized yet, initialize it
if [ ! -f ".mbed" ]; then
    echo "Initializing MBED project..."
    mbed new .
    mbed config -G GCC_ARM_PATH "$(which arm-none-eabi-gcc | xargs dirname)"
    mbed target "$TARGET"
    mbed toolchain GCC_ARM
else
    # Just update the target
    mbed target "$TARGET"
fi

# Configure with CMake
echo "Configuring MBED build with CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DMCP_PLATFORM=MBED \
      -DMBED_TARGET="$TARGET" \
      "$SRC_DIR"

# Build
echo "Building for MBED platform..."
cmake --build . -j $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

# Compile with mbed-cli
echo "Compiling with mbed-cli..."
mbed compile

# Flash if port is specified
if [ -n "$PORT" ]; then
    echo "Flashing to MBED device on port $PORT..."
    cp BUILD/"$TARGET"/GCC_ARM/*.bin "$PORT"
fi

echo "MBED build completed successfully"