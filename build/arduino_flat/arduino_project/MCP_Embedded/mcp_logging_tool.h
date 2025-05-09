#ifndef MCP_LOGGING_TOOL_H
#define MCP_LOGGING_TOOL_H

#include "protocol_handler.h"
#include "server.h"
#include "mcp_logging.h"
#include "tool_info.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log configuration tool name
 */
#define MCP_LOGGING_TOOL_NAME "mcp.logging"

/**
 * @brief Initialize logging tool
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingToolInit(void);

/**
 * @brief Deinitialize logging tool
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingToolDeinit(void);

/**
 * @brief Handle tool invocation
 * 
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @param params Tool parameters
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingToolInvoke(const char* sessionId, const char* operationId, 
                        const MCP_Content* params);

/**
 * @brief Register logging tool with the tool registry
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingToolRegister(void);

#if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
/**
 * @brief HOST-specific stub for tool invocation
 * 
 * This function is a simplified stub for the HOST platform to avoid
 * dealing with complex or incomplete types.
 * 
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @param params Tool parameters (void* to avoid type conflicts)
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingToolInvokeHost(const char* sessionId, const char* operationId, void* params);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MCP_LOGGING_TOOL_H */