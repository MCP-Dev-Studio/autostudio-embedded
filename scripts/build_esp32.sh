#!/bin/bash
# Build script for ESP32 platform (ESP-IDF)

# Exit on error
set -e

usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --board=NAME   - Specify the ESP32 board to use (defaults to ESP32-DevKitC)"
    echo "  --port=PATH    - Specify the port for upload (e.g. --port=/dev/ttyUSB0)"
    echo "  --help         - Show this help message"
    exit 1
}

# Source directory
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SRC_DIR}/build/esp32"

# Default board and port
BOARD="ESP32-DevKitC"
PORT=""

# Parse arguments
for arg in "$@"
do
    case $arg in
        --board=*)
        BOARD="${arg#*=}"
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

echo "Using ESP32 board: $BOARD"
if [ -n "$PORT" ]; then
    echo "Using port: $PORT"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Check if ESP-IDF is properly set up
if [ -z "$IDF_PATH" ]; then
    echo "Warning: IDF_PATH environment variable is not set."
    echo "Please set up ESP-IDF environment by running:"
    echo "  . \$HOME/esp/esp-idf/export.sh"
    echo "or similar command based on your ESP-IDF installation."
    echo ""
    echo "Continuing with build but it may fail if ESP-IDF is not properly set up."
fi

# Configure with CMake
echo "Configuring ESP32 build with CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DMCP_PLATFORM=ESP32 \
      -DESP32_BOARD="$BOARD" \
      ${PORT:+-DPORT="$PORT"} \
      "$SRC_DIR"

# Build
echo "Building for ESP32 platform..."
cmake --build . -j $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

# Flash if port is specified and idf.py is available
if [ -n "$PORT" ] && command -v idf.py &> /dev/null; then
    echo "Flashing to ESP32 on port $PORT..."
    idf.py -p "$PORT" flash
fi

echo "ESP32 build completed successfully"