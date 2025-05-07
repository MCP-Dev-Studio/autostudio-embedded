# MCP Server Implementation for Embedded Systems

This project implements the Model Context Protocol (MCP) server for embedded systems. It provides a framework for exposing device capabilities through a standardized protocol and enables dynamic tool registration and execution.

## Features

- **Transport Options**: Supports multiple transport types including:
  - UART
  - TCP
  - Bluetooth
  - USB (New)
  - Ethernet (New)
  - Custom transports

- **Authentication System**: 
  - Starts with open authentication by default (no security required)
  - Can be configured to use various authentication methods
  - Supports Bearer tokens, API keys, and other authentication methods

- **Driver Bridge System**:
  - Allows existing native drivers to be used with MCP
  - Bridges existing hardware drivers to the bytecode system
  - Supports LED drivers and temperature sensors
  - Dynamically registers drivers via JSON

- **Bytecode Execution**:
  - Memory-efficient approach for driver implementation
  - Optimized for embedded systems

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

## Configuration

The MCP server can be configured by modifying the server configuration structure:

```c
MCP_ServerConfig config = {
    .deviceName = "MCP Embedded Device",
    .version = "1.0.0",
    .maxSessions = 4,
    .maxOperationsPerSession = 100,
    .maxContentSize = 4096,
    .sessionTimeout = 30000,
    .enableTools = true,
    .enableResources = true,
    .enableEvents = true,
    .enableAutomation = true,
    .deviceId = "mcp-embedded-001",
    .maxTools = 32,
    .maxDrivers = 16,
    .initialOpenAccess = true,  // Start with open authentication
    .enableUSB = true,          // Enable USB transport
    .enableEthernet = true,     // Enable Ethernet transport
    .enableWifi = false,
    .enableBluetooth = true,
    .enableSerial = true
};
```

## Transport Options

### USB Transport

The USB transport allows MCP to communicate over USB interfaces. It supports:

- CDC (Communications Device Class)
- HID (Human Interface Device)
- Vendor-specific USB classes

Configuration example:

```c
MCP_USBTransportConfig usbConfig = {
    .vendorId = 0x1234,
    .productId = 0x5678,
    .deviceClass = MCP_USB_CLASS_CDC,
    .readTimeout = 5000,
    .writeTimeout = 5000,
    .nonBlocking = false,
    .inEndpoint = 0x81,
    .outEndpoint = 0x01,
    .serialNumber = "MCP123456",
    .manufacturer = "MCP Embedded",
    .productName = "MCP USB Device"
};
```

### Ethernet Transport

The Ethernet transport allows MCP to communicate over local networks. It supports:

- DHCP or static IP configuration
- Configurable TCP port and connection limits
- Optional mDNS/Bonjour discovery

Configuration example:

```c
MCP_EthernetTransportConfig ethernetConfig = {
    .mode = MCP_ETHERNET_MODE_DHCP,
    .port = 5555,
    .maxConnections = 4,
    .connectionTimeout = 30000,
    .enableMDNS = true,
    .mdnsServiceName = "mcp-device"
};
```

## Driver Bridge System

The driver bridge system allows existing hardware drivers to be exposed through MCP. It provides a compatibility layer between native device drivers and the bytecode driver system.

### Using Existing Drivers

You can register existing drivers using the bridge system:

```c
// Register an LED driver
MCP_CreateLEDDriver("led1", "Main Status LED", LED_TYPE_RGB, 13);

// Register a temperature sensor
MCP_CreateDS18B20Driver("tempSensor1", "Water Temperature", 5);
```

LLMs can also register drivers through MCP:

```json
{
  "tool": "system.registerNativeDriver",
  "params": {
    "id": "led1",
    "name": "RGB Status Indicator",
    "type": 1,
    "deviceType": 1002,
    "configSchema": {
      "type": "object",
      "properties": {
        "pin": {"type": "number"},
        "activeHigh": {"type": "boolean"}
      }
    }
  }
}
```

## Authentication System

The authentication system is designed to be open by default but configurable for security:

- When initialized with `initialOpenAccess = true`, all operations work without authentication
- Can be configured to use various authentication methods (Bearer, API key, etc.)
- Provides tools for managing authentication at runtime

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

## Documentation

For detailed documentation, see the docs directory:

- [MCP Server Implementation Requirements](docs/MCP%20Server%20Implementation%20Requirements%20for%20Embedded%20Systems.md)
- [Driver Bridge System](docs/DRIVER_BRIDGE.md)
- [Build Fixes](docs/BUILD_FIXES.md)
- [Code Organization](docs/ORGANIZATION.md)

## License

[MIT License](LICENSE)