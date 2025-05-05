#!/bin/bash
# Simple build script for testing the MCP implementation

# Create build directory
mkdir -p build

# Compile common files
echo "Compiling persistent_storage.c..."
gcc -c -Wall -Wextra -g persistent_storage.c -o build/persistent_storage.o

echo "Compiling json_parser.c..."
gcc -c -Wall -Wextra -g json_parser.c -o build/json_parser.o

echo "Compiling logging.c..."
gcc -c -Wall -Wextra -g logging.c -o build/logging.o

echo "Compiling core/tool_system/tool_registry.c..."
gcc -c -Wall -Wextra -g -I. core/tool_system/tool_registry.c -o build/tool_registry.o

echo "Compiling core/tool_system/context_manager.c..."
gcc -c -Wall -Wextra -g -I. core/tool_system/context_manager.c -o build/context_manager.o

echo "Compiling core/device/device_info.c..."
gcc -c -Wall -Wextra -g -I. core/device/device_info.c -o build/device_info.o

# Build and test dynamic tools
echo "Building dynamic tools test..."
gcc -c -Wall -Wextra -g -I. test_dynamic_tools.c -o build/test_dynamic_tools.o
gcc build/persistent_storage.o build/json_parser.o build/logging.o build/tool_registry.o build/context_manager.o build/device_info.o build/test_dynamic_tools.o -o build/test_dynamic_tools

# Build and test device info
echo "Building device info test..."
gcc -c -Wall -Wextra -g -I. test_device_info.c -o build/test_device_info.o
gcc build/persistent_storage.o build/json_parser.o build/logging.o build/tool_registry.o build/context_manager.o build/device_info.o build/test_device_info.o -o build/test_device_info

echo "Build completed!"

# Run tests
echo
echo "===================================================="
echo "Running dynamic tool test..."
echo "===================================================="
build/test_dynamic_tools

echo
echo "===================================================="
echo "Running device info test..."
echo "===================================================="
build/test_device_info