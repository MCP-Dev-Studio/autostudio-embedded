/**
 * @file arduino_compat.c
 * @brief Implementation of Arduino compatibility functions
 */
#include "arduino_compat.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

/**
 * @brief Get current time in milliseconds
 * 
 * @return uint64_t Current time in milliseconds
 */
uint64_t MCP_GetCurrentTimeMs(void) {
    // For this stub implementation, we'll use time(NULL) * 1000
    // In Arduino environments this would use millis()
    return (uint64_t)time(NULL) * 1000;
}

/**
 * @brief Get system time in milliseconds
 * 
 * @return uint32_t System time in milliseconds
 */
uint32_t MCP_SystemGetTimeMs(void) {
    // In Arduino environments this would use millis()
    return (uint32_t)time(NULL) * 1000;
}

/**
 * @brief Arduino-specific logging functions
 * These are just stubs - in an actual Arduino environment,
 * they would be replaced with Serial.print calls
 */
void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[ERROR] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[WARN] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[INFO] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[DEBUG] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_trace(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[TRACE] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

/**
 * @brief Create a success result with the given JSON
 * 
 * @param jsonResult Optional JSON result string (can be NULL)
 * @return MCP_ToolResult Success result
 */
MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    MCP_ToolResult result;
    result.status = 0; // Success
    result.resultJson = jsonResult ? jsonResult : "{}";
    return result;
}

/**
 * @brief Create an error result with status and message
 * 
 * @param status Error status
 * @param errorMessage Error message
 * @return MCP_ToolResult Error result
 */
MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage) {
    MCP_ToolResult result;
    result.status = status;
    
    // Format the error message as JSON
    static char errorJson[256];
    snprintf(errorJson, sizeof(errorJson), 
             "{\"error\":%d,\"message\":\"%s\"}", 
             status, errorMessage ? errorMessage : "Unknown error");
    
    result.resultJson = errorJson;
    return result;
}

/**
 * @brief Helper function to get string from content by field name for Arduino
 * Renamed to avoid naming conflicts with C++ code
 * 
 * @param content Content object
 * @param key Field name
 * @param value Pointer to store the string value
 * @return bool True if field exists, false otherwise
 */
bool MCP_ContentGetStringValue(const MCP_Content* content, const char* key, const char** value) {
    if (content == NULL || key == NULL || value == NULL) {
        return false;
    }
    
    // Simple stub implementation for Arduino
    // In a real implementation, this would parse JSON
    *value = ""; // Default empty string
    return true;
}

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)