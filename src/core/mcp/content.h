#ifndef MCP_CONTENT_H
#define MCP_CONTENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Include platform compatibility for MCP_ContentType definition
#include "../../util/platform_compatibility.h"

// For Arduino, include Arduino compatibility header
#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
#include "../../util/arduino_compat.h"
#define MCP_CONTENT_DEFINED // Mark MCP_Content as defined after including arduino_compat.h
#endif

/**
 * @brief MCP content structure
 * 
 * Check if we already have a forward declaration from tool_info.h
 */
#if !defined(MCP_CONTENT_DEFINED) && !defined(MCP_CONTENT_FORWARD_DECLARED) && !defined(MCP_PLATFORM_ARDUINO) && !defined(MCP_OS_ARDUINO)
#define MCP_CONTENT_DEFINED
typedef struct MCP_Content {
    MCP_ContentType type;       // Content type
    uint8_t* data;              // Content data
    size_t size;                // Content size
    char* mediaType;            // Media type string (e.g., "application/json")
    bool ownsData;              // Whether the structure owns the data
} MCP_Content;
#elif defined(MCP_CONTENT_FORWARD_DECLARED) && !defined(MCP_CONTENT_DEFINED) && !defined(MCP_PLATFORM_ARDUINO) && !defined(MCP_OS_ARDUINO)
// Structure has been forward-declared but not defined yet
#define MCP_CONTENT_DEFINED
struct MCP_Content {
    MCP_ContentType type;       // Content type
    uint8_t* data;              // Content data
    size_t size;                // Content size
    char* mediaType;            // Media type string (e.g., "application/json")
    bool ownsData;              // Whether the structure owns the data
};
#endif

#if !defined(MCP_PLATFORM_ARDUINO) || !defined(MCP_CPP_FIXES)
/**
 * @brief Create a new content object
 * 
 * @param type Content type
 * @param data Content data (will be copied)
 * @param size Content size
 * @param mediaType Media type string (will be copied)
 * @return MCP_Content* New content object or NULL on failure
 */
MCP_Content* MCP_ContentCreate(MCP_ContentType type, const uint8_t* data, 
                             size_t size, const char* mediaType);

/**
 * @brief Create a content object from JSON
 * 
 * @param json JSON string
 * @param size JSON string length
 * @return MCP_Content* New content object or NULL on failure
 */
MCP_Content* MCP_ContentCreateFromJson(const char* json, size_t size);

/**
 * @brief Create a content object from text
 * 
 * @param text Text string
 * @param size Text string length
 * @return MCP_Content* New content object or NULL on failure
 */
MCP_Content* MCP_ContentCreateFromText(const char* text, size_t size);

/**
 * @brief Create a content object from binary data
 * 
 * @param data Binary data
 * @param size Binary data size
 * @param mediaType Media type string
 * @return MCP_Content* New content object or NULL on failure
 */
MCP_Content* MCP_ContentCreateFromBinary(const uint8_t* data, size_t size, 
                                      const char* mediaType);

/**
 * @brief Free a content object
 * 
 * @param content Content object to free
 */
void MCP_ContentFree(MCP_Content* content);

/**
 * @brief Get content type as string
 * 
 * @param type Content type
 * @return const char* Type string
 */
const char* MCP_ContentGetTypeString(MCP_ContentType type);

/**
 * @brief Parse content type from string
 * 
 * @param typeString Type string
 * @return MCP_ContentType Content type
 */
MCP_ContentType MCP_ContentParseTypeString(const char* typeString);

/**
 * @brief Parse media type from string
 * 
 * @param mediaType Media type string
 * @return MCP_ContentType Best matching content type
 */
MCP_ContentType MCP_ContentParseMediaType(const char* mediaType);

/**
 * @brief Get default media type for content type
 * 
 * @param type Content type
 * @return const char* Default media type string
 */
const char* MCP_ContentGetDefaultMediaType(MCP_ContentType type);

/**
 * @brief Check if a content object is valid
 * 
 * @param content Content object to check
 * @return bool True if valid, false otherwise
 */
bool MCP_ContentIsValid(const MCP_Content* content);

/**
 * @brief Get content data as string
 * 
 * @param content Content object
 * @return const char* Content data as string or NULL if not text/JSON
 */
const char* MCP_ContentGetString(const MCP_Content* content);

/**
 * @brief Serialize content object to binary format
 * 
 * @param content Content object
 * @param buffer Buffer to store serialized data
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_ContentSerialize(const MCP_Content* content, uint8_t* buffer, size_t bufferSize);

/**
 * @brief Deserialize content object from binary format
 * 
 * @param buffer Serialized data
 * @param size Size of serialized data
 * @return MCP_Content* Deserialized content object or NULL on failure
 */
MCP_Content* MCP_ContentDeserialize(const uint8_t* buffer, size_t size);
#endif // !MCP_PLATFORM_ARDUINO || !MCP_CPP_FIXES

// Forward declare these functions regardless of platform
MCP_Content* MCP_ContentCreate(MCP_ContentType type, const uint8_t* data, size_t size, const char* mediaType);
MCP_Content* MCP_ContentCreateFromJson(const char* json, size_t size);
MCP_Content* MCP_ContentCreateFromText(const char* text, size_t size);
MCP_Content* MCP_ContentCreateFromBinary(const uint8_t* data, size_t size, const char* mediaType);
void MCP_ContentFree(MCP_Content* content);
const char* MCP_ContentGetString(const MCP_Content* content);
const char* MCP_ContentGetDefaultMediaType(MCP_ContentType type);

// Include API helpers after MCP_Content is defined
#include "content_api_helpers.h"

#endif /* MCP_CONTENT_H */