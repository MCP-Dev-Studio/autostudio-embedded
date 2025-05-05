#include "tool_registry.h"
#include <stdlib.h>
#include <string.h>

// External JSON utility functions (minimal implementation)
extern char* json_get_string_field(const char* json, const char* field);
extern bool json_validate_schema(const char* json, const char* schema);

// Tool entry structure
typedef struct {
    char* name;
    MCP_ToolHandler handler;
    char* schema;
    bool active;
} ToolEntry;

// Internal state
static ToolEntry* s_tools = NULL;
static int s_maxTools = 0;
static int s_toolCount = 0;
static bool s_initialized = false;

int MCP_ToolRegistryInit(int maxTools) {
    if (s_initialized) {
        return -1;  // Already initialized
    }
    
    // Allocate tool array
    s_tools = (ToolEntry*)calloc(maxTools, sizeof(ToolEntry));
    if (s_tools == NULL) {
        return -2;  // Memory allocation failed
    }
    
    s_maxTools = maxTools;
    s_toolCount = 0;
    s_initialized = true;
    
    return 0;
}

int MCP_ToolRegister(const char* name, MCP_ToolHandler handler, const char* schema) {
    if (!s_initialized || name == NULL || handler == NULL) {
        return -1;
    }
    
    // Check if tool already exists
    int existingIndex = MCP_ToolFind(name);
    if (existingIndex >= 0) {
        return -2;  // Tool already registered
    }
    
    // Check if we have space for a new tool
    if (s_toolCount >= s_maxTools) {
        return -3;  // No space for new tool
    }
    
    // Find free slot
    int i;
    for (i = 0; i < s_maxTools; i++) {
        if (!s_tools[i].active) {
            break;
        }
    }
    
    if (i >= s_maxTools) {
        return -4;  // No free slot found
    }
    
    // Allocate and copy tool information
    s_tools[i].name = strdup(name);
    if (s_tools[i].name == NULL) {
        return -5;  // Memory allocation failed
    }
    
    s_tools[i].handler = handler;
    
    if (schema != NULL) {
        s_tools[i].schema = strdup(schema);
        if (s_tools[i].schema == NULL) {
            free(s_tools[i].name);
            s_tools[i].name = NULL;
            return -6;  // Memory allocation failed
        }
    } else {
        s_tools[i].schema = NULL;
    }
    
    s_tools[i].active = true;
    s_toolCount++;
    
    return 0;
}

int MCP_ToolFind(const char* name) {
    if (!s_initialized || name == NULL) {
        return -1;
    }
    
    for (int i = 0; i < s_maxTools; i++) {
        if (s_tools[i].active && strcmp(s_tools[i].name, name) == 0) {
            return i;
        }
    }
    
    return -2;  // Tool not found
}

MCP_ToolResult MCP_ToolExecute(const char* json, size_t length) {
    if (!s_initialized || json == NULL || length == 0) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Invalid input");
    }
    
    // Extract tool name from JSON
    char* toolName = json_get_string_field(json, "tool");
    if (toolName == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing tool name");
    }
    
    // Find tool
    int toolIndex = MCP_ToolFind(toolName);
    if (toolIndex < 0) {
        free(toolName);
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_NOT_FOUND, "Tool not found");
    }
    
    free(toolName);
    
    // Validate parameters against schema if schema exists
    if (s_tools[toolIndex].schema != NULL) {
        if (!json_validate_schema(json, s_tools[toolIndex].schema)) {
            return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Invalid parameters");
        }
    }
    
    // Execute tool handler
    MCP_ToolResult result = s_tools[toolIndex].handler(json, length);
    
    return result;
}

const char* MCP_ToolGetSchema(const char* name) {
    if (!s_initialized || name == NULL) {
        return NULL;
    }
    
    int toolIndex = MCP_ToolFind(name);
    if (toolIndex < 0) {
        return NULL;
    }
    
    return s_tools[toolIndex].schema;
}

int MCP_ToolGetList(char* buffer, size_t bufferSize) {
    if (!s_initialized || buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // Start JSON array
    int offset = 0;
    offset += snprintf(buffer + offset, bufferSize - offset, "[");
    
    // Add tools
    bool first = true;
    for (int i = 0; i < s_maxTools; i++) {
        if (s_tools[i].active) {
            // Add comma if not first tool
            if (!first) {
                offset += snprintf(buffer + offset, bufferSize - offset, ",");
            }
            first = false;
            
            // Add tool info
            offset += snprintf(buffer + offset, bufferSize - offset, 
                            "{\"name\":\"%s\",\"hasSchema\":%s}", 
                            s_tools[i].name, 
                            s_tools[i].schema != NULL ? "true" : "false");
            
            // Check if we're about to overflow
            if ((size_t)offset >= bufferSize - 2) {
                return -2;  // Buffer too small
            }
        }
    }
    
    // End JSON array
    offset += snprintf(buffer + offset, bufferSize - offset, "]");
    
    return offset;
}

MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    MCP_ToolResult result;
    result.status = MCP_TOOL_RESULT_SUCCESS;
    
    if (jsonResult != NULL) {
        result.resultJson = strdup(jsonResult);
    } else {
        result.resultJson = strdup("{}");
    }
    
    result.resultData = NULL;
    result.resultDataSize = 0;
    
    return result;
}

MCP_ToolResult MCP_ToolCreateErrorResult(MCP_ToolResultStatus status, const char* errorMessage) {
    MCP_ToolResult result;
    result.status = status;
    
    // Format error JSON
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
             "{\"error\":true,\"code\":%d,\"message\":\"%s\"}", 
             status, errorMessage ? errorMessage : "Unknown error");
    
    result.resultJson = strdup(buffer);
    result.resultData = NULL;
    result.resultDataSize = 0;
    
    return result;
}