#!/bin/bash
# Build script for HOST platform

# Exit on error
set -e

usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --debug         - Build in debug mode (default)"
    echo "  --release       - Build in release mode"
    echo "  --run           - Run the executable after building"
    echo "  --run-with-args=\"ARGS\" - Run with specified arguments"
    echo "  --help          - Show this help message"
    exit 1
}

# Source directory
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SRC_DIR}/build/host"

# Default settings
BUILD_TYPE="Debug"
RUN_AFTER_BUILD=false
RUN_ARGS=""

# Parse arguments
for arg in "$@"
do
    case $arg in
        --debug)
        BUILD_TYPE="Debug"
        ;;
        --release)
        BUILD_TYPE="Release"
        ;;
        --run)
        RUN_AFTER_BUILD=true
        ;;
        --run-with-args=*)
        RUN_AFTER_BUILD=true
        RUN_ARGS="${arg#*=}"
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

echo "Building for HOST platform in $BUILD_TYPE mode..."

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring HOST build with CMake..."
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DMCP_PLATFORM=HOST \
      "$SRC_DIR"

# Build
echo "Building for HOST platform..."
cmake --build . -j $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

# Run if requested
if [ "$RUN_AFTER_BUILD" = true ]; then
    echo "Running executable..."
    
    # Find the executable
    if [ -f "$BUILD_DIR/mcp_embedded" ]; then
        EXECUTABLE="$BUILD_DIR/mcp_embedded"
    elif [ -f "$BUILD_DIR/bin/mcp_embedded" ]; then
        EXECUTABLE="$BUILD_DIR/bin/mcp_embedded"
    else
        echo "Error: Executable not found"
        exit 1
    fi
    
    # Run the executable with arguments if provided
    if [ -z "$RUN_ARGS" ]; then
        "$EXECUTABLE"
    else
        "$EXECUTABLE" $RUN_ARGS
    fi
fi

echo "HOST build completed successfully"