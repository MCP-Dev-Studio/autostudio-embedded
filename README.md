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
- **Device Info**: Provides comprehensive information about device capabilities and resources

### Platform Support

- **mbed**: HAL implementation for mbed-based devices
- **Arduino**: HAL implementation for Arduino boards
- **ESP32**: Specialized implementation leveraging ESP-IDF features
- **Raspberry Pi**: Bare metal implementation for Raspberry Pi 1-5

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

#### Platform-Specific Build Instructions

##### Arduino

```bash
# Building for Arduino Uno
arduino-cli compile --fqbn arduino:avr:uno MyProject

# Building for Arduino Mega
arduino-cli compile --fqbn arduino:avr:mega MyProject

# Building for Arduino Due
arduino-cli compile --fqbn arduino:sam:due MyProject
```

The Fully Qualified Board Name (FQBN) encodes the board configuration, which determines available memory, CPU model, clock speed, and pin capabilities.

##### mbed

```bash
# Selecting target board
mbed target NUCLEO_F446RE

# Configuring build profile (release, debug, develop)
mbed compile -t GCC_ARM -m NUCLEO_F446RE --profile release

# Custom mbed_app.json configuration example
{
  "target_overrides": {
    "NUCLEO_F446RE": {
      "target.features_add": ["BLE"],
      "target.components_add": ["SD"],
      "target.macros_add": ["MCP_CUSTOM_CONFIG"]
    }
  }
}
```

The mbed build system provides target information through the target database, mbed_app.json, and optional component libraries.

##### Raspberry Pi

```bash
# Building for Raspberry Pi 4 bare metal
make PLATFORM=rpi RPI_MODEL=4 

# Building for Raspberry Pi Zero bare metal with specific toolchain
make PLATFORM=rpi RPI_MODEL=zero CROSS_COMPILE=arm-none-eabi-

# Building with custom memory layout
make PLATFORM=rpi RPI_MODEL=3 MEM_BASE=0x8000 MEM_SIZE=128M

# Building for Raspberry Pi OS (Linux)
make PLATFORM=rpi LINUX=1 RASPBIAN=1
```

##### ESP32

```bash
# Configure ESP32 build with menuconfig
idf.py menuconfig

# Build for specific ESP32 variant
idf.py -DIDF_TARGET=esp32s3 build

# Build with custom partitioning
idf.py -DPARTITION_TABLE_CSV_FILE=partitions_custom.csv build
```

The ESP-IDF's sdkconfig system provides configuration options for CPU frequency, memory allocation, and wireless capabilities.

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

### Device Information

- `MCP_DeviceInfoInit`: Initialize the device information system
- `MCP_DeviceInfoGet`: Get comprehensive device information
- `MCP_DeviceInfoToJSON`: Convert device information to JSON
- `MCP_DeviceInfoRegisterIOPort`: Register an IO port
- `MCP_DeviceInfoRegisterSensor`: Register a sensor
- `MCP_DeviceInfoRegisterNetwork`: Register a network interface
- `MCP_DeviceInfoRegisterStorage`: Register a storage device
- `MCP_DeviceInfoSetCapabilities`: Set device capabilities

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

## Built-in Tools

### system.defineTool
Dynamically define a new tool at runtime using JSON configuration.

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

### system.getDeviceInfo
Retrieve comprehensive information about the device's capabilities, hardware, and resources.

```json
{
  "tool": "system.getDeviceInfo",
  "params": {
    "format": "full"
  }
}
```

**Response:**
```json
{
  "system": {
    "deviceName": "ESP32 Device",
    "firmwareVersion": "1.0.0",
    "buildDate": "May 5 2025",
    "platformName": "ESP32",
    "uptime": 3600
  },
  "processor": {
    "model": "ESP32",
    "clockSpeedMHz": 240,
    "coreCount": 2,
    "bitWidth": 32
  },
  "memory": {
    "totalRamKB": 320,
    "freeRamKB": 128,
    "totalFlashKB": 4096
  },
  "ioPorts": [
    {
      "name": "GPIO5",
      "type": "digital",
      "isInput": true,
      "isOutput": true,
      "capabilities": {
        "pwm": true,
        "i2c": true,
        "interrupt": true
      }
    }
  ],
  "sensors": [
    {
      "name": "Temperature Sensor",
      "type": "temperature",
      "units": "°C",
      "range": {
        "min": -40.0,
        "max": 85.0
      },
      "currentValue": 23.5
    }
  ],
  "capabilities": {
    "features": [
      "gpio",
      "i2c",
      "spi",
      "uart",
      "adc",
      "pwm"
    ],
    "maxTools": 64,
    "supportedProtocols": [
      "mqtt",
      "http",
      "websocket"
    ]
  }
}
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.