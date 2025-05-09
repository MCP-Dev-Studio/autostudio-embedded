#!/bin/bash
# Build script for simple configuration test

set -e  # Exit on error

# Print command being executed
set -x

# Create build directory
mkdir -p build

# Compile the test
gcc -o build/test_config_simple \
   -DMCP_PLATFORM_RPI \
   -I. \
   tests/test_config_simple.c \
   src/core/mcp/config/mcp_config.c \
   src/core/mcp/config/platform_config.c \
   src/json/json_helpers.c

# Run the test
./build/test_config_simple