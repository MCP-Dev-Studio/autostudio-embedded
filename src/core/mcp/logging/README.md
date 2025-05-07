# MCP Logging System

This module provides a bridge between the embedded system's logging capabilities and the MCP protocol, allowing log messages to be sent to clients over MCP connections.

## Features

- Send log messages to MCP clients as events
- Control log level and filtering on both server and client sides
- Client configuration for log output destination (console, file, memory buffer, or custom handler)
- Support for log rotation for file output
- Colorized console output
- In-memory circular buffer for log storage
- Timestamp, log level, and module name formatting

## Usage

### Server-side Integration

1. Initialize the logging bridge on the server:

```c
#include "mcp_logging.h"

// Initialize with server instance and maximum log level
MCP_LoggingInit(serverInstance, LOG_LEVEL_DEBUG);

// Enable logging to MCP clients
MCP_LoggingEnable(true);
```

2. Use the standard logging system to output logs:

```c
#include "logging.h"

LOG_INFO("MyModule", "This is an informational message");
LOG_ERROR("MyModule", "An error occurred: %d", errorCode);
```

### Client-side Integration

1. Initialize the client logging configuration:

```c
#include "mcp_logging_client.h"

// Initialize with default configuration (console output)
MCP_LogClientInit(NULL);

// Enable client-side logging
MCP_LogClientEnable(true);
```

2. Configure log output destination:

```c
// Output to file
MCP_LogClientConfig config = MCP_LogClientGetDefaultConfig();
config.output = MCP_LOG_OUTPUT_FILE;
config.filePath = "/path/to/log/file.log";
MCP_LogClientSetConfig(&config);

// Or change just the output destination
MCP_LogClientSetOutput(MCP_LOG_OUTPUT_CONSOLE);
```

3. Set log level filter:

```c
// Show only errors and warnings
MCP_LogClientSetLevel(LOG_LEVEL_WARN);
```

4. Use custom log handler:

```c
void myLogHandler(LogLevel level, const char* module, const char* message, uint32_t timestamp) {
    // Custom log handling code
}

MCP_LogClientSetCustomHandler(myLogHandler);
```

## Architecture

This logging system connects the embedded system's logging to the MCP event system:

1. Server-side:
   - Hooks into the existing logging system via a callback
   - Converts log messages to MCP events
   - Sends events to subscribed clients

2. Client-side:
   - Handles log event subscription
   - Processes incoming log events
   - Outputs logs based on client configuration

## References

- [MCP Protocol Specification](https://github.com/anthropics/mcp)
- [Event System Documentation](https://docs.mcp-protocol.org/events)