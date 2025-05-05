#\!/bin/bash
# MCP Embedded System Build Script

set -e  # Exit on error

# Create build directory
mkdir -p build
cd build

# Configure the build
echo "Configuring build..."
cmake ..

# Build the project
echo "Building..."
cmake --build .

echo "Build completed successfully\!"
echo
echo "Executables are available in the build directory:"
echo "  - mcp_embedded: Main application"
echo "  - test_device_info: Device information test"
echo "  - test_dynamic_tools: Dynamic tool registration test"
echo "  - self_contained_test: Self-contained test"
echo
echo "To run tests use: ctest"
