#ifndef MCP_SERVER_DEFS_H
#define MCP_SERVER_DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Include platform compatibility header if building for Arduino with C++ fixes
#if defined(MCP_PLATFORM_ARDUINO) && defined(MCP_CPP_FIXES)
#include "platform_compatibility.h"
#endif

/**
 * @file server_defs.h
 * @brief MCP server structures and type definitions
 * 
 * This file provides forward declarations for server-related functionality,
 * ensuring compatibility across platforms without redefinition issues.
 */

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for MCP_Content to avoid circular dependencies
#if !defined(MCP_PLATFORM_ARDUINO) || !defined(MCP_CPP_FIXES)
#if !defined(MCP_CONTENT_DEFINED) && !defined(MCP_CONTENT_FORWARD_DECLARED)
#define MCP_CONTENT_FORWARD_DECLARED
struct MCP_Content;
#endif
#endif

// Forward declare the server types for all platforms
#if !defined(MCP_SERVER_TYPES_DEFINED)
struct MCP_ServerTransport {
    int type;
    void* config;
    void* userData;
};

struct MCP_Server {
    struct MCP_ServerTransport* transport;
    void* config;
    uint32_t startTime;
    bool initialized;
};
#endif

// Forward declarations for server functions for all platforms
struct MCP_Server* MCP_GetServer(void);
int MCP_SendToolResult(struct MCP_ServerTransport* transport, const char* sessionId, 
                     const char* operationId, bool success, const struct MCP_Content* result);

// Forward declarations for content-related functions used by server
// These may be defined in content.h or platform_compatibility.h, but we declare them here
// to ensure availability without circular dependencies
#if !defined(MCP_PLATFORM_ARDUINO) || !defined(MCP_CPP_FIXES)
struct MCP_Content* MCP_ContentCreateObject(void);
bool MCP_ContentAddString(struct MCP_Content* content, const char* key, const char* value);
bool MCP_ContentAddBoolean(struct MCP_Content* content, const char* key, bool value);
struct MCP_Content* MCP_ContentCreate(int type, const uint8_t* data, size_t size, const char* mediaType);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MCP_SERVER_DEFS_H */