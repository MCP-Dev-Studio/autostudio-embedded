#include "arduino_compat.h"
/**
 * @file content.c
 * @brief Implementation of MCP_Content for all platforms
 */
#include "content.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Implement for all platforms

/**
 * @brief Create a new content object
 */
MCP_Content* MCP_ContentCreate(MCP_ContentType type, const uint8_t* data, 
                           size_t size, const char* mediaType) {
    MCP_Content* content = (MCP_Content*)malloc(sizeof(MCP_Content));
    if (content == NULL) {
        return NULL;
    }
    
    content->type = type;
    content->size = size;
    content->ownsData = true;
    
    // Copy data if provided
    if (data != NULL && size > 0) {
        content->data = (uint8_t*)malloc(size);
        if (content->data == NULL) {
            free(content);
            return NULL;
        }
        memcpy(content->data, data, size);
    } else {
        content->data = NULL;
    }
    
    // Copy media type if provided
    if (mediaType != NULL) {
        content->mediaType = strdup(mediaType);
    } else {
        content->mediaType = strdup(MCP_ContentGetDefaultMediaType(type));
    }
    
    return content;
}

/**
 * @brief Create a content object from JSON
 */
MCP_Content* MCP_ContentCreateFromJson(const char* json, size_t size) {
    if (json == NULL) {
        return NULL;
    }
    
    // If size is 0, calculate it
    if (size == 0) {
        size = strlen(json);
    }
    
    return MCP_ContentCreate(MCP_CONTENT_TYPE_JSON, (const uint8_t*)json, size, "application/json");
}

/**
 * @brief Create a content object from text
 */
MCP_Content* MCP_ContentCreateFromText(const char* text, size_t size) {
    if (text == NULL) {
        return NULL;
    }
    
    // If size is 0, calculate it
    if (size == 0) {
        size = strlen(text);
    }
    
    return MCP_ContentCreate(MCP_CONTENT_TYPE_TEXT, (const uint8_t*)text, size, "text/plain");
}

/**
 * @brief Create a content object from binary data
 */
MCP_Content* MCP_ContentCreateFromBinary(const uint8_t* data, size_t size, 
                                    const char* mediaType) {
    if (data == NULL || size == 0) {
        return NULL;
    }
    
    return MCP_ContentCreate(MCP_CONTENT_TYPE_BINARY, data, size, mediaType);
}

/**
 * @brief Free a content object
 */
void MCP_ContentFree(MCP_Content* content) {
    if (content == NULL) {
        return;
    }
    
    if (content->ownsData && content->data != NULL) {
        free(content->data);
    }
    
    if (content->mediaType != NULL) {
        free(content->mediaType);
    }
    
    free(content);
}

/**
 * @brief Get content type as string
 */
const char* MCP_ContentGetTypeString(MCP_ContentType type) {
    switch (type) {
        case MCP_CONTENT_TYPE_TEXT:
            return "text";
        case MCP_CONTENT_TYPE_JSON:
            return "json";
        case MCP_CONTENT_TYPE_BINARY:
            return "binary";
        case MCP_CONTENT_TYPE_IMAGE:
            return "image";
        case MCP_CONTENT_TYPE_AUDIO:
            return "audio";
        case MCP_CONTENT_TYPE_VIDEO:
            return "video";
        default:
            return "unknown";
    }
}

/**
 * @brief Parse content type from string
 */
MCP_ContentType MCP_ContentParseTypeString(const char* typeString) {
    if (typeString == NULL) {
        return MCP_CONTENT_TYPE_UNKNOWN;
    }
    
    if (strcmp(typeString, "text") == 0) {
        return MCP_CONTENT_TYPE_TEXT;
    } else if (strcmp(typeString, "json") == 0) {
        return MCP_CONTENT_TYPE_JSON;
    } else if (strcmp(typeString, "binary") == 0) {
        return MCP_CONTENT_TYPE_BINARY;
    } else if (strcmp(typeString, "image") == 0) {
        return MCP_CONTENT_TYPE_IMAGE;
    } else if (strcmp(typeString, "audio") == 0) {
        return MCP_CONTENT_TYPE_AUDIO;
    } else if (strcmp(typeString, "video") == 0) {
        return MCP_CONTENT_TYPE_VIDEO;
    }
    
    return MCP_CONTENT_TYPE_UNKNOWN;
}

/**
 * @brief Parse media type from string
 */
MCP_ContentType MCP_ContentParseMediaType(const char* mediaType) {
    if (mediaType == NULL) {
        return MCP_CONTENT_TYPE_UNKNOWN;
    }
    
    if (strncmp(mediaType, "text/", 5) == 0) {
        return MCP_CONTENT_TYPE_TEXT;
    } else if (strcmp(mediaType, "application/json") == 0) {
        return MCP_CONTENT_TYPE_JSON;
    } else if (strncmp(mediaType, "image/", 6) == 0) {
        return MCP_CONTENT_TYPE_IMAGE;
    } else if (strncmp(mediaType, "audio/", 6) == 0) {
        return MCP_CONTENT_TYPE_AUDIO;
    } else if (strncmp(mediaType, "video/", 6) == 0) {
        return MCP_CONTENT_TYPE_VIDEO;
    } else if (strncmp(mediaType, "application/", 12) == 0) {
        return MCP_CONTENT_TYPE_BINARY;
    }
    
    return MCP_CONTENT_TYPE_BINARY;
}

/**
 * @brief Get default media type for content type
 */
const char* MCP_ContentGetDefaultMediaType(MCP_ContentType type) {
    switch (type) {
        case MCP_CONTENT_TYPE_TEXT:
            return "text/plain";
        case MCP_CONTENT_TYPE_JSON:
            return "application/json";
        case MCP_CONTENT_TYPE_BINARY:
            return "application/octet-stream";
        case MCP_CONTENT_TYPE_IMAGE:
            return "image/png";
        case MCP_CONTENT_TYPE_AUDIO:
            return "audio/mpeg";
        case MCP_CONTENT_TYPE_VIDEO:
            return "video/mp4";
        default:
            return "application/octet-stream";
    }
}

/**
 * @brief Check if a content object is valid
 */
bool MCP_ContentIsValid(const MCP_Content* content) {
    return content != NULL && 
           (content->size == 0 || (content->data != NULL && content->size > 0)) &&
           content->mediaType != NULL;
}

/**
 * @brief Get content data as string
 */
const char* MCP_ContentGetString(const MCP_Content* content) {
    if (content == NULL || content->data == NULL) {
        return NULL;
    }
    
    if (content->type == MCP_CONTENT_TYPE_TEXT || 
        content->type == MCP_CONTENT_TYPE_JSON) {
        return (const char*)content->data;
    }
    
    return NULL;
}

/**
 * @brief Serialize content object to binary format - stub implementation
 */
int MCP_ContentSerialize(const MCP_Content* content, uint8_t* buffer, size_t bufferSize) {
    if (content == NULL || buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // This is a simplified implementation for the host platform
    // For a real implementation, this would handle proper serialization
    
    // Check if there's enough room in the buffer
    size_t requiredSize = sizeof(uint32_t) + content->size + 
                         (content->mediaType ? strlen(content->mediaType) + 1 : 1);
    
    if (bufferSize < requiredSize) {
        return -2;
    }
    
    // For the host platform stub, just copy the data if it fits
    if (content->data != NULL && content->size > 0) {
        if (content->size <= bufferSize) {
            memcpy(buffer, content->data, content->size);
            return content->size;
        }
    }
    
    return 0;
}

/**
 * @brief Deserialize content object from binary format - stub implementation
 */
MCP_Content* MCP_ContentDeserialize(const uint8_t* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return NULL;
    }
    
    // This is a simplified implementation for the host platform
    // For a real implementation, this would handle proper deserialization
    
    // For the host platform stub, just treat the data as binary
    return MCP_ContentCreateFromBinary(buffer, size, "application/octet-stream");
}

// Content API methods are now defined in content_api_helpers.c
// This file only defines the core content functionality
