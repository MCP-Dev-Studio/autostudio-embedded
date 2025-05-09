#ifndef LOGGING_H
#define LOGGING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log levels
 */
typedef enum {
    LOG_LEVEL_NONE = 0,    // No logging
    LOG_LEVEL_ERROR = 1,   // Error conditions
    LOG_LEVEL_WARN = 2,    // Warning conditions
    LOG_LEVEL_INFO = 3,    // Informational messages
    LOG_LEVEL_DEBUG = 4,   // Debug-level messages
    LOG_LEVEL_TRACE = 5    // Trace-level messages
} LogLevel;

/**
 * @brief Log output types
 */
typedef enum {
    LOG_OUTPUT_NONE = 0,         // No output
    LOG_OUTPUT_SERIAL = 1 << 0,  // Serial output
    LOG_OUTPUT_MEMORY = 1 << 1,  // In-memory buffer
    LOG_OUTPUT_FILE = 1 << 2,    // File output
    LOG_OUTPUT_NETWORK = 1 << 3, // Network output
    LOG_OUTPUT_CUSTOM = 1 << 4,  // Custom output
    LOG_OUTPUT_CONSOLE = 1 << 5  // Console output (for compatibility)
} LogOutput;

/**
 * @brief Log configuration
 */
typedef struct {
    LogLevel level;               // Logging level
    uint32_t outputs;             // Bit mask of log outputs
    const char* logFileName;      // Log file name (for LOG_OUTPUT_FILE)
    uint32_t maxFileSize;         // Maximum log file size in bytes
    uint32_t maxMemoryEntries;    // Maximum memory buffer entries
    bool includeTimestamp;        // Include timestamp in log entries
    bool includeLevelName;        // Include level name in log entries
    bool includeModuleName;       // Include module name in log entries
    bool colorOutput;             // Enable color output (for supported outputs)
    void (*customLogCallback)(LogLevel level, const char* message); // Custom log callback
} LogConfig;

/**
 * @brief Initialize logging system
 * 
 * @param config Logging configuration
 * @return int 0 on success, negative error code on failure
 */
int log_init(const LogConfig* config);

/**
 * @brief Deinitialize logging system
 * 
 * @return int 0 on success, negative error code on failure
 */
int log_deinit(void);

/**
 * @brief Set active log level
 * 
 * @param level New log level
 * @return LogLevel Previous log level
 */
LogLevel log_set_level(LogLevel level);

/**
 * @brief Get active log level
 * 
 * @return LogLevel Current log level
 */
LogLevel log_get_level(void);

/**
 * @brief Set active log outputs
 * 
 * @param outputs Bit mask of log outputs
 * @return uint32_t Previous log outputs
 */
uint32_t log_set_outputs(uint32_t outputs);

/**
 * @brief Get active log outputs
 * 
 * @return uint32_t Current log outputs
 */
uint32_t log_get_outputs(void);

/**
 * @brief Log a message with specified level
 * 
 * @param level Log level
 * @param module Module name
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void log_message(LogLevel level, const char* module, const char* format, ...);

/**
 * @brief Convenience macros for different log levels
 */
#define LOG_ERROR(module, format, ...) log_message(LOG_LEVEL_ERROR, module, format, ##__VA_ARGS__)
#define LOG_WARN(module, format, ...)  log_message(LOG_LEVEL_WARN, module, format, ##__VA_ARGS__)
#define LOG_INFO(module, format, ...)  log_message(LOG_LEVEL_INFO, module, format, ##__VA_ARGS__)
#define LOG_DEBUG(module, format, ...) log_message(LOG_LEVEL_DEBUG, module, format, ##__VA_ARGS__)
#define LOG_TRACE(module, format, ...) log_message(LOG_LEVEL_TRACE, module, format, ##__VA_ARGS__)

/**
 * @brief Simplified logging functions for Arduino
 * These functions are only available in C++ Arduino code
 */
#if defined(__cplusplus) && (defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO))
void log_error(const char* format, ...);
void log_warn(const char* format, ...);
void log_info(const char* format, ...);
void log_debug(const char* format, ...);
void log_trace(const char* format, ...);
#endif

/**
 * @brief Get memory buffer log entries
 * 
 * @param buffer Buffer to store log entries
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int log_get_memory_entries(char* buffer, size_t bufferSize);

/**
 * @brief Clear memory buffer log entries
 * 
 * @return int 0 on success, negative error code on failure
 */
int log_clear_memory_entries(void);

/**
 * @brief Get memory buffer log entry count
 * 
 * @return int Number of log entries in memory buffer
 */
int log_get_memory_entry_count(void);

/**
 * @brief Flush log outputs
 * 
 * @return int 0 on success, negative error code on failure
 */
int log_flush(void);

/**
 * @brief Set custom log callback
 * 
 * @param callback Custom log callback function
 * @return int 0 on success, negative error code on failure
 */
int log_set_custom_callback(void (*callback)(LogLevel level, const char* message));

/**
 * @brief Get log level name
 * 
 * @param level Log level
 * @return const char* Log level name
 */
const char* log_level_name(LogLevel level);

#ifdef __cplusplus
}
#endif

#endif /* LOGGING_H */