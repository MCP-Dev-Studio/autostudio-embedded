# MCP Driver Bridge System

This document describes the MCP Driver Bridge system, which provides a way to bridge native device drivers with the bytecode driver system, allowing existing hardware drivers to be exposed through the Model Context Protocol (MCP).

## Overview

The Driver Bridge system acts as a compatibility layer between:
- Native device drivers (e.g., LED, temperature sensors, etc.)
- The bytecode driver system used by MCP

This allows existing drivers to be dynamically registered and exposed to LLMs through MCP without having to rewrite them as bytecode drivers.

## Architecture

The Driver Bridge consists of the following components:

1. **Bridge Registry** - Tracks registered native drivers and their function mappings
2. **Function Mapping System** - Maps standard MCP driver functions to native driver functions
3. **Bridge Interface** - Implements the bytecode driver interface but forwards calls to native functions
4. **Tool Handlers** - Provides MCP tools for registering and managing native drivers

## Supported Drivers

The current implementation supports the following native drivers:

### Actuators
- **LED drivers**
  - Simple (on/off)
  - PWM (brightness control)
  - RGB/RGBW (color control)
  - Addressable LED strips

### Sensors
- **Temperature Sensors**
  - DS18B20 (1-Wire digital temperature sensor)

## Using the Bridge System

### Initializing the Bridge System

To use the Driver Bridge system, it must be initialized during system startup:

```c
// Initialize the driver bridge system
MCP_DriverBridgeInit();

// Initialize the bridge tool handlers (optional, for MCP tool access)
MCP_DriverBridgeToolsInit();
```

### Registering Native Drivers Programmatically

You can register native drivers directly in code:

```c
// Register an LED driver
MCP_CreateLEDDriver("led1", "Main Status LED", LED_TYPE_RGB, 13);

// Register a temperature sensor
MCP_CreateDS18B20Driver("tempSensor1", "Water Temperature", 5);
```

### Registering Native Drivers via MCP

LLMs can register native drivers through MCP using the `system.registerNativeDriver` tool:

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

### Executing Native Driver Functions

Once registered, you can execute native driver functions via MCP:

```json
{
  "tool": "system.executeDriverFunction",
  "params": {
    "id": "led1",
    "function": "read"
  }
}
```

You can also use driver-specific functions:

```json
{
  "tool": "system.executeNativeDriverFunction",
  "params": {
    "id": "led1",
    "function": "setColor",
    "args": {
      "r": 255,
      "g": 0,
      "b": 0
    }
  }
}
```

## Device Type Constants

The `DeviceType` enum defines constants for various device types:

```c
// Actuator types (1000-1999)
DEVICE_TYPE_LED_SIMPLE = 1000
DEVICE_TYPE_LED_PWM = 1001
DEVICE_TYPE_LED_RGB = 1002
DEVICE_TYPE_LED_RGBW = 1003
DEVICE_TYPE_LED_ADDRESSABLE = 1004
DEVICE_TYPE_SERVO = 1005
DEVICE_TYPE_RELAY = 1006

// Sensor types (2000-2999)
DEVICE_TYPE_TEMPERATURE_DS18B20 = 2000
DEVICE_TYPE_HUMIDITY_DHT22 = 2001
DEVICE_TYPE_MOTION_PIR = 2002

// Custom types (5000+)
DEVICE_TYPE_CUSTOM = 5000
```

## Adding Support for New Driver Types

To add support for a new native driver type:

1. Add a new device type constant to the `DeviceType` enum in `driver_bridge.h`
2. Create a bridge function in `driver_bridge.c` similar to `bridgeLEDDriver()` or `bridgeDS18B20Driver()`
3. Add a mapping function to map the driver's functions to bytecode functions

Example for adding a new driver type:

```c
// In driver_bridge.c
static int bridgeNewDriverType(const char* id, const char* name) {
    // Map driver functions
    MCP_DriverBridgeMapFunction(id, "init", NewDriver_Init);
    MCP_DriverBridgeMapFunction(id, "deinit", NewDriver_Deinit);
    MCP_DriverBridgeMapFunction(id, "customFunction1", NewDriver_CustomFunction1);
    MCP_DriverBridgeMapFunction(id, "customFunction2", NewDriver_CustomFunction2);
    
    return 0;
}
```

## Integration with Existing Code

The bridge system integrates with the following components:

1. **Driver Manager** - Manages all drivers in the system
2. **Bytecode Driver System** - Provides the bytecode interface for MCP
3. **Tool Registry** - Registers tools for MCP

## Limitations and Constraints

1. Function parameter and return types must be compatible between native drivers and MCP
2. Complex parameter structures may require manual conversion
3. Native drivers still operate within the memory and performance constraints of the embedded system
4. Thread safety depends on the underlying native driver implementations