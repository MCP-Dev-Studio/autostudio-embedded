/**
 * @file build_fix.c
 * @brief Implementation of helper functions to resolve duplicate symbol issues
 */
#include "build_fix.h"
#include "core/tool_system/tool_registry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Implementation of MCP_ToolCreateSuccessResult
MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    MCP_ToolResult result;
    result.status = MCP_TOOL_RESULT_SUCCESS;
    result.resultJson = jsonResult ? strdup(jsonResult) : strdup("{}");
    result.resultData = NULL;
    result.resultDataSize = 0;
    return result;
}

// Implementation of MCP_ToolCreateErrorResult
MCP_ToolResult MCP_ToolCreateErrorResult(MCP_ToolResultStatus status, const char* errorMessage) {
    MCP_ToolResult result;
    result.status = status;
    
    // Format error JSON
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"error\":\"%s\",\"code\":%d}", 
             errorMessage ? errorMessage : "Unknown error", status);
    
    result.resultJson = strdup(buffer);
    result.resultData = NULL;
    result.resultDataSize = 0;
    return result;
}