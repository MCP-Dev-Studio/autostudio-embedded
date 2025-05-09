#ifndef MCP_LOGGING_H
#define MCP_LOGGING_H

#include "event_system.h"
#include "protocol_handler.h"
#include "server.h"
#include "content.h"
#include "logging.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declare MCP_Server for host platform
#if defined(MCP_PLATFORM_HOST)
struct MCP_Server;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log event type for MCP events
 * Clients can subscribe to this event type to receive log messages
 */
#define MCP_EVENT_TYPE_LOG "log"

/**
 * @brief Log configuration event type for MCP events
 * Used for configuring log settings from client
 */
#define MCP_EVENT_TYPE_LOG_CONFIG "log_config"

/**
 * @brief Log message structure for MCP events
 */
typedef struct {
    LogLevel level;            // Log level
    uint32_t timestamp;        // Timestamp in milliseconds
    const char* module;        // Module name
    const char* message;       // Log message
} MCP_LogMessage;

/**
 * @brief Log configuration structure
 */
typedef struct {
    bool enabled;                // Whether logging is enabled
    LogLevel maxLevel;           // Maximum log level to send
    uint32_t outputs;            // Bit mask of log outputs (from LogOutput enum)
    bool includeTimestamp;       // Include timestamp in log entries
    bool includeLevelName;       // Include level name in log entries
    bool includeModuleName;      // Include module name in log entries
    bool filterByModule;         // Whether to filter logs by module
    char** allowedModules;       // List of allowed module names (if filterByModule is true)
    uint32_t allowedModuleCount; // Number of allowed modules
} MCP_LogConfig;

/**
 * @brief Initialize MCP logging bridge
 * 
 * This initializes the bridge between the logging system and MCP events.
 * Log messages will be converted to MCP events and sent to subscribed clients.
 * 
 * @param server Pointer to MCP server
 * @param maxLevel Maximum log level to send (e.g., LOG_LEVEL_INFO)
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingInit(struct MCP_Server* server, LogLevel maxLevel);

/**
 * @brief Deinitialize MCP logging bridge
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingDeinit(void);

/**
 * @brief Set maximum log level to send via MCP
 * 
 * @param level Maximum log level
 * @return LogLevel Previous maximum log level
 */
LogLevel MCP_LoggingSetMaxLevel(LogLevel level);

/**
 * @brief Get maximum log level sent via MCP
 * 
 * @return LogLevel Current maximum log level
 */
LogLevel MCP_LoggingGetMaxLevel(void);

/**
 * @brief Enable or disable MCP logging
 * 
 * @param enable true to enable, false to disable
 * @return bool Previous state
 */
bool MCP_LoggingEnable(bool enable);

/**
 * @brief Check if MCP logging is enabled
 * 
 * @return bool true if enabled, false if disabled
 */
bool MCP_LoggingIsEnabled(void);

/**
 * @brief Get current logging configuration
 * 
 * @param config Pointer to configuration structure to fill
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingGetConfig(MCP_LogConfig* config);

/**
 * @brief Set logging configuration
 * 
 * @param config New configuration to apply
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingSetConfig(const MCP_LogConfig* config);

/**
 * @brief Add module to allowed modules filter list
 * 
 * @param moduleName Module name to allow
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingAddAllowedModule(const char* moduleName);

/**
 * @brief Remove module from allowed modules filter list
 * 
 * @param moduleName Module name to remove
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingRemoveAllowedModule(const char* moduleName);

/**
 * @brief Clear allowed modules filter list
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingClearAllowedModules(void);

/**
 * @brief Set filter by module flag
 * 
 * @param enable true to enable filtering by module, false to disable
 * @return bool Previous state
 */
bool MCP_LoggingSetFilterByModule(bool enable);

/**
 * @brief Handle log event subscription from client
 * 
 * This is called when a client subscribes to log events.
 * 
 * @param sessionId Client session ID
 * @param operationId Operation ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingHandleSubscription(const char* sessionId, const char* operationId);

/**
 * @brief Handle log event unsubscription from client
 * 
 * This is called when a client unsubscribes from log events.
 * 
 * @param sessionId Client session ID
 * @param operationId Operation ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingHandleUnsubscription(const char* sessionId, const char* operationId);

/**
 * @brief Handle log configuration request from client
 * 
 * @param sessionId Client session ID
 * @param operationId Operation ID
 * @param content Configuration content
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingHandleConfig(const char* sessionId, const char* operationId, 
                          const MCP_Content* content);

/**
 * @brief Send current log configuration to client
 * 
 * @param sessionId Client session ID
 * @param operationId Operation ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingSendConfig(const char* sessionId, const char* operationId);

#ifdef __cplusplus
}
#endif

#endif /* MCP_LOGGING_H */