# MCP Framework for LLM-Embedded Systems Integration

## Overview

The Model Context Protocol (MCP) framework is a comprehensive solution designed to enable Large Language Models (LLMs) to interact directly with embedded systems through a standardized protocol. This document outlines the architecture, key components, and integration methods that developers can use to incorporate LLM capabilities into resource-constrained embedded devices.

## Architecture

The MCP framework implements a layered architecture:

```
┌─────────────────────────────────────────────────┐
│                  Application                    │
├─────────────────────────────────────────────────┤
│                  MCP Server                     │
├──────────────┬──────────────┬──────────────┬────┤
│ Tool System  │ Device Layer │ Kernel Layer │ IO │
├──────────────┴──────────────┴──────────────┴────┤
│           Platform-Specific Hardware HAL        │
└─────────────────────────────────────────────────┘
```

### Core Components

1. **MCP Server**: Handles communication with external LLMs via standardized protocols
2. **Tool System**: Manages tools that can be dynamically invoked by LLMs
3. **Device Information System**: Provides comprehensive device capabilities and hardware details
4. **Device Layer**: Controls sensors, actuators, and drivers
5. **Kernel Layer**: Provides task scheduling, memory management, and event handling
6. **Hardware Abstraction Layer (HAL)**: Platform-specific implementations

## LLM Integration Points

LLMs can interact with embedded systems through the following key integration points:

### 1. Device Information System

The Device Information System provides comprehensive details about the embedded device's capabilities, hardware, and resources. This is essential for LLMs to understand the operating environment and make appropriate decisions.

```c
// Initialize device information system
int MCP_DeviceInfoInit(void);

// Get device information as structured data
const MCP_DeviceInfo* MCP_DeviceInfoGet(void);

// Get device information as JSON
char* MCP_DeviceInfoToJSON(bool compact);
```

LLMs can query device information using the built-in `system.getDeviceInfo` tool:

```json
{
  "tool": "system.getDeviceInfo",
  "params": {
    "format": "full"
  }
}
```

The response includes detailed information about:
- System information (device name, firmware version, platform, etc.)
- Processor details (model, clock speed, cores, architecture)
- Memory metrics (RAM, flash, heap, stack)
- Available IO ports and their capabilities
- Connected sensors and their specifications
- Network interfaces and connectivity status
- Storage devices and their capacity
- Specific device capabilities and features

This allows LLMs to:
1. Adapt to the specific hardware platform
2. Make resource-aware decisions
3. Use available sensors and actuators appropriately
4. Understand device constraints and capabilities
5. Create optimized, platform-specific tools and automations

### 2. Tool Registry

The Tool Registry is the primary mechanism for LLMs to execute functions on the embedded device.

```c
// Register a tool that LLMs can invoke
int MCP_ToolRegister(const char* name, MCP_ToolHandler handler, const char* schema);

// Execute a tool using JSON parameters
MCP_ToolResult MCP_ToolExecute(const char* json, size_t length);
```

LLMs can use this mechanism to invoke device functions through structured JSON requests:

```json
{
  "tool": "device.readTemperature",
  "params": {
    "sensorId": "temp_sensor1",
    "unit": "celsius"
  }
}
```

### 2. Dynamic Device Registration

LLMs can dynamically register new sensors and actuators at runtime using the No-Code API:

```c
// Register sensors from JSON configuration
int MCP_NoCodeConfigureSensors(const char* configJson, size_t length);

// Register actuators from JSON configuration
int MCP_NoCodeConfigureActuators(const char* configJson, size_t length);
```

Example JSON for device registration:

```json
{
  "sensors": [
    {
      "id": "temp_sensor1",
      "name": "Living Room Temperature",
      "type": "MCP_SENSOR_TYPE_TEMPERATURE",
      "interface": "MCP_SENSOR_INTERFACE_I2C",
      "driverId": "ds18b20",
      "pin": "D5",
      "configJson": {
        "address": "0x48",
        "sampleInterval": 5000
      }
    }
  ]
}
```

### 3. Automation Rules

LLMs can define automation rules that are executed when specific conditions are met:

```c
// Create automation rule from JSON definition
const char* MCP_AutomationCreateRule(const char* json, size_t length);
```

Example rule definition:

```json
{
  "rule": {
    "name": "HighTemperatureAlert",
    "description": "Send alert when temperature exceeds threshold",
    "triggers": [
      {
        "type": "condition",
        "sensor": "temp_sensor1",
        "operator": "greater_than",
        "value": 30
      }
    ],
    "actions": [
      {
        "type": "notification",
        "message": "Temperature too high!",
        "level": "warning"
      }
    ],
    "enabled": true
  }
}
```

### 4. Dynamic Tool Definition

LLMs can define new tools at runtime using composite operations:

```c
// Register a dynamic tool from JSON definition
int MCP_ToolRegisterDynamic(const char* json, size_t length);

// Save a dynamic tool to persistent storage
int MCP_ToolSaveDynamic(const char* name);

// Load a dynamic tool from persistent storage
int MCP_ToolLoadDynamic(const char* name);
```

Example composite tool definition:

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

Dynamic tools support multiple implementation types:
- **Composite**: Tools composed of multiple steps executed in sequence (as shown above)
- **Script**: Script-based tools in languages like Lua or JavaScript
- **Bytecode**: Pre-compiled bytecode for efficient execution
- **Native**: Direct mapping to C functions

Variable substitution with `{{variable}}` syntax allows for parameter passing between steps.

## Context Management

The Context Manager handles variables and execution environments for LLM-driven operations:

```c
// Create execution context
MCP_ExecutionContext* MCP_ContextCreate(const char* name, MCP_ExecutionContext* parent, int maxVariables);

// Set variable in context
int MCP_ContextSetVariable(MCP_ExecutionContext* context, const char* name, const MCP_Variable* value);

// Store tool execution result in context
int MCP_ContextStoreToolResult(MCP_ExecutionContext* context, const char* name, const MCP_ToolResult* result);

// Substitute variables in template
char* MCP_ContextSubstituteVariables(MCP_ExecutionContext* context, const char* template);
```

The variable substitution syntax `{{variable}}` allows LLMs to pass data between tool execution steps. Optional default values can be specified using the format `{{variable|default}}`.

## Persistent Storage

LLMs can use persistent storage to save and retrieve configurations:

```c
// Write data to persistent storage
int persistent_storage_write(const char* key, const void* data, size_t size);

// Read data from persistent storage
int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize);
```

This enables LLMs to maintain state between sessions and after device restarts.

## Implementation Example

Here's a complete example of how an LLM might interact with the MCP framework:

1. **Register a temperature sensor**:
   ```json
   {
     "sensors": [
       {
         "id": "temp_sensor1",
         "type": "MCP_SENSOR_TYPE_TEMPERATURE",
         "driverId": "ds18b20",
         "pin": "D5"
       }
     ]
   }
   ```

2. **Create an automation rule**:
   ```json
   {
     "rule": {
       "name": "TemperatureMonitor",
       "triggers": [
         {
           "type": "schedule",
           "interval": 60000
         }
       ],
       "actions": [
         {
           "type": "tool",
           "tool": "device.readTemperature",
           "params": {"sensorId": "temp_sensor1"},
           "store": "temperature"
         },
         {
           "type": "notification",
           "message": "Current temperature: {{temperature}} °C"
         }
       ]
     }
   }
   ```

3. **Define a custom tool**:
   ```json
   {
     "tool": "system.defineTool",
     "params": {
       "name": "climate.adjustTemperature",
       "implementationType": "composite",
       "implementation": {
         "steps": [
           {
             "tool": "device.readTemperature",
             "params": {"sensorId": "temp_sensor1"},
             "store": "currentTemp"
           },
           {
             "tool": "device.setHeater",
             "params": {
               "state": "{{currentTemp < targetTemp}}",
               "actuatorId": "heater1"
             }
           }
         ]
       },
       "schema": {
         "properties": {
           "targetTemp": {"type": "number"}
         }
       },
       "persistent": true
     }
   }
   ```

4. **Execute the custom tool**:
   ```json
   {
     "tool": "climate.adjustTemperature",
     "params": {
       "targetTemp": 22.5
     }
   }
   ```

## Platform Support

The MCP framework supports multiple embedded platforms through a comprehensive Hardware Abstraction Layer (HAL) that provides consistent APIs while accommodating platform-specific capabilities:

### Arduino Platform

The Arduino implementation provides broad compatibility across the Arduino ecosystem:

- **Hardware Detection**: Uses preprocessor directives to detect chip architecture at build time
- **Pin Mapping**: Maps standard Arduino pin naming conventions (D0-D13, A0-A5) to physical pins with architecture-specific capabilities
- **Device Information**: Populates device capabilities using Arduino-specific macros
- **Board Support**: Compatible with Arduino Uno, Mega, Due, Zero, MKR series, Nano, and most official and third-party boards
- **Peripheral Access**: Provides standardized access to GPIO, ADC, UART, I2C, SPI, PWM interfaces through the HAL layer

### mbed Platform

The mbed implementation leverages the mbed OS capabilities with enhanced abstractions:

- **Dynamic Configuration**: Uses runtime pin discovery and mapping through configurable systems
- **Interface Discovery**: Rich metadata structures allow dynamic discovery of pin capabilities
- **Logical Mapping**: Abstract mapping system provides flexible mapping between logical IDs and physical pins
- **Board Support**: Compatible with mbed-enabled boards including STM32, NXP, Nordic nRF series
- **RTOS Integration**: Leverages mbed OS task scheduling, synchronization, and memory management

### Raspberry Pi Platform

The Raspberry Pi implementation supports both Linux and bare metal environments:

- **Bare Metal Support**: Direct hardware access for all Raspberry Pi models (1, 2, 3, 4, 400, Zero, Zero W, 5)
- **Model Detection**: Automatically detects Pi model and configures appropriate peripheral base addresses
- **Memory-Mapped I/O**: Provides direct access to GPIO, UART, SPI, I2C, and PWM peripherals
- **Linux Integration**: On Linux-based systems, offers higher-level abstractions through standard system calls
- **Hardware Information**: Comprehensive device reporting with processor model, clock speed, memory specifications

### ESP32 Platform

The ESP32 implementation provides native support for ESP-IDF features:

- **WiFi/BLE Integration**: First-class support for wireless capabilities
- **Deep Sleep Modes**: Power optimization through ESP32 sleep modes
- **Flash Management**: Safe flash access with wear leveling
- **Multi-core Support**: Optimized for dual-core operation
- **Device Information**: Detailed reporting of chip-specific capabilities and features
- **Variant Support**: Compatible with ESP32, ESP32-S2, ESP32-S3, ESP32-C3, and other variants

## Security Considerations

When integrating LLMs with embedded systems:

1. **Input Validation**: All JSON inputs from LLMs should be validated against schemas
2. **Resource Limits**: Implement timeouts and memory limits for LLM-driven operations
3. **Access Control**: Restrict sensitive operations through permission systems
4. **Secure Communication**: Encrypt communication between LLMs and embedded devices

## Best Practices

1. **Device Discovery**: Always query device information first to understand the platform capabilities
2. **Memory Efficiency**: Use the memory manager to control allocations and respect device memory constraints
3. **Error Handling**: Always check return codes from MCP functions
4. **Composite Tools**: Prefer composite tools over complex native implementations
5. **Persistent Storage**: Save important configurations to survive power cycles
6. **Platform Abstraction**: Use HAL functions rather than direct hardware access
7. **Capability-Based Design**: Check for specific device capabilities before attempting to use them
8. **Resource Conservation**: Be mindful of memory, CPU, and power limitations on embedded devices

## Conclusion

The MCP framework provides a powerful yet flexible system for LLM integration with embedded devices. By leveraging dynamic tool registration, automation rules, and context management, developers can create intelligent embedded systems that benefit from the capabilities of Large Language Models while respecting the constraints of resource-limited environments.