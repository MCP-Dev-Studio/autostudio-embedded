#ifndef MCP_SESSION_H
#define MCP_SESSION_H

#include "server.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief MCP session information structure
 */
typedef struct {
    char* id;                       // Session ID
    MCP_SessionState state;         // Session state
    uint32_t creationTime;          // Creation timestamp
    uint32_t lastActivityTime;      // Last activity timestamp
    uint16_t operationCount;        // Total operations in session
    uint16_t activeOperations;      // Active operations
    char* clientInfo;               // Client information
    MCP_ServerTransport* transport; // Associated transport
} MCP_SessionInfo;

/**
 * @brief MCP operation information structure
 */
typedef struct {
    char* id;                    // Operation ID
    char* sessionId;             // Associated session ID
    MCP_OperationType type;      // Operation type
    uint32_t creationTime;       // Creation timestamp
    uint32_t completionTime;     // Completion timestamp (0 if not completed)
    bool completed;              // Completion flag
    bool success;                // Success flag (if completed)
    char* resourcePath;          // Resource path (for resource operations)
    char* eventType;             // Event type (for event operations)
    char* toolName;              // Tool name (for tool operations)
} MCP_OperationInfo;

/**
 * @brief Create a new session
 * 
 * @param transport Transport interface
 * @param clientInfo Client information string
 * @return char* Session ID or NULL on failure (caller must NOT free)
 */
char* MCP_SessionCreate(MCP_ServerTransport* transport, const char* clientInfo);

/**
 * @brief Find a session by ID
 * 
 * @param sessionId Session ID
 * @return MCP_SessionInfo* Session information or NULL if not found
 */
const MCP_SessionInfo* MCP_SessionFind(const char* sessionId);

/**
 * @brief Close a session
 * 
 * @param sessionId Session ID
 * @param reason Close reason string
 * @return int 0 on success, negative error code on failure
 */
int MCP_SessionClose(const char* sessionId, const char* reason);

/**
 * @brief Update session activity timestamp
 * 
 * @param sessionId Session ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_SessionUpdateActivity(const char* sessionId);

/**
 * @brief Create a new operation in a session
 * 
 * @param sessionId Session ID
 * @param type Operation type
 * @return char* Operation ID or NULL on failure (caller must NOT free)
 */
char* MCP_SessionCreateOperation(const char* sessionId, MCP_OperationType type);

/**
 * @brief Find an operation by ID
 * 
 * @param operationId Operation ID
 * @return MCP_OperationInfo* Operation information or NULL if not found
 */
const MCP_OperationInfo* MCP_SessionFindOperation(const char* operationId);

/**
 * @brief Complete an operation
 * 
 * @param operationId Operation ID
 * @param success Success flag
 * @param resultData Result data
 * @param resultDataLength Length of result data
 * @return int 0 on success, negative error code on failure
 */
int MCP_SessionCompleteOperation(const char* operationId, bool success, 
                               const uint8_t* resultData, size_t resultDataLength);

/**
 * @brief Cancel an operation
 * 
 * @param operationId Operation ID
 * @param reason Cancel reason string
 * @return int 0 on success, negative error code on failure
 */
int MCP_SessionCancelOperation(const char* operationId, const char* reason);

/**
 * @brief Set operation resource path
 * 
 * @param operationId Operation ID
 * @param resourcePath Resource path
 * @return int 0 on success, negative error code on failure
 */
int MCP_SessionSetOperationResource(const char* operationId, const char* resourcePath);

/**
 * @brief Set operation event type
 * 
 * @param operationId Operation ID
 * @param eventType Event type
 * @return int 0 on success, negative error code on failure
 */
int MCP_SessionSetOperationEvent(const char* operationId, const char* eventType);

/**
 * @brief Set operation tool name
 * 
 * @param operationId Operation ID
 * @param toolName Tool name
 * @return int 0 on success, negative error code on failure
 */
int MCP_SessionSetOperationTool(const char* operationId, const char* toolName);

/**
 * @brief Process session timeouts
 * 
 * @param currentTimeMs Current system time in milliseconds
 * @param sessionTimeout Session timeout in milliseconds
 * @return int Number of sessions closed due to timeout
 */
int MCP_SessionProcessTimeouts(uint32_t currentTimeMs, uint32_t sessionTimeout);

/**
 * @brief Get session list as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_SessionGetList(char* buffer, size_t bufferSize);

/**
 * @brief Get operation list for a session as JSON
 * 
 * @param sessionId Session ID
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_SessionGetOperationList(const char* sessionId, char* buffer, size_t bufferSize);

#endif /* MCP_SESSION_H */