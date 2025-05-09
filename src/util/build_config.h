/**
 * @file build_config.h
 * @brief Controls which files provide implementations for common functions
 * 
 * This file helps prevent duplicate symbol errors during linking by ensuring
 * that implementation functions are only defined in one compilation unit.
 */

#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Only include tool_registry.h for non-HOST platforms
#if !defined(MCP_OS_HOST) && !defined(MCP_PLATFORM_HOST)
#include "core/tool_system/tool_registry.h"
#endif

/**
 * Define this in the one file where MCP_Tool functions should be implemented
 * (tool_registry.c). In all other files that need these functions, include
 * this header without defining MCP_IMPL_TOOL_FUNCTIONS.
 */
// #define MCP_IMPL_TOOL_FUNCTIONS

/**
 * Define this in the one file where JSON functions should be implemented
 * (json_parser.c). In all other files that need these functions, include
 * this header without defining MCP_IMPL_JSON_FUNCTIONS.
 */
// #define MCP_IMPL_JSON_FUNCTIONS

/**
 * Define this in the one file where device info functions should be implemented
 * (device_info.c). In all other files that need these functions, include
 * this header without defining MCP_IMPL_DEVICE_INFO_FUNCTIONS.
 */
// #define MCP_IMPL_DEVICE_INFO_FUNCTIONS

// Tool functions declarations
#ifndef MCP_TOOL_RESULT_FUNCTIONS_DECLARED
#define MCP_TOOL_RESULT_FUNCTIONS_DECLARED
#if !defined(MCP_OS_HOST) && !defined(MCP_PLATFORM_HOST)
// Regular platform declarations
extern MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult);
extern MCP_ToolResult MCP_ToolCreateErrorResult(MCP_ToolResultStatus status, const char* errorMessage);
#else
// HOST platform declarations with simplified types
struct MCP_ToolResult {
    int status;
    const char* resultJson;
    void* resultData;
    size_t resultDataSize;
};
extern struct MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult);
extern struct MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage);
#endif
#endif

// JSON functions declarations
#ifndef JSON_FUNCTIONS_DECLARED
#define JSON_FUNCTIONS_DECLARED
extern char* json_get_string_field(const char* json, const char* field);
extern bool json_validate_schema(const char* json, const char* schema);
extern void* json_get_array_field(const char* json, const char* field);
extern void* json_get_object_field(const char* json, const char* field);
extern size_t json_array_length(const void* array);
extern void* json_array_get_object(const void* array, size_t index);
extern bool json_get_bool_field(const char* json, const char* field, bool defaultValue);
#endif

// Device info functions declarations
#ifndef MCP_DEVICE_INFO_FUNCTIONS_DECLARED
#define MCP_DEVICE_INFO_FUNCTIONS_DECLARED
extern int MCP_DeviceInfoInit(void);
#endif

// Implementations - will only be included in the file that defines the corresponding macro
#ifdef MCP_IMPL_TOOL_FUNCTIONS
#if !defined(MCP_OS_HOST) && !defined(MCP_PLATFORM_HOST)
// Regular platform implementations
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
#else
// HOST platform implementations with simplified types
struct MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    struct MCP_ToolResult result;
    result.status = 0; // Success

    if (jsonResult != NULL) {
        result.resultJson = strdup(jsonResult);
    } else {
        result.resultJson = strdup("{}");
    }

    result.resultData = NULL;
    result.resultDataSize = 0;

    return result;
}

struct MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage) {
    struct MCP_ToolResult result;
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
#endif
#endif // MCP_IMPL_TOOL_FUNCTIONS

#endif // BUILD_CONFIG_H