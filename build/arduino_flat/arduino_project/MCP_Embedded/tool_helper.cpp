#include "arduino_compat.h"
/**
 * @file tool_helper.c
 * @brief Helper functions for the MCP tool system
 * 
 * This file contains unified implementations of common helper functions
 * for the MCP tool system, including result creation functions.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tool_info.h"

// Only compile these functions for C platforms (not Arduino C++)
#if !defined(MCP_PLATFORM_ARDUINO) && !defined(MCP_OS_ARDUINO)

/**
 * @brief Create a default success result
 * Unified implementation that works on all platforms
 *
 * @param jsonResult Optional JSON result string (can be NULL)
 * @return MCP_ToolResult Success result
 */
MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    MCP_ToolResult result;
    
    // Use numeric value from enum for status (consistent with int type)
    result.status = MCP_TOOL_RESULT_SUCCESS;
    
    // Use provided JSON or default to empty object
    if (jsonResult != NULL) {
        result.resultJson = jsonResult;
    } else {
        result.resultJson = "{}";
    }
    
    return result;
}

/**
 * @brief Create an error result with given status and message
 * Unified implementation that works on all platforms
 *
 * @param status Error status code (numeric value)
 * @param errorMessage Error message
 * @return MCP_ToolResult Error result
 */
MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage) {
    MCP_ToolResult result;
    
    // Set status directly from parameter
    result.status = status;
    
    // Create a JSON object with error message
    // Using static buffer for simplicity
    static char errorJson[256];
    snprintf(errorJson, sizeof(errorJson), "{\"error\": \"%s\"}", 
             errorMessage ? errorMessage : "Unknown error");
    
    result.resultJson = errorJson;
    
    return result;
}

#endif // !defined(MCP_PLATFORM_ARDUINO) && !defined(MCP_OS_ARDUINO)
