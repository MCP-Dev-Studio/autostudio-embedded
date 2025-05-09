/**
 * @file tool_registry.h
 * @brief Tool registry for MCP system
 */
#ifndef MCP_TOOL_REGISTRY_H
#define MCP_TOOL_REGISTRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "tool_info.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define platform-specific elements
// Forward declarations for MCP_Content
struct MCP_Content;

/**
 * @brief Initialize the tool registry
 *
 * @param maxTools Maximum number of tools to register
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolRegistryInit(int maxTools);

/**
 * @brief Register a tool in the registry
 *
 * @param info Pointer to the tool information structure
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolRegister(const void* info);

/**
 * @brief Register a legacy tool (compatibility function)
 * 
 * @param name Tool name
 * @param handler Tool handler function
 * @param schema JSON schema for tool parameters
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolRegister_Legacy(const char* name, void* handler, const char* schema);

/**
 * @brief Register a dynamic tool from JSON definition
 *
 * @param json JSON string with tool definition
 * @param length Length of JSON string
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolRegisterDynamic(const char* json, size_t length);

/**
 * @brief Find a tool by name
 *
 * @param name Tool name to find
 * @return int Tool index or negative error code if not found
 */
int MCP_ToolFind(const char* name);

/**
 * @brief Get tool definition by name
 *
 * @param name Tool name to find
 * @return Tool definition or NULL if not found
 */
const void* MCP_ToolGetDefinition(const char* name);

/**
 * @brief Execute a tool with JSON parameters
 *
 * @param json JSON string with format {"tool": "toolName", "params": {...}}
 * @param length Length of JSON string
 * @return MCP_ToolResult Result of tool execution
 */
MCP_ToolResult MCP_ToolExecute(const char* json, size_t length);

/**
 * @brief Get the JSON schema for a tool
 *
 * @param name Tool name
 * @return const char* JSON schema string or NULL if not found
 */
const char* MCP_ToolGetSchema(const char* name);

/**
 * @brief Get the list of registered tools as JSON
 *
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_ToolGetList(char* buffer, size_t bufferSize);

/**
 * @brief Save a dynamic tool to persistent storage
 *
 * @param name Tool name
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolSaveDynamic(const char* name);

/**
 * @brief Load a dynamic tool from persistent storage
 *
 * @param name Tool name
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolLoadDynamic(const char* name);

/**
 * @brief Load all dynamic tools from persistent storage
 *
 * @return int Number of tools loaded or negative error code
 */
int MCP_ToolLoadAllDynamic(void);

/**
 * @brief Create a success result with the given JSON
 * Note: Function is declared in platform_compatibility.h for cross-platform consistency
 *
 * @param jsonResult Optional JSON result string (can be NULL)
 * @return MCP_ToolResult Success result
 */
// MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult);

/**
 * @brief Create an error result with status and message
 * Note: Function is declared in platform_compatibility.h for cross-platform consistency
 *
 * @param status Error status
 * @param errorMessage Error message
 * @return MCP_ToolResult Error result
 */
// MCP_ToolResult MCP_ToolCreateErrorResult(int status, const char* errorMessage);

/**
 * @brief Stub tool invocation handler
 */
int MCP_StubToolInvoke(const char* sessionId, const char* operationId, const struct MCP_Content* params);

/**
 * @brief Initialize the tool handler
 */
int MCP_ToolHandlerInit(void);

/**
 * @brief Handle tool invocation
 */
int MCP_ToolHandleInvocation(const char* sessionId, const char* operationId,
                         const uint8_t* contentJson, size_t contentLength);

/**
 * @brief Handle getting tool list
 */
int MCP_ToolHandleGetList(const char* sessionId, const char* operationId);

/**
 * @brief Handle getting tool schema
 */
int MCP_ToolHandleGetSchema(const char* sessionId, const char* operationId, const char* toolName);

#ifdef __cplusplus
}
#endif

#endif /* MCP_TOOL_REGISTRY_H */