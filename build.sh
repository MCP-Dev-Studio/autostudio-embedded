#!/bin/bash
# Simple build script for testing the MCP implementation

# Create build directory
mkdir -p build

# Compile all necessary files
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

echo "Compiling test_dynamic_tools.c..."
gcc -c -Wall -Wextra -g -I. test_dynamic_tools.c -o build/test_dynamic_tools.o

# Link test executable
echo "Linking test_dynamic_tools..."
gcc build/persistent_storage.o build/json_parser.o build/logging.o build/tool_registry.o build/context_manager.o build/test_dynamic_tools.o -o build/test_dynamic_tools

echo "Build completed!"

# Run test
echo "Running dynamic tool test..."
build/test_dynamic_tools