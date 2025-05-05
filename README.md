# MCP Server Implementation for Embedded Systems

This project implements a Model Context Protocol (MCP) server designed for resource-constrained embedded systems. It enables Large Language Models (LLMs) to interact with embedded devices through a standardized protocol, providing tool registration, execution, and context management capabilities.

## Key Features

- **Cross-Platform Support**: Works on mbed, Arduino, ESP32, and other embedded platforms
- **Dynamic Tool Registration**: Define tools at runtime using JSON
- **Composite Tool Support**: Create tools composed of multiple steps executed in sequence
- **Persistent Storage**: Store and retrieve tools across device restarts
- **Variable Substitution**: Pass data between tool steps using template variables
- **Memory Efficient**: Designed for constrained memory environments
- **Extensible Architecture**: Supports native, composite, script, and bytecode tools

## Architecture

The implementation follows a modular design with the following components:

### Core Components

- **Tool Registry**: Manages tool registration, discovery, and execution
- **Context Manager**: Handles variable storage and substitution between tool steps
- **JSON Parser**: Provides lightweight JSON parsing for dynamic tool definitions
- **Persistent Storage**: Cross-platform storage abstraction for saving tool definitions

### Platform Support

- **mbed**: HAL implementation for mbed-based devices
- **Arduino**: HAL implementation for Arduino boards
- **ESP32**: Specialized implementation leveraging ESP-IDF features

## Tool Types

The system supports multiple tool types:

1. **Native Tools**: Direct C function implementations
2. **Composite Tools**: Multi-step workflows defined in JSON
3. **Script Tools**: Interpreted script-based tools (e.g., Lua)
4. **Bytecode Tools**: Pre-compiled bytecode for efficient execution

## Getting Started

### Building the Project

Use the included build script to compile and test the implementation:

```bash
# Make the build script executable
chmod +x build.sh

# Build and run the test
./build.sh
```

### Testing Dynamic Tool Registration

The `test_dynamic_tools.c` file demonstrates how to use the dynamic tool registration and persistence features:

```c
// Register a dynamic composite tool
int result = MCP_ToolRegisterDynamic(toolJson, strlen(toolJson));

// Execute the tool
MCP_ToolResult result = MCP_ToolExecute(execJson, strlen(execJson));

// Save tool to persistent storage
MCP_ToolSaveDynamic(toolName);

// Load tool from persistent storage
MCP_ToolLoadDynamic(toolName);
```

## API Reference

### Tool Registry

- `MCP_ToolRegistryInit`: Initialize the tool registry
- `MCP_ToolRegister`: Register a native tool
- `MCP_ToolRegisterDynamic`: Register a tool from JSON definition
- `MCP_ToolExecute`: Execute a registered tool
- `MCP_ToolGetDefinition`: Get tool definition
- `MCP_ToolSaveDynamic`: Save a tool to persistent storage
- `MCP_ToolLoadDynamic`: Load a tool from persistent storage

### Context Manager

- `MCP_ContextCreate`: Create a new execution context
- `MCP_ContextFree`: Free a context and its resources
- `MCP_ContextStoreVariable`: Store a variable in a context
- `MCP_ContextGetVariable`: Retrieve a variable from a context
- `MCP_ContextSubstituteVariables`: Perform variable substitution in a template

### Persistent Storage

- `persistent_storage_init`: Initialize storage system
- `persistent_storage_write`: Write data to storage
- `persistent_storage_read`: Read data from storage
- `persistent_storage_delete`: Delete a key from storage
- `persistent_storage_get_keys`: List all stored keys
- `persistent_storage_commit`: Commit changes to storage

## Example Dynamic Tool Definition

```json
{
  "tool": "system.defineTool",
  "params": {
    "name": "device.ledBlink",
    "description": "Blinks an LED",
    "implementationType": "composite",
    "implementation": {
      "steps": [
        {
          "tool": "device.ledOn",
          "params": {"pin": "{{pin}}"},
          "store": "step1Result"
        },
        {
          "tool": "system.delay",
          "params": {"milliseconds": 500}
        },
        {
          "tool": "device.ledOff",
          "params": {"pin": "{{pin}}"},
          "store": "step2Result"
        }
      ]
    },
    "schema": {
      "properties": {
        "pin": {"type": "number"}
      },
      "required": ["pin"]
    },
    "persistent": true
  }
}
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.