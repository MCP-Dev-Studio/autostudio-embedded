#!/bin/bash
# Simple build script for configuration tests

set -e  # Exit on error

# Print command being executed
set -x

# Create build directory
mkdir -p build

# Compile the test
gcc -o build/config_test \
   -DPLATFORM_RPI \
   -I. \
   tests/config_test.c \
   tests/logging_stub.c \
   src/core/mcp/config/mcp_config.c \
   src/core/mcp/config/platform_config.c \
   src/json/json_helpers.c

# Run the test
./build/config_test