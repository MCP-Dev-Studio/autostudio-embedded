#!/bin/bash
# Main build script for the MCP embedded framework

# Exit on error
set -e

usage() {
    echo "Usage: $0 [platform] [options]"
    echo "Available platforms:"
    echo "  host     - Build for HOST platform"
    echo "  esp32    - Build for ESP32 platform (ESP-IDF)"
    echo "  rpi      - Build for Raspberry Pi platform"
    echo "  arduino  - Build for Arduino platform"
    echo "  mbed     - Build for MBED platform"
    echo "  all      - Build for all platforms"
    echo "  help     - Show this help message"
    echo ""
    echo "Platform-specific options (run with just platform to see options):"
    echo "  Host platform options:"
    echo "    --debug, --release, --run, --run-with-args=ARGS"
    echo ""
    echo "  ESP32 platform options:"
    echo "    --board=NAME, --port=PATH"
    echo ""
    echo "  Raspberry Pi platform options:"
    echo "    --cross-compile, --host=ADDRESS"
    echo ""
    echo "  Arduino platform options:"
    echo "    --board=NAME, --port=PATH, --cpp-fixes"
    echo ""
    echo "  MBED platform options:"
    echo "    --target=NAME, --port=PATH"
    echo ""
    echo "If no platform is specified, HOST platform is used by default."
    exit 1
}

# Root directory
SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPTS_DIR/.." && pwd)"

# Default platform
PLATFORM="host"

# Parse platform
if [ $# -gt 0 ]; then
    PLATFORM="$1"
    shift # Remove the platform argument
fi

# Check for help
if [ "$PLATFORM" = "help" ]; then
    usage
fi

# Check if the platform-specific script exists
PLATFORM_SCRIPT="$SCRIPTS_DIR/build_${PLATFORM}.sh"
if [ ! -f "$PLATFORM_SCRIPT" ] && [ "$PLATFORM" != "all" ]; then
    echo "Error: Unknown platform '$PLATFORM'"
    usage
fi

# Execute the selected platform build
case "$PLATFORM" in
    "host")
        # Call HOST platform script with all remaining arguments
        "$PLATFORM_SCRIPT" "$@"
        ;;
    "esp32")
        # Call ESP32 platform script with all remaining arguments
        "$PLATFORM_SCRIPT" "$@"
        ;;
    "rpi")
        # Call Raspberry Pi platform script with all remaining arguments
        "$PLATFORM_SCRIPT" "$@"
        ;;
    "arduino")
        # Use the simplified Arduino build script
        "$PLATFORM_SCRIPT" "$@"
        ;;
    "mbed")
        # Call MBED platform script with all remaining arguments
        "$PLATFORM_SCRIPT" "$@"
        ;;
    "all")
        echo "Building for all platforms..."
        
        # Build HOST platform
        echo "-----------------------------------------"
        echo "Building HOST platform..."
        echo "-----------------------------------------"
        "$SCRIPTS_DIR/build_host.sh" --debug
        
        # Build ESP32 platform
        echo "-----------------------------------------"
        echo "Building ESP32 platform..."
        echo "-----------------------------------------"
        "$SCRIPTS_DIR/build_esp32.sh"
        
        # Build Raspberry Pi platform
        echo "-----------------------------------------"
        echo "Building Raspberry Pi platform..."
        echo "-----------------------------------------"
        "$SCRIPTS_DIR/build_rpi.sh"
        
        # Build Arduino platform with C++ fixes
        echo "-----------------------------------------"
        echo "Building Arduino platform with C++ fixes..."
        echo "-----------------------------------------"
        "$SCRIPTS_DIR/build_arduino.sh" --cpp-fixes
        
        # Build MBED platform
        echo "-----------------------------------------"
        echo "Building MBED platform..."
        echo "-----------------------------------------"
        "$SCRIPTS_DIR/build_mbed.sh"
        
        echo "All platforms built successfully!"
        ;;
    *)
        echo "Error: Unknown platform '$PLATFORM'"
        usage
        ;;
esac

exit 0