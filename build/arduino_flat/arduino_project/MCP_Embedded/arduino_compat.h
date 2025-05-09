/**
 * @file arduino_compat.h
 * @brief Arduino compatibility layer for MCP system
 * 
 * This file provides compatibility definitions and functions for Arduino platforms.
 * It centralizes all platform-specific adaptations needed for Arduino builds.
 */
#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

// Include platform_compatibility.h first to get MCP_ContentType definition
#include "platform_compatibility.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extended MCP_Content structure for Arduino compatibility
 * 
 * This structure includes an additional resultJson field for compatibility
 * with Arduino builds, while maintaining the same memory layout as the
 * base structure for other platforms.
 */
typedef struct MCP_Content {
    MCP_ContentType type;       // Content type
    uint8_t* data;              // Content data
    size_t size;                // Content size
    char* mediaType;            // Media type string (e.g., "application/json")
    bool ownsData;              // Whether the structure owns the data
    const char* resultJson;     // Result as JSON string (Arduino compatibility)
} MCP_Content;

// MCP_ToolResult is defined in platform_compatibility.h
// This comment is left here to indicate that we consciously removed the duplicate definition

// Function declarations for time functions
uint64_t MCP_GetCurrentTimeMs(void);
uint32_t MCP_SystemGetTimeMs(void);

// Logging functions for Arduino
void log_error(const char* format, ...);
void log_warn(const char* format, ...);
void log_info(const char* format, ...);
void log_debug(const char* format, ...);
void log_trace(const char* format, ...);

// String handling functions for Arduino
bool MCP_ContentGetStringValue(const MCP_Content* content, const char* key, const char** value);

// Tool result creation functions
MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult);
MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage);

#ifdef __cplusplus
}
#endif

#endif /* ARDUINO_COMPAT_H */