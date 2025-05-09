/**
 * @file content_helpers_arduino.cpp
 * @brief Arduino-specific helper functions for content handling
 */
#include "content.h"
#include "content_api_helpers.h"

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Helper function to get string from content by field name
 * Compatible with Arduino C++ environment
 * 
 * @param content Content object
 * @param key Field name
 * @param value Pointer to store the string value
 * @return bool True if field exists, false otherwise
 */
bool MCP_ContentGetStringValue(const MCP_Content* content, const char* key, const char** value) {
    if (content == NULL || key == NULL || value == NULL) {
        return false;
    }
    
    // Simple implementation for Arduino - just pass through to string field getter
    return MCP_ContentGetStringField(content, key, value);
}

#ifdef __cplusplus
}
#endif

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)