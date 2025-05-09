# MCP Configuration System

This document explains how to use the MCP configuration system to make all settings configurable via MCP without requiring code changes.

## Overview

The MCP configuration system provides a standardized way to manage settings across all supported platforms (Raspberry Pi, ESP32, Arduino, and Mbed). It includes:

1. A common configuration structure shared by all platforms
2. Platform-specific configuration extensions
3. JSON serialization and deserialization
4. Persistent storage of settings
5. Runtime configuration updates via API

## Key Features

- **Consistent API**: Same API across all platforms for getting and setting configuration
- **Platform-independent Core**: Common configuration that applies to all platforms
- **Platform-specific Extensions**: Each platform has its own specialized settings
- **JSON Configuration**: All settings can be loaded/saved in JSON format
- **Persistent Storage**: Settings are saved to non-volatile storage
- **Thread Safety**: Mutex-based protection for configuration access
- **Secure Credential Handling**: Safe storage of sensitive settings
- **Runtime Updates**: Configuration can be modified without code changes

## Architecture

The configuration system consists of these components:

1. **Common Configuration Structure** (`MCP_CommonConfig`): Core settings shared by all platforms
2. **Platform-specific Extensions**: Each platform adds its own specialized settings
3. **Configuration API**: Functions to load, save, and modify settings
4. **JSON Serialization/Deserialization**: Convert configuration to/from JSON
5. **Persistent Storage Interface**: Save/load configuration from storage
6. **Platform-specific Implementations**: Each platform implements the required functionality

## Using the Configuration System

### Initialization

Each platform has its own initialization function that takes a platform-specific configuration:

```c
// Raspberry Pi
MCP_RPiConfig config = {
    .deviceName = "My RPi Device",
    .version = "1.0.0",
    .enableDebug = true,
    // Other settings
};
MCP_SystemInit(&config);

// ESP32
MCP_ESP32Config config = {
    .deviceName = "My ESP32 Device",
    .version = "1.0.0",
    .enableDebug = true,
    // Other settings
};
MCP_SystemInit(&config);

// Arduino
MCP_ArduinoConfig config = {
    .deviceName = "My Arduino Device",
    .version = "1.0.0",
    .enableDebug = true,
    // Other settings
};
MCP_SystemInit(&config);

// Mbed
MCP_MbedConfig config = {
    .deviceName = "My Mbed Device",
    .version = "1.0.0",
    .enableDebug = true,
    // Other settings
};
MCP_SystemInit(&config);
```

### Getting and Setting Configuration via JSON

You can get or set the entire configuration as JSON:

```c
// Get configuration as JSON
char config_json[4096];
int len = MCP_GetConfiguration(config_json, sizeof(config_json));

// Set configuration via JSON
const char* update_json = "{"
    "\"device\": {"
    "  \"name\": \"Updated Device Name\","
    "  \"debug_enabled\": true"
    "}"
    // Other settings
"}";
int result = MCP_SetConfiguration(update_json);
```

### Persistent Storage

The configuration is automatically saved to persistent storage when updated via the API:

```c
// Save configuration manually
int result = MCP_SavePersistentState();

// Load configuration manually
int result = MCP_LoadPersistentState();
```

### External Configuration File

You can use an external JSON file for configuration. By default, the system looks for a configuration file at the path specified in the configuration structure:

```c
config.configFile = "/etc/mcp/config.json";
```

### Thread Safety

All configuration operations are thread-safe, using mutex protection to prevent race conditions.

## Example JSON Configuration

Here's an example of a complete configuration file:

```json
{
  "device": {
    "name": "MCP Example Device",
    "firmware_version": "1.2.0",
    "debug_enabled": true
  },
  "server": {
    "enabled": true,
    "port": 8080,
    "auto_start": true
  },
  "network": {
    "wifi": {
      "enabled": true,
      "ssid": "YourNetworkSSID",
      "password": "YourNetworkPassword",
      "auto_connect": true
    },
    "wifi_ap": {
      "enabled": false,
      "ssid": "MCP-Device-AP",
      "password": "mcp-access",
      "channel": 6
    },
    "ethernet": {
      "enabled": true,
      "interface": "eth0",
      "dhcp": true,
      "static_ip": "192.168.1.100",
      "gateway": "192.168.1.1",
      "subnet": "255.255.255.0"
    }
  },
  "interfaces": {
    "i2c": {
      "enabled": true,
      "bus_number": 1
    },
    "spi": {
      "enabled": true,
      "bus_number": 0
    },
    "uart": {
      "enabled": true,
      "number": 0,
      "baud_rate": 115200
    },
    "gpio": {
      "enabled": true
    }
  },
  "system": {
    "heap_size": 1048576,
    "config_file_path": "/etc/mcp/config.json",
    "enable_persistence": true
  },
  "platform": {
    "rpi": {
      "enable_camera": true,
      "camera_resolution": 1080
    }
  }
}
```

## Platform-specific Settings

Each platform has its own specific settings:

### Raspberry Pi

- **Camera Settings**: Enable/disable camera and set resolution
- **GPIO Configuration**: Pin mappings and functions
- **Network Interfaces**: WiFi, Ethernet, and Bluetooth configurations

### ESP32

- **OTA Updates**: Enable/disable OTA updates
- **Web Server**: Enable/disable built-in web server
- **Deep Sleep**: Enable/disable deep sleep and set sleep duration

### Arduino

- **Analog Reference**: Set analog reference mode
- **Watchdog**: Enable/disable watchdog timer

### Mbed

- **RTOS Settings**: Enable/disable RTOS and set task stack size
- **Thread Configuration**: Configure thread priorities and stack sizes

## Security Considerations

- **Passwords**: Sensitive information is stored in configuration but isn't exposed in debug output
- **File Permissions**: Configuration files have appropriate permissions
- **Memory Management**: Proper bounds checking and memory allocation/deallocation

## Example Code

See the following examples for practical usage:

- `docs/examples/config_example.c`: Example code demonstrating configuration API
- `docs/examples/config_example.json`: Example JSON configuration file