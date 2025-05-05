/**
 * @file logging.c
 * @brief Simple logging implementation for embedded systems
 */
#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

// Log configuration
static LogLevel s_logLevel = LOG_LEVEL_INFO;
static LogHandler s_logHandler = NULL;
static void* s_logContext = NULL;
static bool s_timestampEnabled = true;
static bool s_colorEnabled = true;

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

// Default log handler function
static void defaultLogHandler(LogLevel level, const char* message, void* context) {
    const char* levelStr = "UNKNOWN";
    const char* color = "";
    
    // Set level string and color
    switch (level) {
        case LOG_LEVEL_TRACE:
            levelStr = "TRACE";
            color = COLOR_CYAN;
            break;
        case LOG_LEVEL_DEBUG:
            levelStr = "DEBUG";
            color = COLOR_GREEN;
            break;
        case LOG_LEVEL_INFO:
            levelStr = "INFO";
            color = COLOR_WHITE;
            break;
        case LOG_LEVEL_WARNING:
            levelStr = "WARN";
            color = COLOR_YELLOW;
            break;
        case LOG_LEVEL_ERROR:
            levelStr = "ERROR";
            color = COLOR_RED;
            break;
        case LOG_LEVEL_FATAL:
            levelStr = "FATAL";
            color = COLOR_MAGENTA;
            break;
    }
    
    // Generate timestamp if enabled
    char timestamp[24] = "";
    if (s_timestampEnabled) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S] ", tm_info);
    }
    
    // Print log message
    if (s_colorEnabled) {
        printf("%s%s[%s]%s %s\n", timestamp, color, levelStr, COLOR_RESET, message);
    } else {
        printf("%s[%s] %s\n", timestamp, levelStr, message);
    }
}

void log_init(LogLevel level, LogHandler handler, void* context) {
    s_logLevel = level;
    s_logHandler = handler;
    s_logContext = context;
    
    // Use default handler if none provided
    if (s_logHandler == NULL) {
        s_logHandler = defaultLogHandler;
    }
}

void log_set_level(LogLevel level) {
    s_logLevel = level;
}

LogLevel log_get_level(void) {
    return s_logLevel;
}

void log_set_handler(LogHandler handler, void* context) {
    s_logHandler = handler;
    s_logContext = context;
    
    // Use default handler if none provided
    if (s_logHandler == NULL) {
        s_logHandler = defaultLogHandler;
    }
}

void log_enable_timestamp(bool enable) {
    s_timestampEnabled = enable;
}

void log_enable_color(bool enable) {
    s_colorEnabled = enable;
}

void log_message(LogLevel level, const char* format, ...) {
    if (level < s_logLevel || s_logHandler == NULL) {
        return;
    }
    
    // Format message
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Call log handler
    s_logHandler(level, buffer, s_logContext);
}

void log_trace(const char* format, ...) {
    if (LOG_LEVEL_TRACE < s_logLevel || s_logHandler == NULL) {
        return;
    }
    
    // Format message
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Call log handler
    s_logHandler(LOG_LEVEL_TRACE, buffer, s_logContext);
}

void log_debug(const char* format, ...) {
    if (LOG_LEVEL_DEBUG < s_logLevel || s_logHandler == NULL) {
        return;
    }
    
    // Format message
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Call log handler
    s_logHandler(LOG_LEVEL_DEBUG, buffer, s_logContext);
}

void log_info(const char* format, ...) {
    if (LOG_LEVEL_INFO < s_logLevel || s_logHandler == NULL) {
        return;
    }
    
    // Format message
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Call log handler
    s_logHandler(LOG_LEVEL_INFO, buffer, s_logContext);
}

void log_warning(const char* format, ...) {
    if (LOG_LEVEL_WARNING < s_logLevel || s_logHandler == NULL) {
        return;
    }
    
    // Format message
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Call log handler
    s_logHandler(LOG_LEVEL_WARNING, buffer, s_logContext);
}

void log_error(const char* format, ...) {
    if (LOG_LEVEL_ERROR < s_logLevel || s_logHandler == NULL) {
        return;
    }
    
    // Format message
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Call log handler
    s_logHandler(LOG_LEVEL_ERROR, buffer, s_logContext);
}

void log_fatal(const char* format, ...) {
    if (LOG_LEVEL_FATAL < s_logLevel || s_logHandler == NULL) {
        return;
    }
    
    // Format message
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Call log handler
    s_logHandler(LOG_LEVEL_FATAL, buffer, s_logContext);
}

void log_hex_dump(LogLevel level, const void* data, size_t size, const char* prefix) {
    if (level < s_logLevel || s_logHandler == NULL || data == NULL) {
        return;
    }
    
    const uint8_t* bytes = (const uint8_t*)data;
    char buffer[1024];
    size_t offset = 0;
    
    // Add prefix if provided
    if (prefix != NULL) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s ", prefix);
    }
    
    // Generate hex dump
    for (size_t i = 0; i < size && offset < sizeof(buffer) - 4; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%02X ", bytes[i]);
        
        // Add newline every 16 bytes
        if ((i + 1) % 16 == 0 && i < size - 1) {
            // Call log handler with current line
            s_logHandler(level, buffer, s_logContext);
            
            // Reset buffer
            offset = 0;
            if (prefix != NULL) {
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s ", prefix);
            }
        }
    }
    
    // Call log handler with remaining bytes
    if (offset > 0) {
        s_logHandler(level, buffer, s_logContext);
    }
}