# MCP Server Implementation for Embedded Systems

This project implements the Model Context Protocol (MCP) server for embedded systems. It provides a framework for exposing device capabilities through a standardized protocol and enables dynamic tool registration and execution.

## Directory Structure

```
.
├── CMakeLists.txt        # Main CMake build configuration
├── docs/                 # Documentation
├── scripts/              # Build and utility scripts
├── src/                  # Source code
│   ├── core/             # Core MCP implementation
│   │   ├── device/       # Device abstraction layer
│   │   ├── kernel/       # Kernel services (task scheduler, config system)
│   │   ├── mcp/          # MCP protocol implementation
│   │   └── tool_system/  # Tool registration and execution system
│   ├── driver/           # Device drivers
│   │   ├── actuators/    # Actuator drivers (LED, motors, relays)
│   │   └── sensors/      # Sensor drivers (temperature, humidity, motion)
│   ├── hal/              # Hardware Abstraction Layer
│   │   ├── arduino/      # Arduino platform support
│   │   ├── esp32/        # ESP32 platform support
│   │   ├── mbed/         # MbedOS platform support
│   │   └── rpi/          # Raspberry Pi platform support
│   ├── json/             # JSON parsing and generation
│   ├── main.cpp          # Main application entry point
│   ├── system/           # System services (logging, persistent storage)
│   ├── test/             # Test implementations and mocks
│   └── util/             # Utility functions and helper modules
├── tests/                # Test cases
├── .gitignore
└── README.md
```

## Building the Project

### Prerequisites

- CMake 3.10 or higher
- C99 compatible compiler
- C++11 compatible compiler (for C++ components)

### Build Instructions

```bash
# Create a build directory
mkdir -p build && cd build

# Configure the build
cmake ..

# Build the project
cmake --build .

# Run tests
ctest
```

## Platform Support

The MCP server is designed to be portable across multiple embedded platforms:

- ESP32
- Arduino
- MbedOS
- Raspberry Pi

## Key Components

### Device Information System

The device information system provides a standardized way to expose device capabilities, including:

- System information (device name, firmware version, etc.)
- Processor information (model, clock speed, cores)
- Memory information (RAM, flash)
- IO Ports (digital/analog pins, capabilities)
- Network interfaces
- Sensors
- Storage devices

### Tool Registry

The tool registry enables dynamic registration and execution of tools. Tools can be:

- Native (implemented in C/C++)
- Composite (composed of other tools)
- Script-based
- Bytecode

### Persistent Storage

The persistent storage system provides a reliable way to store tool definitions and other configuration data across device resets.

## License

[MIT License](LICENSE)