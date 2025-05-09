/**
 * @file tool_helper_arduino.cpp
 * @brief Helper functions for the MCP tool system - Arduino compatible version
 * 
 * This file contains the same unified implementations as tool_helper.c,
 * but compiled as C++ for Arduino compatibility.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tool_info.h"

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

/**
 * @brief Create a default success result
 * Identical implementation with tool_helper.c but compiled as C++
 *
 * @param jsonResult Optional JSON result string (can be NULL)
 * @return MCP_ToolResult Success result
 */
MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    MCP_ToolResult result;
    
    // Use numeric value from enum for status
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
 * Identical implementation with tool_helper.c but compiled as C++
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
    static char errorJson[256];
    snprintf(errorJson, sizeof(errorJson), "{\"error\": \"%s\"}", 
             errorMessage ? errorMessage : "Unknown error");
    
    result.resultJson = errorJson;
    
    return result;
}

#endif // MCP_PLATFORM_ARDUINO || MCP_OS_ARDUINO