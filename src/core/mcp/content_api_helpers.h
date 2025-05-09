#ifndef MCP_CONTENT_API_HELPERS_H
#define MCP_CONTENT_API_HELPERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Include platform compatibility header if building for Arduino with C++ fixes
#if defined(MCP_PLATFORM_ARDUINO) && defined(MCP_CPP_FIXES)
#include "platform_compatibility.h"
#else
#include "content.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for MCP_Content if needed
#if !defined(MCP_CONTENT_DEFINED) && !defined(MCP_CONTENT_FORWARD_DECLARED)
#define MCP_CONTENT_FORWARD_DECLARED
struct MCP_Content;
#endif

/**
 * @brief Create an object content
 * 
 * @return MCP_Content* New content object or NULL on failure
 */
MCP_Content* MCP_ContentCreateObject(void);

/**
 * @brief Create an array content
 * 
 * @return MCP_Content* New content array or NULL on failure
 */
MCP_Content* MCP_ContentCreateArray(void);

/**
 * @brief Helper function to add string to a content object
 * 
 * @param content Content object
 * @param key Field name
 * @param value String value
 * @return bool True if successful, false otherwise
 */
bool MCP_ContentAddString(MCP_Content* content, const char* key, const char* value);

/**
 * @brief Helper function to add boolean to a content object
 * 
 * @param content Content object
 * @param key Field name
 * @param value Boolean value
 * @return bool True if successful, false otherwise
 */
bool MCP_ContentAddBoolean(MCP_Content* content, const char* key, bool value);

/**
 * @brief Helper function to add number to a content object
 * 
 * @param content Content object
 * @param key Field name
 * @param value Number value
 * @return bool True if successful, false otherwise
 */
bool MCP_ContentAddNumber(MCP_Content* content, const char* key, double value);

/**
 * @brief Helper function to add object to a content object
 * 
 * @param content Content object
 * @param key Field name
 * @param value Object to add
 * @return bool True if successful, false otherwise
 */
bool MCP_ContentAddObject(MCP_Content* content, const char* key, MCP_Content* value);

/**
 * @brief Helper function to add array to a content object
 * 
 * @param content Content object
 * @param key Field name
 * @param array Array to add
 * @return bool True if successful, false otherwise
 */
bool MCP_ContentAddArray(MCP_Content* content, const char* key, MCP_Content* array);

/**
 * @brief Helper function to add string to an array
 * 
 * @param array Array content
 * @param value String value
 * @return bool True if successful, false otherwise
 */
bool MCP_ContentAddArrayString(MCP_Content* array, const char* value);

/**
 * @brief Helper function to get string from content by field name
 * 
 * @param content Content object
 * @param key Field name
 * @param value Pointer to store the string value
 * @return bool True if field exists, false otherwise
 */
bool MCP_ContentGetStringField(const MCP_Content* content, const char* key, const char** value);

/**
 * @brief Arduino-specific version of content string getter with key
 * 
 * @param content Content object
 * @param key Field name
 * @param value Pointer to store the string value
 * @return bool True if field exists, false otherwise
 */
#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
bool MCP_ContentGetStringValue(const MCP_Content* content, const char* key, const char** value);
#endif

/**
 * @brief Helper function to get boolean from a content object
 * 
 * @param content Content object
 * @param key Field name
 * @param value Pointer to store the boolean value
 * @return bool True if field exists, false otherwise
 */
bool MCP_ContentGetBoolean(const MCP_Content* content, const char* key, bool* value);

/**
 * @brief Helper function to get number from a content object
 * 
 * @param content Content object
 * @param key Field name
 * @param value Pointer to store the number value
 * @return bool True if field exists, false otherwise
 */
bool MCP_ContentGetNumber(const MCP_Content* content, const char* key, double* value);

/**
 * @brief Helper function to get object from a content object
 * 
 * @param content Content object
 * @param key Field name
 * @param value Pointer to store the object
 * @return bool True if field exists, false otherwise
 */
bool MCP_ContentGetObject(const MCP_Content* content, const char* key, MCP_Content** value);

/**
 * @brief Helper function to get array from a content object
 * 
 * @param content Content object
 * @param key Field name
 * @param array Pointer to store the array
 * @return bool True if field exists, false otherwise
 */
bool MCP_ContentGetArray(const MCP_Content* content, const char* key, MCP_Content** array);

/**
 * @brief Get string from array at specified index
 * 
 * @param array Array content
 * @param index Index in array
 * @param value Pointer to store the string value
 * @return bool True if successful, false otherwise
 */
bool MCP_ContentGetArrayStringAt(const MCP_Content* array, uint32_t index, const char** value);

/**
 * @brief Get array length
 * 
 * @param array Array content
 * @return uint32_t Array length or 0 if not an array
 */
uint32_t MCP_ContentGetArrayLength(const MCP_Content* array);

#ifdef __cplusplus
}
#endif

#endif /* MCP_CONTENT_API_HELPERS_H */