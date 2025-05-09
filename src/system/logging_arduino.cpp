/**
 * @file logging_arduino.cpp
 * @brief Arduino-specific logging implementation
 */
#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

// Arduino-specific logging functions
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

// Define log output types for Arduino
#define LOG_OUTPUT_CONSOLE 1
#define LOG_OUTPUT_FILE 2
#define LOG_OUTPUT_SERIAL 4
#define LOG_OUTPUT_MEMORY 8
#define LOG_OUTPUT_CUSTOM 16

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)