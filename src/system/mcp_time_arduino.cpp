/**
 * @file mcp_time_arduino.cpp
 * @brief Time utilities for Arduino
 */

#include <stdint.h>
#include <time.h>

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get current time in milliseconds
 * 
 * @return uint64_t Current time in milliseconds
 */
uint64_t MCP_GetCurrentTimeMs(void) {
    // On Arduino, we can use millis() function which is available in Arduino.h
    // For this stub implementation, we'll use time(NULL) * 1000
    return (uint64_t)time(NULL) * 1000;
}

#ifdef __cplusplus
}
#endif

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)