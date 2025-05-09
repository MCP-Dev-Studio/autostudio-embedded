#!/bin/bash
# Build script for the simplified MCP configuration test

set -e  # Exit on error

# Print command being executed
set -x

# Create build directory
mkdir -p build

# Compile the test with our test implementation
gcc -o build/test_mcp_config \
   -DMCP_PLATFORM_RPI \
   -I. \
   tests/test_mcp_config_simple.c \
   tests/mcp_config_test.c \
   src/core/mcp/config/platform_config.c \
   src/json/json_helpers.c

# Run the test
./build/test_mcp_config