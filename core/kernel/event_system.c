#include "event_system.h"
#include <stdlib.h>
#include <string.h>

// Handler structure
typedef struct {
    uint32_t id;
    int type;                  // -1 for all types
    const char* source;        // NULL for all sources
    MCP_EventHandler handler;
    void* userData;
    bool active;
} HandlerInfo;

// Circular queue for events
typedef struct {
    MCP_Event* events;
    uint16_t size;
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} EventQueue;

// Internal state
static HandlerInfo* s_handlers = NULL;
static uint16_t s_maxHandlers = 0;
static uint16_t s_handlerCount = 0;
static uint32_t s_nextHandlerId = 1;
static EventQueue s_queue = {0};
static bool s_initialized = false;

int MCP_EventSystemInit(uint16_t maxHandlers, uint16_t queueSize) {
    if (s_initialized) {
        return -1;
    }
    
    // Allocate handler array
    s_handlers = (HandlerInfo*)calloc(maxHandlers, sizeof(HandlerInfo));
    if (s_handlers == NULL) {
        return -2;
    }
    
    // Allocate event queue
    s_queue.events = (MCP_Event*)calloc(queueSize, sizeof(MCP_Event));
    if (s_queue.events == NULL) {
        free(s_handlers);
        s_handlers = NULL;
        return -3;
    }
    
    s_maxHandlers = maxHandlers;
    s_handlerCount = 0;
    s_nextHandlerId = 1;
    
    s_queue.size = queueSize;
    s_queue.head = 0;
    s_queue.tail = 0;
    s_queue.count = 0;
    
    s_initialized = true;
    return 0;
}

uint32_t MCP_EventRegisterHandler(int type, const char* source, MCP_EventHandler handler, void* userData) {
    if (!s_initialized || handler == NULL) {
        return 0;
    }
    
    if (s_handlerCount >= s_maxHandlers) {
        return 0;  // No space for new handler
    }
    
    // Find free slot
    uint16_t i;
    for (i = 0; i < s_maxHandlers; i++) {
        if (!s_handlers[i].active) {
            break;
        }
    }
    
    if (i >= s_maxHandlers) {
        return 0;  // No free slot found
    }
    
    // Register handler
    s_handlers[i].id = s_nextHandlerId++;
    s_handlers[i].type = type;
    s_handlers[i].source = source;
    s_handlers[i].handler = handler;
    s_handlers[i].userData = userData;
    s_handlers[i].active = true;
    
    s_handlerCount++;
    
    return s_handlers[i].id;
}

int MCP_EventUnregisterHandler(uint32_t handlerId) {
    if (!s_initialized || handlerId == 0) {
        return -1;
    }
    
    // Find handler by ID
    for (uint16_t i = 0; i < s_maxHandlers; i++) {
        if (s_handlers[i].active && s_handlers[i].id == handlerId) {
            // Mark as inactive
            s_handlers[i].active = false;
            s_handlerCount--;
            return 0;
        }
    }
    
    return -2;  // Handler not found
}

static bool isQueueFull(void) {
    return s_queue.count == s_queue.size;
}

static bool isQueueEmpty(void) {
    return s_queue.count == 0;
}

static int enqueueEvent(const MCP_Event* event) {
    if (isQueueFull()) {
        return -1;  // Queue full
    }
    
    // Copy event to queue
    memcpy(&s_queue.events[s_queue.tail], event, sizeof(MCP_Event));
    
    // Move tail pointer
    s_queue.tail = (s_queue.tail + 1) % s_queue.size;
    s_queue.count++;
    
    return 0;
}

static int dequeueEvent(MCP_Event* event) {
    if (isQueueEmpty()) {
        return -1;  // Queue empty
    }
    
    // Copy event from queue
    memcpy(event, &s_queue.events[s_queue.head], sizeof(MCP_Event));
    
    // Move head pointer
    s_queue.head = (s_queue.head + 1) % s_queue.size;
    s_queue.count--;
    
    return 0;
}

int MCP_EventPublish(const MCP_Event* event) {
    if (!s_initialized || event == NULL) {
        return -1;
    }
    
    // Add to queue
    return enqueueEvent(event);
}

static bool shouldHandleEvent(const HandlerInfo* handler, const MCP_Event* event) {
    // Check event type
    if (handler->type != -1 && handler->type != event->type) {
        return false;
    }
    
    // Check event source
    if (handler->source != NULL && event->source != NULL) {
        if (strcmp(handler->source, event->source) != 0) {
            return false;
        }
    } else if (handler->source != NULL) {
        return false;  // Handler requires source, but event has none
    }
    
    return true;
}

int MCP_EventProcess(uint16_t maxEvents) {
    if (!s_initialized) {
        return -1;
    }
    
    // Process all events if maxEvents is 0
    if (maxEvents == 0) {
        maxEvents = s_queue.count;
    }
    
    int processedCount = 0;
    MCP_Event event;
    
    for (uint16_t i = 0; i < maxEvents; i++) {
        // Get next event
        if (dequeueEvent(&event) != 0) {
            break;  // No more events
        }
        
        // Dispatch to handlers
        for (uint16_t j = 0; j < s_maxHandlers; j++) {
            if (s_handlers[j].active && shouldHandleEvent(&s_handlers[j], &event)) {
                s_handlers[j].handler(&event, s_handlers[j].userData);
            }
        }
        
        processedCount++;
    }
    
    return processedCount;
}

// This is a minimal JSON implementation for simplicity
// In a real implementation, you'd use a proper JSON library
int MCP_EventToJson(const MCP_Event* event, char* buffer, size_t bufferSize) {
    if (event == NULL || buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // Format the JSON
    int written = snprintf(buffer, bufferSize,
                         "{"
                         "\"type\":%d,"
                         "\"id\":%lu,"
                         "\"source\":\"%s\","
                         "\"timestamp\":%lu,"
                         "\"dataSize\":%lu"
                         "}",
                         event->type,
                         (unsigned long)event->id,
                         event->source ? event->source : "",
                         (unsigned long)event->timestamp,
                         (unsigned long)event->dataSize);
    
    return (written < 0 || (size_t)written >= bufferSize) ? -2 : written;
}