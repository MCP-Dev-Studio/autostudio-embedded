#include "arduino_compat.h"
#include "mcp_logging.h"
#include "content.h"  // Include content.h which defines MCP_Content
#include "json_helpers.h"
#include "logging.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

// Add stub implementations for the host platform
#if defined(MCP_PLATFORM_HOST)
// Minimal MCP_Server structure for host platform testing
struct MCP_Server {
    char* deviceName;
    char* version;
    int sessionCount;
    void* transport;
};

// Declare the server function to get rather than redefining
struct MCP_Server* MCP_GetServer(void) {
    static struct MCP_Server dummyServer;
    dummyServer.deviceName = "Host Testing Device";
    dummyServer.version = "1.0.0";
    dummyServer.sessionCount = 0;
    dummyServer.transport = NULL;
    return &dummyServer;
}

// Don't redefine MCP_SendEventData, it's already defined in protocol_handler.h
// Just implement a stub that matches the real prototype
int __real_MCP_SendEventData(MCP_ServerTransport* transport, const char* sessionId, 
                           const char* eventType, const MCP_Content* content) {
    printf("[HOST] MCP_SendEventData called: %s\n", eventType);
    return 0;
}

// This is a local helper function for our logging implementation
static int mcp_local_send_event_data(void* transport, const char* sessionId, 
                                    const char* eventType, MCP_Content* content) {
    printf("[HOST] Local send event: %s\n", eventType);
    return 0;
}

int MCP_ServerBroadcastEvent(struct MCP_Server* server, const char* eventType, MCP_Content* content) {
    printf("[HOST] MCP_ServerBroadcastEvent called: %s\n", eventType);
    return 0;
}

uint32_t MCP_GetCurrentTimeMs(void) {
    return 0;
}
#endif

// Static variables
static struct MCP_Server* s_server = NULL;
static LogLevel s_maxLevel = LOG_LEVEL_INFO;
static bool s_enabled = false;
static void (*s_originalLogCallback)(LogLevel level, const char* message) = NULL;

// Module filtering
static bool s_filterByModule = false;
static char** s_allowedModules = NULL;
static uint32_t s_allowedModuleCount = 0;
static uint32_t s_allowedModuleCapacity = 0;

// Log formatting
static bool s_includeTimestamp = true;
static bool s_includeLevelName = true;
static bool s_includeModuleName = true;
static uint32_t s_outputs = LOG_OUTPUT_SERIAL | LOG_OUTPUT_MEMORY;

// Forward declarations
static void mcp_log_callback(LogLevel level, const char* message);
static int mcp_send_log_event(LogLevel level, const char* module, const char* message);
static bool is_module_allowed(const char* module);
static int parse_log_config(const MCP_Content* content, MCP_LogConfig* config);
static int serialize_log_config(const MCP_LogConfig* config, MCP_Content* content);

/**
 * @brief Initialize MCP logging bridge
 */
int MCP_LoggingInit(struct MCP_Server* server, LogLevel maxLevel) {
    if (server == NULL) {
        return -1;
    }
    
    // Free any existing module list
    MCP_LoggingClearAllowedModules();
    
    s_server = server;
    s_maxLevel = maxLevel;
    s_enabled = true;
    s_filterByModule = false;
    s_includeTimestamp = true;
    s_includeLevelName = true;
    s_includeModuleName = true;
    s_outputs = LOG_OUTPUT_SERIAL | LOG_OUTPUT_MEMORY;
    
    // Store the original log callback if any
    s_originalLogCallback = NULL;
    log_set_custom_callback(&mcp_log_callback);
    
    return 0;
}

/**
 * @brief Deinitialize MCP logging bridge
 */
int MCP_LoggingDeinit(void) {
    // Restore original callback
    log_set_custom_callback(s_originalLogCallback);
    
    // Free module list
    MCP_LoggingClearAllowedModules();
    
    s_server = NULL;
    s_enabled = false;
    s_originalLogCallback = NULL;
    
    return 0;
}

/**
 * @brief Set maximum log level to send via MCP
 */
LogLevel MCP_LoggingSetMaxLevel(LogLevel level) {
    LogLevel prevLevel = s_maxLevel;
    s_maxLevel = level;
    return prevLevel;
}

/**
 * @brief Get maximum log level sent via MCP
 */
LogLevel MCP_LoggingGetMaxLevel(void) {
    return s_maxLevel;
}

/**
 * @brief Enable or disable MCP logging
 */
bool MCP_LoggingEnable(bool enable) {
    bool prevState = s_enabled;
    s_enabled = enable;
    return prevState;
}

/**
 * @brief Check if MCP logging is enabled
 */
bool MCP_LoggingIsEnabled(void) {
    return s_enabled;
}

/**
 * @brief Get current logging configuration
 */
int MCP_LoggingGetConfig(MCP_LogConfig* config) {
    if (config == NULL) {
        return -1;
    }
    
    config->enabled = s_enabled;
    config->maxLevel = s_maxLevel;
    config->outputs = s_outputs;
    config->includeTimestamp = s_includeTimestamp;
    config->includeLevelName = s_includeLevelName;
    config->includeModuleName = s_includeModuleName;
    config->filterByModule = s_filterByModule;
    
    // Copy allowed modules
    config->allowedModuleCount = s_allowedModuleCount;
    config->allowedModules = NULL;
    
    if (s_allowedModuleCount > 0) {
        config->allowedModules = (char**)malloc(sizeof(char*) * s_allowedModuleCount);
        if (config->allowedModules == NULL) {
            return -2;
        }
        
        for (uint32_t i = 0; i < s_allowedModuleCount; i++) {
            if (s_allowedModules[i] != NULL) {
                config->allowedModules[i] = strdup(s_allowedModules[i]);
                if (config->allowedModules[i] == NULL) {
                    // Clean up on failure
                    for (uint32_t j = 0; j < i; j++) {
                        free(config->allowedModules[j]);
                    }
                    free(config->allowedModules);
                    config->allowedModules = NULL;
                    config->allowedModuleCount = 0;
                    return -3;
                }
            } else {
                config->allowedModules[i] = NULL;
            }
        }
    }
    
    return 0;
}

/**
 * @brief Set logging configuration
 */
int MCP_LoggingSetConfig(const MCP_LogConfig* config) {
    if (config == NULL) {
        return -1;
    }
    
    s_enabled = config->enabled;
    s_maxLevel = config->maxLevel;
    s_outputs = config->outputs;
    s_includeTimestamp = config->includeTimestamp;
    s_includeLevelName = config->includeLevelName;
    s_includeModuleName = config->includeModuleName;
    s_filterByModule = config->filterByModule;
    
    // Update allowed modules
    MCP_LoggingClearAllowedModules();
    
    if (config->allowedModuleCount > 0 && config->allowedModules != NULL) {
        for (uint32_t i = 0; i < config->allowedModuleCount; i++) {
            if (config->allowedModules[i] != NULL) {
                MCP_LoggingAddAllowedModule(config->allowedModules[i]);
            }
        }
    }
    
    // Update system log configuration
    LogConfig sysLogConfig = {
        .level = s_maxLevel,
        .outputs = s_outputs,
        .logFileName = NULL,
        .maxFileSize = 0,
        .maxMemoryEntries = 100,
        .includeTimestamp = s_includeTimestamp,
        .includeLevelName = s_includeLevelName,
        .includeModuleName = s_includeModuleName,
        .colorOutput = true,
        .customLogCallback = s_originalLogCallback
    };
    
    log_init(&sysLogConfig);
    log_set_custom_callback(&mcp_log_callback);
    
    return 0;
}

/**
 * @brief Add module to allowed modules filter list
 */
int MCP_LoggingAddAllowedModule(const char* moduleName) {
    if (moduleName == NULL) {
        return -1;
    }
    
    // Check if module already exists
    for (uint32_t i = 0; i < s_allowedModuleCount; i++) {
        if (s_allowedModules[i] != NULL && strcmp(s_allowedModules[i], moduleName) == 0) {
            return 0; // Already exists
        }
    }
    
    // Expand array if needed
    if (s_allowedModuleCount >= s_allowedModuleCapacity) {
        uint32_t newCapacity = s_allowedModuleCapacity == 0 ? 8 : s_allowedModuleCapacity * 2;
        char** newArray = (char**)realloc(s_allowedModules, sizeof(char*) * newCapacity);
        if (newArray == NULL) {
            return -2;
        }
        
        s_allowedModules = newArray;
        s_allowedModuleCapacity = newCapacity;
    }
    
    // Add module
    s_allowedModules[s_allowedModuleCount] = strdup(moduleName);
    if (s_allowedModules[s_allowedModuleCount] == NULL) {
        return -3;
    }
    
    s_allowedModuleCount++;
    return 0;
}

/**
 * @brief Remove module from allowed modules filter list
 */
int MCP_LoggingRemoveAllowedModule(const char* moduleName) {
    if (moduleName == NULL || s_allowedModules == NULL) {
        return -1;
    }
    
    for (uint32_t i = 0; i < s_allowedModuleCount; i++) {
        if (s_allowedModules[i] != NULL && strcmp(s_allowedModules[i], moduleName) == 0) {
            // Free this entry
            free(s_allowedModules[i]);
            
            // Move remaining entries
            if (i < s_allowedModuleCount - 1) {
                memmove(&s_allowedModules[i], &s_allowedModules[i + 1], 
                        (s_allowedModuleCount - i - 1) * sizeof(char*));
            }
            
            s_allowedModuleCount--;
            return 0;
        }
    }
    
    return -2; // Module not found
}

/**
 * @brief Clear allowed modules filter list
 */
int MCP_LoggingClearAllowedModules(void) {
    if (s_allowedModules != NULL) {
        for (uint32_t i = 0; i < s_allowedModuleCount; i++) {
            free(s_allowedModules[i]);
        }
        
        free(s_allowedModules);
        s_allowedModules = NULL;
    }
    
    s_allowedModuleCount = 0;
    s_allowedModuleCapacity = 0;
    return 0;
}

/**
 * @brief Set filter by module flag
 */
bool MCP_LoggingSetFilterByModule(bool enable) {
    bool prevState = s_filterByModule;
    s_filterByModule = enable;
    return prevState;
}

/**
 * @brief Handle log event subscription from client
 */
int MCP_LoggingHandleSubscription(const char* sessionId, const char* operationId) {
    // No specific handling needed here, just acknowledge subscription
    // The protocol handler will add this session to subscribers for the log event type
    
#if !defined(MCP_PLATFORM_HOST)
    // Send a confirmation event and current configuration
    MCP_Content* content = MCP_ContentCreateObject();
    if (content == NULL) {
        return -1;
    }
    
    MCP_ContentAddString(content, "status", "subscribed");
    MCP_ContentAddString(content, "eventType", MCP_EVENT_TYPE_LOG);
    MCP_ContentAddNumber(content, "maxLevel", (double)s_maxLevel);
    MCP_ContentAddBoolean(content, "enabled", s_enabled);
    
    int result = MCP_SendEventData(
        s_server->transport, 
        sessionId, 
        MCP_EVENT_TYPE_LOG, 
        content
    );
    
    MCP_ContentFree(content);
#else
    // For host platform, just simulate a successful send
    printf("[HOST] Handling subscription for: %s\n", sessionId);
#endif
    
    // Also send current configuration
    return MCP_LoggingSendConfig(sessionId, operationId);
}

/**
 * @brief Handle log event unsubscription from client
 */
int MCP_LoggingHandleUnsubscription(const char* sessionId, const char* operationId) {
    // No specific handling needed here
    // The protocol handler will remove this session from subscribers
    return 0;
}

/**
 * @brief Handle log configuration request from client
 */
int MCP_LoggingHandleConfig(const char* sessionId, const char* operationId, 
                          const MCP_Content* content) {
    if (content == NULL) {
        return -1;
    }
    
    // Parse configuration from content
    MCP_LogConfig config;
    memset(&config, 0, sizeof(config));
    
    int result = parse_log_config(content, &config);
    if (result < 0) {
        return result;
    }
    
    // Apply configuration
    result = MCP_LoggingSetConfig(&config);
    
    // Free allocated memory
    if (config.allowedModules != NULL) {
        for (uint32_t i = 0; i < config.allowedModuleCount; i++) {
            free(config.allowedModules[i]);
        }
        free(config.allowedModules);
    }
    
    if (result < 0) {
        return result;
    }
    
    // Send updated configuration to all clients
    MCP_Content* updatedContent = MCP_ContentCreateObject();
    if (updatedContent == NULL) {
        return -3;
    }
    
    MCP_ContentAddString(updatedContent, "action", "config_updated");
    
    // Add current configuration
    MCP_LogConfig currentConfig;
    memset(&currentConfig, 0, sizeof(currentConfig));
    MCP_LoggingGetConfig(&currentConfig);
    
    MCP_Content* configContent = MCP_ContentCreateObject();
    serialize_log_config(&currentConfig, configContent);
    MCP_ContentAddObject(updatedContent, "config", configContent);
    
    // Free allocated memory
    if (currentConfig.allowedModules != NULL) {
        for (uint32_t i = 0; i < currentConfig.allowedModuleCount; i++) {
            free(currentConfig.allowedModules[i]);
        }
        free(currentConfig.allowedModules);
    }
    
    // Broadcast the configuration update to all subscribed clients
    #if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
    // For Arduino, use the other server event function
    result = MCP_ServerSendEvent(MCP_EVENT_TYPE_LOG_CONFIG, (const uint8_t*)updatedContent->resultJson, strlen(updatedContent->resultJson));
    #else
    result = MCP_ServerBroadcastEvent(s_server, MCP_EVENT_TYPE_LOG_CONFIG, updatedContent);
    #endif
    
    MCP_ContentFree(updatedContent);
    return result;
}

/**
 * @brief Send current log configuration to client
 */
int MCP_LoggingSendConfig(const char* sessionId, const char* operationId) {
#if !defined(MCP_PLATFORM_HOST)
    // Create content with current configuration
    MCP_Content* content = MCP_ContentCreateObject();
    if (content == NULL) {
        return -1;
    }
    
    MCP_ContentAddString(content, "action", "current_config");
    
    // Get current configuration
    MCP_LogConfig config;
    memset(&config, 0, sizeof(config));
    int result = MCP_LoggingGetConfig(&config);
    
    if (result < 0) {
        MCP_ContentFree(content);
        return result;
    }
    
    // Add configuration to content
    MCP_Content* configContent = MCP_ContentCreateObject();
    if (configContent == NULL) {
        MCP_ContentFree(content);
        return -2;
    }
    
    serialize_log_config(&config, configContent);
    MCP_ContentAddObject(content, "config", configContent);
    
    // Free allocated memory
    if (config.allowedModules != NULL) {
        for (uint32_t i = 0; i < config.allowedModuleCount; i++) {
            free(config.allowedModules[i]);
        }
        free(config.allowedModules);
    }
    
    // Send to the client
    result = MCP_SendEventData(
        s_server->transport,
        sessionId,
        MCP_EVENT_TYPE_LOG_CONFIG,
        content
    );
    
    MCP_ContentFree(content);
    return result;
#else
    // For host platform, just simulate a successful send
    printf("[HOST] Sending config to: %s\n", sessionId);
    return 0;
#endif
}

/**
 * @brief Custom log callback to forward logs to MCP clients
 */
static void mcp_log_callback(LogLevel level, const char* message) {
    if (!s_enabled || level > s_maxLevel || s_server == NULL) {
        // If disabled or level too high, skip
        if (s_originalLogCallback != NULL) {
            s_originalLogCallback(level, message);
        }
        return;
    }
    
    // Extract module name from message (assumes format "[MODULE] message")
    const char* moduleEnd = strchr(message, ']');
    const char* moduleStart = strchr(message, '[');
    
    char module[32] = "unknown";
    if (moduleStart != NULL && moduleEnd != NULL && moduleEnd > moduleStart) {
        size_t moduleLen = moduleEnd - moduleStart - 1;
        if (moduleLen < sizeof(module)) {
            strncpy(module, moduleStart + 1, moduleLen);
            module[moduleLen] = '\0';
        }
    }
    
    // Check module filtering
    if (s_filterByModule && !is_module_allowed(module)) {
        // Skip this module
        if (s_originalLogCallback != NULL) {
            s_originalLogCallback(level, message);
        }
        return;
    }
    
    // Extract actual message (after the module prefix)
    const char* actualMessage = message;
    if (moduleEnd != NULL) {
        actualMessage = moduleEnd + 1;
        while (*actualMessage == ' ') {
            actualMessage++;
        }
    }
    
    // Send the log message as an MCP event
    mcp_send_log_event(level, module, actualMessage);
    
    // Chain to original callback if any
    if (s_originalLogCallback != NULL) {
        s_originalLogCallback(level, message);
    }
}

/**
 * @brief Send a log message as an MCP event
 */
static int mcp_send_log_event(LogLevel level, const char* module, const char* message) {
    if (s_server == NULL || !s_enabled) {
        return -1;
    }
    
    // Create event content
    MCP_Content* content = MCP_ContentCreateObject();
    if (content == NULL) {
        return -1;
    }
    
    // Add log data
    MCP_ContentAddNumber(content, "level", (double)level);
    MCP_ContentAddString(content, "levelName", log_level_name(level));
    MCP_ContentAddString(content, "module", module);
    MCP_ContentAddString(content, "message", message);
    #if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
    // For Arduino, use a simpler timestamp approach
    MCP_ContentAddNumber(content, "timestamp", (double)MCP_GetCurrentTimeMs());
    #else
    MCP_ContentAddNumber(content, "timestamp", (double)MCP_GetCurrentTimeMs());
    #endif
    
    // Add formatting flags
    MCP_ContentAddBoolean(content, "includeTimestamp", s_includeTimestamp);
    MCP_ContentAddBoolean(content, "includeLevelName", s_includeLevelName);
    MCP_ContentAddBoolean(content, "includeModuleName", s_includeModuleName);
    
    // Send to all subscribed sessions
    #if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
    // For Arduino, use the other server event function
    int result = MCP_ServerSendEvent(MCP_EVENT_TYPE_LOG, (const uint8_t*)content->resultJson, strlen(content->resultJson));
    #else
    int result = MCP_ServerBroadcastEvent(s_server, MCP_EVENT_TYPE_LOG, content);
    #endif
    
    MCP_ContentFree(content);
    return result;
}

/**
 * @brief Check if a module is in the allowed list
 */
static bool is_module_allowed(const char* module) {
    if (!s_filterByModule || module == NULL) {
        return true;
    }
    
    if (s_allowedModules == NULL || s_allowedModuleCount == 0) {
        return false;
    }
    
    for (uint32_t i = 0; i < s_allowedModuleCount; i++) {
        if (s_allowedModules[i] != NULL && strcmp(s_allowedModules[i], module) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Parse log configuration from MCP content
 */
static int parse_log_config(const MCP_Content* content, MCP_LogConfig* config) {
    if (content == NULL || config == NULL) {
        return -1;
    }
    
    memset(config, 0, sizeof(MCP_LogConfig));
    
    // Get configuration fields
    MCP_ContentGetBoolean(content, "enabled", &config->enabled);
    
    double maxLevel = (double)LOG_LEVEL_INFO;
    MCP_ContentGetNumber(content, "maxLevel", &maxLevel);
    config->maxLevel = (LogLevel)(int)maxLevel;
    
    double outputs = (double)(LOG_OUTPUT_SERIAL | LOG_OUTPUT_MEMORY);
    MCP_ContentGetNumber(content, "outputs", &outputs);
    config->outputs = (uint32_t)outputs;
    
    MCP_ContentGetBoolean(content, "includeTimestamp", &config->includeTimestamp);
    MCP_ContentGetBoolean(content, "includeLevelName", &config->includeLevelName);
    MCP_ContentGetBoolean(content, "includeModuleName", &config->includeModuleName);
    MCP_ContentGetBoolean(content, "filterByModule", &config->filterByModule);
    
    // Get allowed modules
    MCP_Content* modulesArray = NULL;
    config->allowedModuleCount = 0;
    config->allowedModules = NULL;
    
    if (MCP_ContentGetArray(content, "allowedModules", &modulesArray) && modulesArray != NULL) {
        uint32_t count = MCP_ContentGetArrayLength(modulesArray);
        if (count > 0) {
            config->allowedModules = (char**)malloc(sizeof(char*) * count);
            if (config->allowedModules == NULL) {
                return -2;
            }
            
            for (uint32_t i = 0; i < count; i++) {
                const char* module = NULL;
                if (MCP_ContentGetArrayStringAt(modulesArray, i, &module) && module != NULL) {
                    config->allowedModules[config->allowedModuleCount] = strdup(module);
                    if (config->allowedModules[config->allowedModuleCount] != NULL) {
                        config->allowedModuleCount++;
                    }
                }
            }
        }
    }
    
    return 0;
}

/**
 * @brief Serialize log configuration to MCP content
 */
static int serialize_log_config(const MCP_LogConfig* config, MCP_Content* content) {
    if (config == NULL || content == NULL) {
        return -1;
    }
    
    // Add basic configuration
    MCP_ContentAddBoolean(content, "enabled", config->enabled);
    MCP_ContentAddNumber(content, "maxLevel", (double)config->maxLevel);
    MCP_ContentAddNumber(content, "outputs", (double)config->outputs);
    MCP_ContentAddBoolean(content, "includeTimestamp", config->includeTimestamp);
    MCP_ContentAddBoolean(content, "includeLevelName", config->includeLevelName);
    MCP_ContentAddBoolean(content, "includeModuleName", config->includeModuleName);
    MCP_ContentAddBoolean(content, "filterByModule", config->filterByModule);
    
    // Add allowed modules
    MCP_Content* modulesArray = MCP_ContentCreateArray();
    if (modulesArray != NULL) {
        for (uint32_t i = 0; i < config->allowedModuleCount; i++) {
            if (config->allowedModules[i] != NULL) {
                MCP_ContentAddArrayString(modulesArray, config->allowedModules[i]);
            }
        }
        
        MCP_ContentAddArray(content, "allowedModules", modulesArray);
    }
    
    return 0;
}
