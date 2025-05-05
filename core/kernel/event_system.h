#ifndef MCP_EVENT_SYSTEM_H
#define MCP_EVENT_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Event types
 */
typedef enum {
    MCP_EVENT_TYPE_SYSTEM,      // System events
    MCP_EVENT_TYPE_SENSOR,      // Sensor events
    MCP_EVENT_TYPE_ACTUATOR,    // Actuator events
    MCP_EVENT_TYPE_INPUT,       // Input events
    MCP_EVENT_TYPE_NETWORK,     // Network events
    MCP_EVENT_TYPE_USER,        // User-defined events
    MCP_EVENT_TYPE_TOOL         // Tool execution events
} MCP_EventType;

/**
 * @brief Event structure
 */
typedef struct {
    MCP_EventType type;         // Event type
    uint32_t id;                // Event ID
    const char* source;         // Event source (e.g., sensor ID)
    uint32_t timestamp;         // Event timestamp (ms)
    void* data;                 // Event data
    size_t dataSize;            // Event data size
} MCP_Event;

/**
 * @brief Event handler function type
 */
typedef void (*MCP_EventHandler)(const MCP_Event* event, void* userData);

/**
 * @brief Initialize the event system
 * 
 * @param maxHandlers Maximum number of event handlers
 * @param queueSize Event queue size
 * @return int 0 on success, negative error code on failure
 */
int MCP_EventSystemInit(uint16_t maxHandlers, uint16_t queueSize);

/**
 * @brief Register an event handler
 * 
 * @param type Event type to handle (or -1 for all events)
 * @param source Event source to handle (or NULL for all sources)
 * @param handler Handler function
 * @param userData User data to pass to the handler
 * @return uint32_t Handler ID or 0 on failure
 */
uint32_t MCP_EventRegisterHandler(int type, const char* source, MCP_EventHandler handler, void* userData);

/**
 * @brief Unregister an event handler
 * 
 * @param handlerId Handler ID to unregister
 * @return int 0 on success, negative error code on failure
 */
int MCP_EventUnregisterHandler(uint32_t handlerId);

/**
 * @brief Publish an event to the event system
 * 
 * @param event Event to publish
 * @return int 0 on success, negative error code on failure
 */
int MCP_EventPublish(const MCP_Event* event);

/**
 * @brief Process pending events in the event queue
 * 
 * @param maxEvents Maximum number of events to process (0 for all pending)
 * @return int Number of events processed
 */
int MCP_EventProcess(uint16_t maxEvents);

/**
 * @brief Create a JSON representation of an event
 * 
 * @param event Event to convert
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_EventToJson(const MCP_Event* event, char* buffer, size_t bufferSize);

#endif /* MCP_EVENT_SYSTEM_H */