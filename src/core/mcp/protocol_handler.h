#ifndef MCP_PROTOCOL_HANDLER_H
#define MCP_PROTOCOL_HANDLER_H

#include "content.h"
#include "server.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief MCP message type
 */
typedef enum {
    MCP_MESSAGE_TYPE_HELLO,
    MCP_MESSAGE_TYPE_WELCOME,
    MCP_MESSAGE_TYPE_ERROR,
    MCP_MESSAGE_TYPE_CONTENT_REQUEST,
    MCP_MESSAGE_TYPE_CONTENT_RESPONSE,
    MCP_MESSAGE_TYPE_TOOL_INVOKE,
    MCP_MESSAGE_TYPE_TOOL_RESULT,
    MCP_MESSAGE_TYPE_EVENT_SUBSCRIBE,
    MCP_MESSAGE_TYPE_EVENT_DATA,
    MCP_MESSAGE_TYPE_EVENT_UNSUBSCRIBE,
    MCP_MESSAGE_TYPE_RESOURCE_GET,
    MCP_MESSAGE_TYPE_RESOURCE_SET,
    MCP_MESSAGE_TYPE_RESOURCE_DATA,
    MCP_MESSAGE_TYPE_GOODBYE,
    MCP_MESSAGE_TYPE_PING,
    MCP_MESSAGE_TYPE_PONG
} MCP_MessageType;

/**
 * @brief MCP message structure
 */
typedef struct {
    MCP_MessageType type;          // Message type
    char* messageId;               // Message ID
    char* sessionId;               // Session ID
    char* operationId;             // Operation ID
    MCP_Content* content;          // Message content
    char* resourcePath;            // Resource path
    char* eventType;               // Event type
    char* toolName;                // Tool name
    char* errorCode;               // Error code
    char* errorMessage;            // Error message
} MCP_Message;

/**
 * @brief MCP protocol handler callbacks
 */
typedef struct {
    // New session callback
    void (*onNewSession)(const char* sessionId, const char* clientInfo);
    
    // Session closed callback
    void (*onSessionClosed)(const char* sessionId, const char* reason);
    
    // Content request callback
    void (*onContentRequest)(const char* sessionId, const char* operationId,
                           const MCP_Content* requestContent);
    
    // Tool invoke callback
    void (*onToolInvoke)(const char* sessionId, const char* operationId,
                       const char* toolName, const MCP_Content* params);
    
    // Event subscribe callback
    void (*onEventSubscribe)(const char* sessionId, const char* operationId,
                           const char* eventType);
    
    // Event unsubscribe callback
    void (*onEventUnsubscribe)(const char* sessionId, const char* operationId,
                             const char* eventType);
    
    // Resource get callback
    void (*onResourceGet)(const char* sessionId, const char* operationId,
                        const char* resourcePath);
    
    // Resource set callback
    void (*onResourceSet)(const char* sessionId, const char* operationId,
                        const char* resourcePath, const MCP_Content* value);
    
    // Error callback
    void (*onError)(const char* sessionId, const char* messageId,
                  const char* errorCode, const char* errorMessage);
} MCP_ProtocolCallbacks;

/**
 * @brief Initialize the MCP protocol handler
 * 
 * @param callbacks Protocol callback functions
 * @return int 0 on success, negative error code on failure
 */
int MCP_ProtocolHandlerInit(const MCP_ProtocolCallbacks* callbacks);

/**
 * @brief Process incoming data from a transport
 * 
 * @param transport Transport interface
 * @param sessionId Session ID (can be NULL for new sessions)
 * @param timeout Maximum time to block in milliseconds
 * @return int 0 on success, negative error code on failure
 */
int MCP_ProtocolHandlerProcessTransport(MCP_ServerTransport* transport, 
                                      const char* sessionId, uint32_t timeout);

/**
 * @brief Create a new MCP message
 * 
 * @param type Message type
 * @return MCP_Message* New message or NULL on failure
 */
MCP_Message* MCP_MessageCreate(MCP_MessageType type);

/**
 * @brief Free an MCP message
 * 
 * @param message Message to free
 */
void MCP_MessageFree(MCP_Message* message);

/**
 * @brief Send a message over a transport
 * 
 * @param transport Transport interface
 * @param message Message to send
 * @return int Number of bytes sent or negative error code
 */
int MCP_MessageSend(MCP_ServerTransport* transport, const MCP_Message* message);

/**
 * @brief Parse a message from binary data
 * 
 * @param data Binary data
 * @param size Data size
 * @return MCP_Message* Parsed message or NULL on failure
 */
MCP_Message* MCP_MessageParse(const uint8_t* data, size_t size);

/**
 * @brief Serialize a message to binary data
 * 
 * @param message Message to serialize
 * @param buffer Buffer to store serialized data
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_MessageSerialize(const MCP_Message* message, uint8_t* buffer, size_t bufferSize);

/**
 * @brief Create and send a content response message
 * 
 * @param transport Transport interface
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @param content Response content
 * @return int Number of bytes sent or negative error code
 */
int MCP_SendContentResponse(MCP_ServerTransport* transport, const char* sessionId,
                          const char* operationId, const MCP_Content* content);

/**
 * @brief Create and send a tool result message
 * 
 * @param transport Transport interface
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @param success Success flag
 * @param result Result content
 * @return int Number of bytes sent or negative error code
 */
int MCP_SendToolResult(MCP_ServerTransport* transport, const char* sessionId,
                     const char* operationId, bool success, const MCP_Content* result);

/**
 * @brief Create and send an event data message
 * 
 * @param transport Transport interface
 * @param sessionId Session ID
 * @param eventType Event type
 * @param data Event data
 * @return int Number of bytes sent or negative error code
 */
int MCP_SendEventData(MCP_ServerTransport* transport, const char* sessionId,
                    const char* eventType, const MCP_Content* data);

/**
 * @brief Create and send a resource data message
 * 
 * @param transport Transport interface
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @param resourcePath Resource path
 * @param data Resource data
 * @return int Number of bytes sent or negative error code
 */
int MCP_SendResourceData(MCP_ServerTransport* transport, const char* sessionId,
                       const char* operationId, const char* resourcePath,
                       const MCP_Content* data);

/**
 * @brief Create and send an error message
 * 
 * @param transport Transport interface
 * @param sessionId Session ID
 * @param messageId Message ID (can be NULL)
 * @param errorCode Error code
 * @param errorMessage Error message
 * @return int Number of bytes sent or negative error code
 */
int MCP_SendError(MCP_ServerTransport* transport, const char* sessionId,
                const char* messageId, const char* errorCode, const char* errorMessage);

#endif /* MCP_PROTOCOL_HANDLER_H */