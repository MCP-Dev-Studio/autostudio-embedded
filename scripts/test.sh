#\!/bin/bash
# MCP Embedded System Test Script

set -e  # Exit on error

# Make sure we're in the right directory
if [ \! -d "build" ]; then
  echo "Build directory not found. Please run build.sh first."
  exit 1
fi

cd build

# Run all tests
echo "Running tests..."
ctest --output-on-failure

echo "Tests completed\!"
