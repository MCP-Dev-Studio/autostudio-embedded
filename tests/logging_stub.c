#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../src/system/logging.h"

// Basic stubs for the logging functions

void log_message(LogLevel level, const char* module, const char* format, ...) {
    const char* level_str = "UNKNOWN";
    switch (level) {
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        case LOG_LEVEL_WARN:  level_str = "WARN"; break;
        case LOG_LEVEL_INFO:  level_str = "INFO"; break;
        case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
        case LOG_LEVEL_TRACE: level_str = "TRACE"; break;
        default: break;
    }
    
    va_list args;
    va_start(args, format);
    
    // For non-Arduino platforms
#ifndef MCP_PLATFORM_ARDUINO
    printf("[%s] ", level_str);
    if (module != NULL) {
        printf("[%s] ", module);
    }
    vprintf(format, args);
    printf("\n");
#else
    // For Arduino platforms
    char buffer[256];
    int len = 0;
    
    len += snprintf(buffer + len, sizeof(buffer) - len, "[%s] ", level_str);
    if (module != NULL) {
        len += snprintf(buffer + len, sizeof(buffer) - len, "[%s] ", module);
    }
    
    len += vsnprintf(buffer + len, sizeof(buffer) - len, format, args);
    
    // On Arduino, use Serial.println
    // Serial.println(buffer);
#endif
    
    va_end(args);
}

// Simple aliases for log_message
void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("[ERROR] ");
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

void log_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("[WARN] ");
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("[INFO] ");
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

void log_debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("[DEBUG] ");
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

void log_trace(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("[TRACE] ");
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

// Other logging functions
int log_init(const LogConfig* config) {
    (void)config; // Unused
    return 0;
}

int log_deinit(void) {
    return 0;
}

LogLevel log_set_level(LogLevel level) {
    return level; // Just return the same level
}

LogLevel log_get_level(void) {
    return LOG_LEVEL_INFO; // Default to INFO
}

// Simple stubs for persistent storage functions
int persistent_storage_write(const char* key, const void* data, size_t size) {
    FILE* fp = fopen(key, "wb");
    if (fp == NULL) {
        return -1;
    }
    
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    
    return (written == size) ? 0 : -2;
}

int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize) {
    FILE* fp = fopen(key, "rb");
    if (fp == NULL) {
        *actualSize = 0;
        return -1;
    }
    
    *actualSize = fread(data, 1, maxSize, fp);
    fclose(fp);
    
    return 0;
}