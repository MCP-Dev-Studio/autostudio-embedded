/**
 * @file server.c
 * @brief Implementation of MCP server for host platform
 */
#include "server.h"
#include "content.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Only build this stub for host platform
#if defined(MCP_PLATFORM_HOST)

// Define the MCP_Server type for host platform
typedef struct {
    char* deviceName;         // Device name
    char* version;            // Server version
    int sessionCount;         // Current active sessions
    MCP_ServerTransport* transport; // Transport layer
} MCP_Server;

// Server transport stub implementation - matches the header
struct MCP_ServerTransport {
    MCP_ServerTransportType type;
    int (*read)(uint8_t* buffer, size_t maxLength, uint32_t timeout);
    int (*write)(const uint8_t* data, size_t length);
    int (*close)(void);
    uint32_t (*getStatus)(void);
    void* config;
    void* userData;
};

// Static global server instance
static struct {
    char* deviceName;
    char* version;
    int sessionCount;
    MCP_ServerTransport* transport;
} s_server = {0};

static bool s_initialized = false;

// Forward declarations of internal functions
static int stub_transport_read(uint8_t* buffer, size_t maxLength, uint32_t timeout);
static int stub_transport_write(const uint8_t* data, size_t length);
static int stub_transport_close(void);
static uint32_t stub_transport_status(void);

/**
 * @brief Initialize the MCP server
 */
int MCP_ServerInit(const MCP_ServerConfig* config) {
    if (s_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize server structure with default values if config is NULL
    if (config == NULL) {
        s_server.deviceName = strdup("MCP Host Device");
        s_server.version = strdup("1.0.0");
    } else {
        s_server.deviceName = config->deviceName ? strdup(config->deviceName) : strdup("MCP Host Device");
        s_server.version = config->version ? strdup(config->version) : strdup("1.0.0");
    }
    
    s_server.sessionCount = 0;
    
    // Create and initialize transport
    s_server.transport = (MCP_ServerTransport*)malloc(sizeof(MCP_ServerTransport));
    if (s_server.transport == NULL) {
        free(s_server.deviceName);
        free(s_server.version);
        return -1;
    }
    
    // Set up a dummy transport
    s_server.transport->type = MCP_TRANSPORT_TCP;
    s_server.transport->read = stub_transport_read;
    s_server.transport->write = stub_transport_write;
    s_server.transport->close = stub_transport_close;
    s_server.transport->getStatus = stub_transport_status;
    s_server.transport->config = NULL;
    s_server.transport->userData = NULL;
    
    s_initialized = true;
    printf("MCP Server initialized for host platform\n");
    return 0;
}

/**
 * @brief Get the global server instance
 */
MCP_Server* MCP_GetServer(void) {
    static MCP_Server server;
    
    // Create a wrapper around our internal structure
    server.deviceName = s_server.deviceName;
    server.version = s_server.version;
    server.sessionCount = s_server.sessionCount;
    server.transport = s_server.transport;
    
    return &server;
}

/**
 * @brief Start the MCP server
 */
int MCP_ServerStart(void) {
    if (!s_initialized) {
        return -1;
    }
    
    printf("MCP Server started (stub implementation for host platform)\n");
    return 0;
}

/**
 * @brief Stop the MCP server
 */
int MCP_ServerStop(void) {
    if (!s_initialized) {
        return -1;
    }
    
    printf("MCP Server stopped (stub implementation for host platform)\n");
    return 0;
}

/**
 * @brief Get server status
 */
bool MCP_ServerIsRunning(void) {
    return s_initialized;
}

/**
 * @brief Send tool result via transport
 */
int MCP_SendToolResult(MCP_ServerTransport* transport, const char* sessionId, 
                     const char* operationId, bool success, const MCP_Content* content) {
    (void)transport; // Suppress unused parameter warning
    (void)content;   // Suppress unused parameter warning
    
    printf("MCP Tool Result (stub): Session=%s, Operation=%s, Success=%s\n",
           sessionId, operationId, success ? "true" : "false");
    return 0;
}

// Stub implementations for transport functions
static int stub_transport_read(uint8_t* buffer, size_t maxLength, uint32_t timeout) {
    (void)buffer;     // Suppress unused parameter warning
    (void)maxLength;  // Suppress unused parameter warning
    (void)timeout;    // Suppress unused parameter warning
    return 0;
}

static int stub_transport_write(const uint8_t* data, size_t length) {
    (void)data;      // Suppress unused parameter warning
    (void)length;    // Suppress unused parameter warning
    return 0;
}

static int stub_transport_close(void) {
    return 0;
}

static uint32_t stub_transport_status(void) {
    return 0;
}

// Implement other server functions as needed
int MCP_ServerConnect(MCP_ServerTransport* transport) {
    (void)transport;  // Suppress unused parameter warning
    printf("MCP_ServerConnect called (stub)\n");
    return 1; // Return a dummy session ID
}

int MCP_ServerProcess(uint32_t timeout) {
    (void)timeout;  // Suppress unused parameter warning
    return 0; // No operations processed
}

const char* MCP_ServerRegisterOperation(const char* sessionId, MCP_OperationType type) {
    (void)sessionId;  // Suppress unused parameter warning
    (void)type;       // Suppress unused parameter warning
    return "op_123"; // Return a dummy operation ID
}

int MCP_ServerCompleteOperation(const char* sessionId, const char* operationId, 
                               bool success, const uint8_t* data, size_t dataLength) {
    (void)sessionId;    // Suppress unused parameter warning
    (void)operationId;  // Suppress unused parameter warning
    (void)success;      // Suppress unused parameter warning
    (void)data;         // Suppress unused parameter warning
    (void)dataLength;   // Suppress unused parameter warning
    return 0;
}

int MCP_ServerCloseSession(const char* sessionId) {
    (void)sessionId;  // Suppress unused parameter warning
    return 0;
}

int MCP_ServerGetStatus(char* buffer, size_t bufferSize) {
    if (buffer != NULL && bufferSize > 0) {
        const char* status = "{\"running\": true, \"sessions\": 0}";
        size_t len = strlen(status);
        
        if (len < bufferSize) {
            memcpy(buffer, status, len);
            buffer[len] = '\0';
            return (int)len;
        }
    }
    return -1;
}

int MCP_ServerSendEvent(const char* eventType, const uint8_t* eventData, size_t eventDataLength) {
    (void)eventType;       // Suppress unused parameter warning
    (void)eventData;       // Suppress unused parameter warning
    (void)eventDataLength; // Suppress unused parameter warning
    return 0;
}

bool MCP_ValidateAuth(int method, const char* token) {
    (void)method;  // Suppress unused parameter warning
    (void)token;   // Suppress unused parameter warning
    return true; // All authentication is valid for host platform stub
}

MCP_ServerTransport* MCP_USBTransportInit(const MCP_USBTransportConfig* config) {
    (void)config;  // Suppress unused parameter warning
    return NULL; // Not implemented for host platform
}

int MCP_USBTransportStart(MCP_ServerTransport* transport) {
    (void)transport;  // Suppress unused parameter warning
    return -1; // Not implemented for host platform
}

MCP_ServerTransport* MCP_EthernetTransportInit(const MCP_EthernetTransportConfig* config) {
    (void)config;  // Suppress unused parameter warning
    return NULL; // Not implemented for host platform
}

int MCP_EthernetTransportStart(MCP_ServerTransport* transport) {
    (void)transport;  // Suppress unused parameter warning
    return -1; // Not implemented for host platform
}

#endif // MCP_PLATFORM_HOST

// Arduino platform implementation
#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

// Static singleton instance of the MCP server
static MCP_Server s_arduino_server = {
    .transport = NULL
};

/**
 * @brief Get the singleton MCP server instance
 * 
 * @return MCP_Server* Pointer to the server instance
 */
MCP_Server* MCP_GetServer(void) {
    return &s_arduino_server;
}

/**
 * @brief Send tool result via transport for Arduino
 */
int MCP_SendToolResult(MCP_ServerTransport* transport, const char* sessionId, 
                     const char* operationId, bool success, const MCP_Content* content) {
    (void)transport; // Suppress unused parameter warning
    (void)content;   // Suppress unused parameter warning
    
    printf("MCP Tool Result (Arduino): Session=%s, Operation=%s, Success=%s\n",
           sessionId, operationId, success ? "true" : "false");
    return 0;
}

/**
 * @brief Broadcast an event to all connected clients
 * Arduino-specific stub implementation
 */
int MCP_ServerBroadcastEvent(MCP_Server* server, const char* eventType, const MCP_Content* content) {
    (void)server;    // Suppress unused parameter warning
    (void)content;   // Suppress unused parameter warning
    
    printf("MCP ServerBroadcastEvent (Arduino): EventType=%s\n", eventType);
    return 0;
}

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)