#!/bin/bash
# Fixed build script for testing the MCP implementation

# Create build directory
mkdir -p build

# Compile with strict standards
echo "Compiling persistent_storage.c..."
gcc -c -Wall -Wextra -g -std=c99 persistent_storage.c -o build/persistent_storage.o

echo "Compiling build_fix.c..."
gcc -c -Wall -Wextra -g -std=c99 build_fix.c -o build/build_fix.o

echo "Compiling logging.c..."
gcc -c -Wall -Wextra -g -std=c99 logging.c -o build/logging.o

echo "Compiling core/tool_system/tool_registry.c..."
gcc -c -Wall -Wextra -g -std=c99 -I. core/tool_system/tool_registry.c -o build/tool_registry.o

echo "Compiling core/tool_system/context_manager.c..."
gcc -c -Wall -Wextra -g -std=c99 -I. core/tool_system/context_manager.c -o build/context_manager.o || true

echo "Compiling core/device/device_info.c..."
gcc -c -Wall -Wextra -g -std=c99 -I. core/device/device_info.c -o build/device_info.o

# Compile JSON mocks
echo "Compiling json_parser.c..."
gcc -c -Wall -Wextra -g -std=c99 json_parser.c -o build/json_parser.o

echo "Compiling json_mock.c..."
gcc -c -Wall -Wextra -g -std=c99 json_mock.c -o build/json_mock.o

echo "Compiling json_device_info_mock.c..."
gcc -c -Wall -Wextra -g -std=c99 json_device_info_mock.c -o build/json_device_info_mock.o

# Build test_dynamic_tools with json_mock.c
echo "Building dynamic tools test..."
gcc -c -Wall -Wextra -g -std=c99 -I. test_dynamic_tools.c -o build/test_dynamic_tools.o
gcc build/build_fix.o build/persistent_storage.o build/logging.o build/tool_registry.o build/context_manager.o build/test_dynamic_tools.o build/json_mock.o -o build/test_dynamic_tools

# Build test_device_info with json_device_info_mock.c
echo "Building device info test..."
gcc -c -Wall -Wextra -g -std=c99 -I. test_device_info.c -o build/test_device_info.o
gcc build/build_fix.o build/persistent_storage.o build/logging.o build/tool_registry.o build/context_manager.o build/device_info.o build/test_device_info.o build/json_device_info_mock.o -o build/test_device_info

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