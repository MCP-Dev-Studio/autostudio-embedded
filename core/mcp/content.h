#ifndef MCP_CONTENT_H
#define MCP_CONTENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief MCP content type
 */
typedef enum {
    MCP_CONTENT_TYPE_UNKNOWN,
    MCP_CONTENT_TYPE_TEXT,
    MCP_CONTENT_TYPE_JSON,
    MCP_CONTENT_TYPE_BINARY,
    MCP_CONTENT_TYPE_IMAGE,
    MCP_CONTENT_TYPE_AUDIO,
    MCP_CONTENT_TYPE_VIDEO
} MCP_ContentType;

/**
 * @brief MCP content structure
 */
typedef struct {
    MCP_ContentType type;       // Content type
    uint8_t* data;              // Content data
    size_t size;                // Content size
    char* mediaType;            // Media type string (e.g., "application/json")
    bool ownsData;              // Whether the structure owns the data
} MCP_Content;

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

#endif /* MCP_CONTENT_H */