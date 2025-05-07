# MCP Logging System

The MCP Logging System provides a bridge between the embedded system's logging capabilities and the MCP protocol, allowing log messages to be sent to clients over MCP connections and configured remotely via MCP tools.

## Features

- Send log messages to MCP clients as events
- Remote configuration of logging parameters via MCP tool
- Control log level and filtering on the server side
- Module-based filtering to include/exclude specific modules
- Full control over log formatting
- Configuration persistence
- Real-time configuration updates to all connected clients

## Architecture

The MCP Logging System consists of several components:

1. **Core Logging Bridge** (`mcp_logging.h/c`)
   - Hooks into the existing logging system
   - Converts log messages to MCP events
   - Applies filtering and formatting
   - Sends events to subscribed clients

2. **Configuration Tool** (`mcp_logging_tool.h/c`)
   - Exposes logging configuration as an MCP tool
   - Allows clients to retrieve and modify logging settings
   - Broadcasts configuration changes to all connected clients

3. **Log Events**
   - `MCP_EVENT_TYPE_LOG` - Contains log messages
   - `MCP_EVENT_TYPE_LOG_CONFIG` - Contains configuration updates

## Usage

### Server-side Integration

1. Initialize the logging bridge:

```c
#include "core/mcp/logging/mcp_logging.h"

// Initialize with server instance and maximum log level
MCP_LoggingInit(serverInstance, LOG_LEVEL_DEBUG);

// Enable logging to MCP clients
MCP_LoggingEnable(true);
```

2. Register the logging tool:

```c
#include "core/mcp/logging/mcp_logging_tool.h"

// Initialize and register the tool
MCP_LoggingToolInit();
MCP_LoggingToolRegister();
```

3. Use the standard logging system:

```c
#include "system/logging.h"

LOG_INFO("MyModule", "This is an informational message");
LOG_ERROR("MyModule", "An error occurred: %d", errorCode);
```

### Client-side Interaction

Clients can interact with the logging system in two ways:

1. **Subscribing to Log Events**

   Clients can subscribe to the `log` event type to receive log messages as they are generated:

   ```json
   {
     "type": "EVENT_SUBSCRIBE",
     "eventType": "log"
   }
   ```

   When subscribed, clients will receive log messages with this format:

   ```json
   {
     "type": "EVENT_DATA",
     "eventType": "log",
     "data": {
       "level": 3,
       "levelName": "INFO",
       "module": "MyModule",
       "message": "This is an informational message",
       "timestamp": 1620000000123,
       "includeTimestamp": true,
       "includeLevelName": true,
       "includeModuleName": true
     }
   }
   ```

2. **Using the Logging Tool**

   Clients can use the `mcp.logging` tool to retrieve and modify logging configuration:

   ```json
   {
     "type": "TOOL_INVOKE",
     "tool": "mcp.logging",
     "parameters": {
       "action": "getConfig"
     }
   }
   ```

   And receive the current configuration:

   ```json
   {
     "type": "TOOL_RESULT",
     "success": true,
     "result": {
       "config": {
         "enabled": true,
         "maxLevel": 3,
         "outputs": 3,
         "includeTimestamp": true,
         "includeLevelName": true,
         "includeModuleName": true,
         "filterByModule": false,
         "allowedModules": []
       }
     }
   }
   ```

## Tool Actions

The `mcp.logging` tool supports the following actions:

| Action | Description | Parameters | Result |
|--------|-------------|------------|--------|
| `getConfig` | Retrieve current logging configuration | None | Complete configuration object |
| `setConfig` | Set complete logging configuration | `config` object | Success status |
| `enableLogging` | Enable log transmission via MCP | None | Previous state |
| `disableLogging` | Disable log transmission via MCP | None | Previous state |
| `setLevel` | Set maximum log level | `level` string | Previous level |
| `addModule` | Add module to allowed list | `moduleName` string | Success status |
| `removeModule` | Remove module from allowed list | `moduleName` string | Success status |
| `clearModules` | Clear module whitelist | None | Success status |
| `enableModuleFilter` | Enable module filtering | None | Previous state |
| `disableModuleFilter` | Disable module filtering | None | Previous state |

### Log Levels

The supported log levels (in order of severity):

1. `none` - No logging
2. `error` - Error conditions only
3. `warn` - Warning conditions
4. `info` - Informational messages
5. `debug` - Debug-level messages
6. `trace` - Trace-level messages (very verbose)

### Configuration Object

The complete configuration object contains:

```json
{
  "enabled": true,          // Whether logging is enabled
  "maxLevel": 3,            // Maximum log level (3=INFO)
  "outputs": 3,             // Bit mask of output destinations
  "includeTimestamp": true, // Include timestamp in logs
  "includeLevelName": true, // Include level name in logs
  "includeModuleName": true, // Include module name in logs
  "filterByModule": false,  // Whether to filter by module
  "allowedModules": []      // List of allowed module names
}
```

## Examples

### Setting Log Level

```json
{
  "type": "TOOL_INVOKE",
  "tool": "mcp.logging",
  "parameters": {
    "action": "setLevel",
    "level": "debug"
  }
}
```

### Enabling Module Filtering

```json
{
  "type": "TOOL_INVOKE",
  "tool": "mcp.logging",
  "parameters": {
    "action": "addModule",
    "moduleName": "NETWORK"
  }
}
```

```json
{
  "type": "TOOL_INVOKE",
  "tool": "mcp.logging",
  "parameters": {
    "action": "enableModuleFilter"
  }
}
```

### Setting Complete Configuration

```json
{
  "type": "TOOL_INVOKE",
  "tool": "mcp.logging",
  "parameters": {
    "action": "setConfig",
    "config": {
      "enabled": true,
      "maxLevel": 4,
      "outputs": 3,
      "includeTimestamp": true,
      "includeLevelName": true,
      "includeModuleName": true,
      "filterByModule": true,
      "allowedModules": ["MAIN", "NETWORK", "SENSOR"]
    }
  }
}
```

## Implementation Details

For full implementation details and more examples, see:

- `src/core/mcp/logging/mcp_logging.h` - Core logging interface
- `src/core/mcp/logging/mcp_logging_tool.h` - Tool interface
- `docs/examples/log_config_example.c` - Complete usage example
- `docs/examples/log_config_example.json` - Example tool invocations