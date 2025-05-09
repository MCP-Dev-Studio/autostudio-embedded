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

To build this project, you need the appropriate toolchain for your target platform:

- **Raspberry Pi**: Standard Linux build tools (gcc, make, cmake)
- **ESP32**: ESP-IDF toolchain or Arduino CLI with ESP32 support
- **Arduino**: Arduino CLI with appropriate board support packages
- **Mbed**: Mbed CLI and the GNU Arm Embedded toolchain

For detailed platform-specific setup instructions, prerequisites, and build processes, see the [Platform Guide](docs/PLATFORM_GUIDE.md).

### Cross-Platform Build

The project includes a build script that uses the appropriate native toolchain for each platform:

```bash
# Build for ESP32 (default)
./scripts/build.sh esp32

# Build for Raspberry Pi
./scripts/build.sh rpi

# Build for Arduino
./scripts/build.sh arduino

# Build for Mbed
./scripts/build.sh mbed
```

This build script automatically:
- Detects if the required platform-specific toolchain is installed
- Sets up the appropriate project structure for the target platform
- Configures and builds using the native build system for that platform
- Falls back to a host-based build for testing if necessary

For detailed platform-specific instructions, see the [Platform Guide](docs/PLATFORM_GUIDE.md).

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

### Platform-Specific Considerations

#### Raspberry Pi

- Builds using standard Linux tools (gcc, make, cmake)
- Camera support through Pi Camera libraries
- Full network stack support (WiFi, Ethernet, Bluetooth)
- GPIO access through standard Linux interfaces
- Native file system for configuration storage

#### ESP32

- Requires ESP-IDF toolchain for actual device deployment
- OTA update support for field upgrades
- Deep sleep power management features
- Web server capabilities
- NVS (Non-Volatile Storage) for configuration

#### Arduino

- Packaged as an Arduino library for integration with Arduino IDE/CLI
- Limited resources require optimization of memory usage
- Watchdog timer support for reliability
- Platform-specific analog reference configuration
- EEPROM or SD card for configuration storage

#### Mbed

- Built using Mbed CLI and the Mbed OS development environment
- RTOS support for multi-threaded applications
- Configurable task stack sizes
- Hardware abstraction through Mbed HAL
- Multiple storage options (SD, flash) for configuration

## Key Components

### Configuration System

The configuration system provides a cross-platform way to manage settings across all supported platforms:

- **Unified Structure**: Common configuration fields shared across all platforms
- **Platform Extensions**: Platform-specific configuration options
- **JSON Support**: All settings configurable via JSON
- **Persistent Storage**: Configurations saved to appropriate platform storage
- **Runtime Updates**: Change configuration without recompiling

Each platform implements the configuration system differently:

- **Raspberry Pi**: JSON files in the filesystem
- **ESP32**: NVS (Non-Volatile Storage)
- **Arduino**: EEPROM or SD card
- **Mbed**: FlashIAP or SD card

To learn more, see the [MCP Configuration documentation](docs/MCP_CONFIGURATION.md).

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
- Composite (composed of other tools)[build.sh](scripts/build.sh)
- Script-based
- Bytecode

### Persistent Storage

The persistent storage system provides a reliable way to store tool definitions and other configuration data across device resets.

## Documentation

For detailed documentation, see the docs directory:

- [MCP Server Implementation Requirements](docs/MCP%20Server%20Implementation%20Requirements%20for%20Embedded%20Systems.md)
- [Platform Guide](docs/PLATFORM_GUIDE.md) - Detailed setup and build instructions for each platform
- [Driver Bridge System](docs/DRIVER_BRIDGE.md) - How drivers are bridged to the MCP system
- [MCP Configuration](docs/MCP_CONFIGURATION.md) - Configuration system documentation
- [Code Organization](docs/ORGANIZATION.md) - Structure and organization of the codebase

## License

[Apache License 2.0](LICENSE)

Copyright 2025 MCP Embedded System

Licensed under the Apache License, Version 2.0