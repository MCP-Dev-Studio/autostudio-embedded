#include "arduino_compat.h"
#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

// Simple stub implementation of the logging system for Arduino
static LogConfig s_log_config = {
    .level = LOG_LEVEL_INFO,
    .outputs = LOG_OUTPUT_SERIAL,
    .logFileName = NULL,
    .maxFileSize = 0,
    .maxMemoryEntries = 100,
    .includeTimestamp = true,
    .includeLevelName = true,
    .includeModuleName = true,
    .colorOutput = true,
    .customLogCallback = NULL
};

/**
 * @brief Initialize logging system
 */
int log_init(const LogConfig* config) {
    if (config != NULL) {
        memcpy(&s_log_config, config, sizeof(LogConfig));
    }
    return 0;
}

/**
 * @brief Deinitialize logging system
 */
int log_deinit(void) {
    return 0;
}

/**
 * @brief Set active log level
 */
LogLevel log_set_level(LogLevel level) {
    LogLevel previous = s_log_config.level;
    s_log_config.level = level;
    return previous;
}

/**
 * @brief Get active log level
 */
LogLevel log_get_level(void) {
    return s_log_config.level;
}

/**
 * @brief Set active log outputs
 */
uint32_t log_set_outputs(uint32_t outputs) {
    uint32_t previous = s_log_config.outputs;
    s_log_config.outputs = outputs;
    return previous;
}

/**
 * @brief Get active log outputs
 */
uint32_t log_get_outputs(void) {
    return s_log_config.outputs;
}

/**
 * @brief Log a message with specified level
 */
void log_message(LogLevel level, const char* module, const char* format, ...) {
    // Skip if level is higher than configured level
    if (level > s_log_config.level) {
        return;
    }

    // Skip if no outputs are configured
    if (s_log_config.outputs == LOG_OUTPUT_NONE) {
        return;
    }

    // Format the message
    char buffer[256];
    size_t pos = 0;

    // Add timestamp if configured
    if (s_log_config.includeTimestamp) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        pos += strftime(buffer + pos, sizeof(buffer) - pos, "[%Y-%m-%d %H:%M:%S] ", tm_info);
    }

    // Add level name if configured
    if (s_log_config.includeLevelName) {
        const char* level_name = log_level_name(level);
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "[%s] ", level_name);
    }

    // Add module name if configured
    if (s_log_config.includeModuleName && module != NULL) {
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "[%s] ", module);
    }

    // Add the message
    va_list args;
    va_start(args, format);
    pos += vsnprintf(buffer + pos, sizeof(buffer) - pos, format, args);
    va_end(args);

    // Output to serial if configured
    if (s_log_config.outputs & LOG_OUTPUT_SERIAL) {
        // Just use printf for C code
        printf("%s\n", buffer);
    }

    // Output to custom callback if configured
    if ((s_log_config.outputs & LOG_OUTPUT_CUSTOM) && s_log_config.customLogCallback != NULL) {
        s_log_config.customLogCallback(level, buffer);
    }
}

/**
 * @brief Get log level name
 */
const char* log_level_name(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_NONE:  return "NONE";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_TRACE: return "TRACE";
        default:              return "UNKNOWN";
    }
}

/**
 * @brief Get memory buffer log entries
 */
int log_get_memory_entries(char* buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // This is a stub implementation
    const char* stub_message = "Memory buffer log not implemented in stub logging";
    size_t len = strlen(stub_message);
    
    if (len >= bufferSize) {
        return -2;
    }
    
    strcpy(buffer, stub_message);
    return (int)len;
}

/**
 * @brief Clear memory buffer log entries
 */
int log_clear_memory_entries(void) {
    // This is a stub implementation
    return 0;
}

/**
 * @brief Get memory buffer log entry count
 */
int log_get_memory_entry_count(void) {
    // This is a stub implementation
    return 0;
}

/**
 * @brief Flush log outputs
 */
int log_flush(void) {
    // This is a stub implementation
    return 0;
}

/**
 * @brief Set custom log callback
 */
int log_set_custom_callback(void (*callback)(LogLevel level, const char* message)) {
    s_log_config.customLogCallback = callback;
    return 0;
}

// Helper function for Arduino library stubs
void LogInit(LogLevel level) {
    LogConfig config;
    config.level = level;
    config.outputs = LOG_OUTPUT_SERIAL;
    config.logFileName = NULL;
    config.maxFileSize = 0;
    config.maxMemoryEntries = 100;
    config.includeTimestamp = true;
    config.includeLevelName = true;
    config.includeModuleName = true;
    config.colorOutput = true;
    config.customLogCallback = NULL;
    
    log_init(&config);
}

// Helper function for Arduino library stubs
void LogSetLevel(LogLevel level) {
    log_set_level(level);
}
