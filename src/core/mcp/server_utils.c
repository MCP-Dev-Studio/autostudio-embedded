#include "server_utils.h"
#include "server.h"
#include "content.h"
#include "protocol_handler.h"
#include <string.h>

// Include platform-specific headers for time function
#if defined(MCP_OS_MBED)
    #include "../hal/mbed/hal_mbed.h"
#elif defined(MCP_OS_ARDUINO)
    #include "../hal/arduino/hal_arduino.h"
#elif defined(MCP_OS_ESP32)
    #include "../hal/esp32/hal_esp32.h"
#else
    // Default implementation
    #include <time.h>
#endif

// Global server instance
static MCP_Server s_server = {0};

/**
 * @brief Get global MCP server instance
 */
MCP_Server* MCP_GetServer(void) {
    if (!s_server.initialized) {
        return NULL;
    }
    return &s_server;
}

/**
 * @brief Initialize global server instance
 */
int MCP_ServerInitGlobal(const MCP_ServerConfig* config, MCP_ServerTransport* transport) {
    if (config == NULL) {
        return -1;
    }

    // Copy configuration
    memcpy(&s_server.config, config, sizeof(MCP_ServerConfig));
    
    // Set transport if provided
    if (transport != NULL) {
        s_server.transport = transport;
    }
    
    // Set initialization time
    s_server.startTime = MCP_GetCurrentTimeMs();
    s_server.initialized = true;
    
    return 0;
}

/**
 * @brief Broadcast an event to all subscribed sessions
 */
int MCP_ServerBroadcastEvent(MCP_Server* server, const char* eventType, MCP_Content* content) {
    if (server == NULL || eventType == NULL || content == NULL) {
        return -1;
    }
    
    // Convert MCP_Content to raw data
    uint8_t buffer[2048]; // Temporary buffer for serialized data
    
    // Serialize content to buffer
    int result = MCP_ContentSerialize(content, buffer, sizeof(buffer));
    if (result < 0) {
        return result;
    }
    
    // Use existing MCP_ServerSendEvent to broadcast
    return MCP_ServerSendEvent(eventType, buffer, (size_t)result);
}

/**
 * @brief Get current time in milliseconds
 */
uint32_t MCP_GetCurrentTimeMs(void) {
    // Platform-specific implementation
#if defined(MCP_OS_MBED)
    return HAL_MbedGetMillis();
#elif defined(MCP_OS_ARDUINO)
    return HAL_ArduinoMillis();
#elif defined(MCP_OS_ESP32)
    return HAL_ESP32GetMillis();
#else
    // Default implementation
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}