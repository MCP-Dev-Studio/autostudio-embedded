/**
 * @file platform_compatibility.c
 * @brief Implementation of platform-specific compatibility functions
 */
#include "platform_compatibility.h"

// Platform-specific implementations
#if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
// =========================================================================
// HOST Platform Implementations
// =========================================================================

MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    MCP_ToolResult result;
    result.status = 0; // Success status for HOST platform
    result.resultJson = jsonResult ? strdup(jsonResult) : strdup("{}");
    
    return result;
}

MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage) {
    MCP_ToolResult result;
    result.status = status;
    
    // Format error JSON
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"error\":\"%s\",\"code\":%d}", 
             errorMessage ? errorMessage : "Unknown error", status);
    
    result.resultJson = strdup(buffer);
    
    return result;
}

#elif defined(MCP_OS_ARDUINO) || defined(MCP_PLATFORM_ARDUINO)
// =========================================================================
// Arduino Platform Implementations
// =========================================================================

MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    MCP_ToolResult result;
    result.status = 0; // Success status for Arduino platform
    result.resultJson = jsonResult ? strdup(jsonResult) : strdup("{}");
    
    return result;
}

MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage) {
    MCP_ToolResult result;
    result.status = status;
    
    // Format error JSON
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"error\":\"%s\",\"code\":%d}", 
             errorMessage ? errorMessage : "Unknown error", status);
    
    result.resultJson = strdup(buffer);
    
    return result;
}

#elif defined(MCP_OS_ESP32) || defined(MCP_PLATFORM_ESP32)
// =========================================================================
// ESP32 Platform Implementations
// =========================================================================

MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    MCP_ToolResult result;
    result.status = 0; // Success status for ESP32 platform
    result.resultJson = jsonResult ? strdup(jsonResult) : strdup("{}");
    result.resultData = NULL;
    result.resultDataSize = 0;
    
    return result;
}

MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage) {
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

#else
// For other platforms, functions are likely implemented elsewhere
// (e.g., in tool_registry.c), so no implementation is provided here
#endif

// Common implementations for all platforms
int MCP_DeviceInfoInit(void) {
    // Common device info initialization
    return 0;
}