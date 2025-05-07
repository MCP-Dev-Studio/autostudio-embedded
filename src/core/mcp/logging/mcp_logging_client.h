#ifndef MCP_LOGGING_CLIENT_H
#define MCP_LOGGING_CLIENT_H

#include "../../../system/logging.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log output destination for client
 */
typedef enum {
    MCP_LOG_OUTPUT_CONSOLE = 0,  // Output logs to console (standard output)
    MCP_LOG_OUTPUT_FILE = 1,     // Output logs to file
    MCP_LOG_OUTPUT_MEMORY = 2,   // Output logs to memory buffer
    MCP_LOG_OUTPUT_CUSTOM = 3    // Output logs to custom handler
} MCP_LogOutputDest;

/**
 * @brief Client log configuration
 */
typedef struct {
    bool enabled;                // Whether logging is enabled
    LogLevel level;              // Maximum log level to display
    MCP_LogOutputDest output;    // Log output destination
    char* filePath;              // Log file path (for MCP_LOG_OUTPUT_FILE)
    uint32_t maxFileSize;        // Maximum log file size in bytes
    uint32_t maxMemoryEntries;   // Maximum memory buffer entries (for MCP_LOG_OUTPUT_MEMORY)
    bool includeTimestamp;       // Include timestamp in log entries
    bool includeLevelName;       // Include level name in log entries
    bool includeModuleName;      // Include module name in log entries
    bool colorOutput;            // Enable color output (for supported outputs)
    void (*customHandler)(LogLevel level, const char* module, 
                         const char* message, uint32_t timestamp); // Custom log handler
} MCP_LogClientConfig;

/**
 * @brief Initialize client log configuration
 * 
 * @param config Log configuration or NULL for default
 * @return int 0 on success, negative error code on failure
 */
int MCP_LogClientInit(const MCP_LogClientConfig* config);

/**
 * @brief Get current client log configuration
 * 
 * @param config Pointer to structure to receive configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_LogClientGetConfig(MCP_LogClientConfig* config);

/**
 * @brief Set client log configuration
 * 
 * @param config New log configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_LogClientSetConfig(const MCP_LogClientConfig* config);

/**
 * @brief Enable or disable client logging
 * 
 * @param enable true to enable, false to disable
 * @return bool Previous state
 */
bool MCP_LogClientEnable(bool enable);

/**
 * @brief Set client log level
 * 
 * @param level New log level
 * @return LogLevel Previous log level
 */
LogLevel MCP_LogClientSetLevel(LogLevel level);

/**
 * @brief Set client log output destination
 * 
 * @param output New output destination
 * @return MCP_LogOutputDest Previous output destination
 */
MCP_LogOutputDest MCP_LogClientSetOutput(MCP_LogOutputDest output);

/**
 * @brief Set custom log handler
 * 
 * @param handler Custom log handler function
 * @return int 0 on success, negative error code on failure
 */
int MCP_LogClientSetCustomHandler(void (*handler)(LogLevel level, const char* module,
                                                const char* message, uint32_t timestamp));

/**
 * @brief Process incoming log event from server
 * 
 * @param level Log level
 * @param module Module name
 * @param message Log message
 * @param timestamp Log timestamp
 * @return int 0 on success, negative error code on failure
 */
int MCP_LogClientProcessEvent(LogLevel level, const char* module,
                            const char* message, uint32_t timestamp);

/**
 * @brief Get default client log configuration
 * 
 * @return MCP_LogClientConfig Default configuration
 */
MCP_LogClientConfig MCP_LogClientGetDefaultConfig(void);

#ifdef __cplusplus
}
#endif

#endif /* MCP_LOGGING_CLIENT_H */