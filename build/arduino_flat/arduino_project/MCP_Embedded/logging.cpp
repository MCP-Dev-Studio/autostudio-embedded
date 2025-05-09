#include "arduino_compat.h"
/**
 * @file logging.c
 * @brief Simple logging implementation for embedded systems
 */
#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

// Define LogHandler type
typedef void (*LogHandler)(LogLevel level, const char* message, void* context);

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
    
    // Unused parameter
    (void)context;
    
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
        case LOG_LEVEL_WARN:
            levelStr = "WARN";
            color = COLOR_YELLOW;
            break;
        case LOG_LEVEL_ERROR:
            levelStr = "ERROR";
            color = COLOR_RED;
            break;
        case LOG_LEVEL_NONE:
            levelStr = "NONE";
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

int log_init(const LogConfig* config) {
    // Basic implementation for compatibility
    s_logLevel = config ? config->level : LOG_LEVEL_INFO;
    s_logHandler = defaultLogHandler;
    s_timestampEnabled = config ? config->includeTimestamp : true;
    s_colorEnabled = config ? config->colorOutput : true;
    
    return 0;
}

int log_deinit(void) {
    // Nothing to do for basic implementation
    return 0;
}

LogLevel log_set_level(LogLevel level) {
    LogLevel previous = s_logLevel;
    s_logLevel = level;
    return previous;
}

LogLevel log_get_level(void) {
    return s_logLevel;
}

void log_message(LogLevel level, const char* module, const char* format, ...) {
    if (level > s_logLevel || s_logHandler == NULL) {
        return; // Higher log level than current or no handler
    }
    
    // Format message with module prefix if provided
    char buffer[1024];
    char fullMessage[1152]; // buffer + prefix + margin
    
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (module && *module) {
        snprintf(fullMessage, sizeof(fullMessage), "[%s] %s", module, buffer);
    } else {
        strncpy(fullMessage, buffer, sizeof(fullMessage) - 1);
        fullMessage[sizeof(fullMessage) - 1] = '\0';
    }
    
    // Call log handler
    s_logHandler(level, fullMessage, s_logContext);
}

// Utility function to get level name
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

// Additional functions not required for basic build
int log_flush(void) { return 0; }
int log_get_memory_entries(char* buffer, size_t bufferSize) { (void)buffer; (void)bufferSize; return 0; }
int log_clear_memory_entries(void) { return 0; }
int log_get_memory_entry_count(void) { return 0; }
uint32_t log_set_outputs(uint32_t outputs) { (void)outputs; return 0; }
uint32_t log_get_outputs(void) { return 0; }
int log_set_custom_callback(void (*callback)(LogLevel, const char*)) { (void)callback; return 0; }
