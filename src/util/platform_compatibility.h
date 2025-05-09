/**
 * @file platform_compatibility.h
 * @brief Platform-specific type definitions and compatibility functions
 * 
 * This file provides platform-specific type definitions and functions to ensure
 * compatibility across different platforms (HOST, Arduino, ESP32, etc.)
 */
#ifndef PLATFORM_COMPATIBILITY_H
#define PLATFORM_COMPATIBILITY_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Include Arduino compatibility header if on Arduino platform
#if defined(MCP_OS_ARDUINO) || defined(MCP_PLATFORM_ARDUINO)
#include <stdint.h>
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of MCP_ContentType for content functions
typedef enum {
    MCP_CONTENT_TYPE_UNKNOWN,
    MCP_CONTENT_TYPE_TEXT,
    MCP_CONTENT_TYPE_JSON,
    MCP_CONTENT_TYPE_BINARY,
    MCP_CONTENT_TYPE_IMAGE,
    MCP_CONTENT_TYPE_AUDIO,
    MCP_CONTENT_TYPE_VIDEO
} MCP_ContentType;

// Common functions needed across all platforms
const char* MCP_ContentGetDefaultMediaType(MCP_ContentType type);

// =========================================================================
// Unified platform definitions (common across all platforms)
// =========================================================================

/**
 * @brief Tool result structure - unified for all platforms
 */
#ifndef MCP_TOOL_RESULT_DEFINED
#define MCP_TOOL_RESULT_DEFINED

// Check for Arduino platform
#if defined(MCP_OS_ARDUINO) || defined(MCP_PLATFORM_ARDUINO)
// For Arduino, define tool result structure directly
typedef struct {
    int status;
    const char* resultJson;
} MCP_ToolResult;
#elif defined(MCP_OS_ESP32) || defined(MCP_PLATFORM_ESP32)
// ESP32 specific tool result structure with additional fields
typedef struct {
    int status;
    const char* resultJson;
    void* resultData;        // ESP32 specific
    size_t resultDataSize;   // ESP32 specific
} MCP_ToolResult;
#else
// Standard tool result for all other platforms
typedef struct {
    int status;
    const char* resultJson;
} MCP_ToolResult;
#endif

#endif // MCP_TOOL_RESULT_DEFINED

// Common tool result functions used across all platforms
MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult);
MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage);

// Common forward declarations for all platforms
int MCP_DeviceInfoInit(void);

// Time functions used across all platforms
uint64_t MCP_GetCurrentTimeMs(void);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_COMPATIBILITY_H