# How to Fix the MCP_ToolInfo Inconsistency Issue

## Problem Summary

The core issue is that the MCP_ToolInfo structure is being defined differently across different platform implementations (host vs ESP32 vs Arduino), leading to compilation errors when trying to use `MCP_ToolRegister` in cross-platform code.

## Solution Approach

1. Create a unified `tool_info.h` file that defines the MCP_ToolInfo structure consistently
2. Update `tool_registry.h` to include the new unified header
3. Add backward compatibility through a legacy function `MCP_ToolRegister_Legacy` 
4. Update calls to the old function in various files

## Implementation Steps

### 1. Create/Update the tool_info.h file

```c
// File: src/core/tool_system/tool_info.h
#ifndef MCP_TOOL_INFO_H
#define MCP_TOOL_INFO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../mcp/content.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tool information structure used for registration
 */
typedef struct {
    const char* name;                /**< Tool name */
    const char* description;         /**< Tool description */
    const char* schemaJson;          /**< JSON schema for tool parameters */
    int (*init)(void);               /**< Initialization function */
    int (*deinit)(void);             /**< Deinitialization function */
    int (*invoke)(const char* sessionId, const char* operationId, const MCP_Content* params); /**< Tool invocation handler */
} MCP_ToolInfo;

#ifdef __cplusplus
}
#endif

#endif /* MCP_TOOL_INFO_H */
```

### 2. Update tool_registry.h to include legacy function

```c
// Add this to src/core/tool_system/tool_registry.h
/**
 * @brief Register a tool in the registry (legacy version)
 *
 * @param name Tool name
 * @param handler Tool handler function
 * @param schema JSON schema for tool parameters
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolRegister_Legacy(const char* name, MCP_ToolHandler handler, const char* schema);
```

### 3. Implement the legacy function in tool_registry.c

```c
// Add this to src/core/tool_system/tool_registry.c
/**
 * @brief Register a tool in the registry (legacy version)
 *
 * @param name Tool name
 * @param handler Tool handler function
 * @param schema JSON schema for tool parameters
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolRegister_Legacy(const char* name, MCP_ToolHandler handler, const char* schema) {
    MCP_ToolInfo info;
    memset(&info, 0, sizeof(info));
    
    info.name = name;
    info.description = "Legacy tool";
    info.schemaJson = schema;
    
    // Create a wrapper for the legacy handler
    // This is a simplified implementation - in a complete solution,
    // you would need a more robust approach to handle the callback
    static int invoke_wrapper(const char* sessionId, const char* operationId, const MCP_Content* params) {
        // Convert params to JSON string
        char paramsJson[4096] = "{}";
        // Call the legacy handler
        MCP_ToolResult result = handler(paramsJson, strlen(paramsJson));
        // Process the result
        return 0;
    }
    
    info.invoke = invoke_wrapper;
    
    return MCP_ToolRegister(&info);
}
```

### 4. Update all MCP_ToolRegister calls to use the legacy version

In your build script, add a step to automatically update all old-style calls:

```bash
# Update MCP_ToolRegister calls to use MCP_ToolRegister_Legacy
for file in "$OUTPUT_DIR"/*.c; do
    if [ -f "$file" ]; then
        # Check if file contains MCP_ToolRegister with old signature
        if grep -q "MCP_ToolRegister(.*,.*,.*)" "$file"; then
            # Handle macOS vs Linux sed differences
            if [[ "$OSTYPE" == "darwin"* ]]; then
                # For macOS
                sed -i '' -E 's/MCP_ToolRegister\("([^"]+)", ([^,]+), "([^"]+)"\)/MCP_ToolRegister_Legacy("\1", \2, "\3")/g' "$file"
            else
                # For Linux
                sed -i -E 's/MCP_ToolRegister\("([^"]+)", ([^,]+), "([^"]+)"\)/MCP_ToolRegister_Legacy("\1", \2, "\3")/g' "$file"
            fi
        fi
    fi
done
```

### 5. Fix bytecode_config.c to work with the MCP_Server structure

Add the following forward declarations at the top of bytecode_config.c after the includes:

```c
// Forward declarations
typedef enum {
    MCP_TRANSPORT_UART,
    MCP_TRANSPORT_TCP,
    MCP_TRANSPORT_BLUETOOTH,
    MCP_TRANSPORT_USB,
    MCP_TRANSPORT_ETHERNET,
    MCP_TRANSPORT_CUSTOM
} MCP_ServerTransportType;

typedef struct {
    MCP_ServerTransportType type;
    int (*read)(uint8_t* buffer, size_t maxLength, uint32_t timeout);
    int (*write)(const uint8_t* data, size_t length);
    int (*close)(void);
    uint32_t (*getStatus)(void);
    void* config;
    void* userData;
} MCP_ServerTransport;

typedef struct {
    MCP_ServerTransport* transport;
    // Other fields not used in bytecode_config.c
} MCP_Server;

// Function prototypes
MCP_Server* MCP_GetServer(void);
int MCP_SendToolResult(MCP_ServerTransport* transport, const char* sessionId, const char* operationId, bool success, const MCP_Content* result);
```

## Conclusion

This approach provides a consistent definition of the MCP_ToolInfo structure and a backward-compatible way to handle existing code that uses the older function signature. This should resolve the cross-platform compilation issues when building for both host and embedded platforms.