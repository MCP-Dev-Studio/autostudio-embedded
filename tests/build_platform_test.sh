#!/bin/bash
# Build script for platform configuration tests

set -e  # Exit on error

# Print command being executed
set -x

# Create build directory
mkdir -p build

# Compile the test
gcc -o build/test_platform_config \
   -DMCP_PLATFORM_RPI \
   -I. \
   tests/test_platform_config.c \
   tests/logging_stub.c \
   src/core/mcp/config/platform_config.c

# Run the test
./build/test_platform_config