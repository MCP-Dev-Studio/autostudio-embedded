#ifndef MCP_SERVER_UTILS_H
#define MCP_SERVER_UTILS_H

#include "server.h"
#include "content.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCP server structure
 */
typedef struct {
    MCP_ServerConfig config;           // Server configuration
    MCP_ServerTransport* transport;    // Server transport
    uint32_t startTime;                // Server start time (ms)
    bool initialized;                  // Whether server is initialized
} MCP_Server;

/**
 * @brief Get global MCP server instance
 * 
 * @return MCP_Server* Server instance or NULL if not initialized
 */
MCP_Server* MCP_GetServer(void);

/**
 * @brief Broadcast an event to all subscribed sessions
 * 
 * This function sends an event to all sessions that have subscribed
 * to the specified event type.
 * 
 * @param server Server instance
 * @param eventType Event type
 * @param content Event content
 * @return int Number of sessions event was sent to or negative error code
 */
int MCP_ServerBroadcastEvent(MCP_Server* server, const char* eventType, MCP_Content* content);

/**
 * @brief Get current time in milliseconds
 * 
 * @return uint32_t Current time in milliseconds
 */
uint32_t MCP_GetCurrentTimeMs(void);

#ifdef __cplusplus
}
#endif

#endif /* MCP_SERVER_UTILS_H */