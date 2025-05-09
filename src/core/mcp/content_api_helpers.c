#include "content_api_helpers.h"
#include "content.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Common implementations for all platforms
#if defined(MCP_PLATFORM_HOST) || defined(MCP_OS_ESP32) || defined(MCP_OS_ARDUINO)

/**
 * @brief Create an object content
 */
MCP_Content* MCP_ContentCreateObject(void) {
    // Create an empty JSON object
    const char* emptyObject = "{}";
    return MCP_ContentCreateFromJson(emptyObject, 2);
}

/**
 * @brief Create an array content
 */
MCP_Content* MCP_ContentCreateArray(void) {
    // Create an empty JSON array
    const char* emptyArray = "[]";
    return MCP_ContentCreateFromJson(emptyArray, 2);
}

/**
 * @brief Helper function to add string to a content object
 */
bool MCP_ContentAddString(MCP_Content* content, const char* key, const char* value) {
    if (content == NULL || key == NULL || value == NULL) {
        return false;
    }
    
    #if defined(MCP_PLATFORM_HOST)
    // This is a stub implementation for the host platform
    printf("MCP_ContentAddString: key=%s, value=%s\n", key, value);
    #endif
    
    return true;
}

/**
 * @brief Helper function to add boolean to a content object
 */
bool MCP_ContentAddBoolean(MCP_Content* content, const char* key, bool value) {
    if (content == NULL || key == NULL) {
        return false;
    }
    
    #if defined(MCP_PLATFORM_HOST)
    // This is a stub implementation for the host platform
    printf("MCP_ContentAddBoolean: key=%s, value=%s\n", key, value ? "true" : "false");
    #endif
    
    return true;
}

/**
 * @brief Helper function to add number to a content object
 */
bool MCP_ContentAddNumber(MCP_Content* content, const char* key, double value) {
    if (content == NULL || key == NULL) {
        return false;
    }
    
    #if defined(MCP_PLATFORM_HOST)
    // This is a stub implementation for the host platform
    printf("MCP_ContentAddNumber: key=%s, value=%f\n", key, value);
    #endif
    
    return true;
}

/**
 * @brief Helper function to add object to a content object
 */
bool MCP_ContentAddObject(MCP_Content* content, const char* key, MCP_Content* value) {
    if (content == NULL || key == NULL || value == NULL) {
        return false;
    }
    
    #if defined(MCP_PLATFORM_HOST)
    // This is a stub implementation for the host platform
    printf("MCP_ContentAddObject: key=%s\n", key);
    #endif
    
    return true;
}

/**
 * @brief Helper function to add array to a content object
 */
bool MCP_ContentAddArray(MCP_Content* content, const char* key, MCP_Content* array) {
    if (content == NULL || key == NULL || array == NULL) {
        return false;
    }
    
    #if defined(MCP_PLATFORM_HOST)
    // This is a stub implementation for the host platform
    printf("MCP_ContentAddArray: key=%s\n", key);
    #endif
    
    return true;
}

/**
 * @brief Helper function to add string to an array
 */
bool MCP_ContentAddArrayString(MCP_Content* array, const char* value) {
    if (array == NULL || value == NULL) {
        return false;
    }
    
    #if defined(MCP_PLATFORM_HOST)
    // This is a stub implementation for the host platform
    printf("MCP_ContentAddArrayString: value=%s\n", value);
    #endif
    
    return true;
}

/**
 * @brief Helper function to get string from content by field name
 */
bool MCP_ContentGetStringField(const MCP_Content* content, const char* key, const char** value) {
    if (content == NULL || key == NULL || value == NULL) {
        return false;
    }
    
    #if defined(MCP_PLATFORM_HOST)
    // This is a stub implementation for the host platform
    // In a real implementation, this would properly extract from the JSON structure
    *value = "stub_value";
    #else
    // For other platforms, return a placeholder
    *value = "";
    #endif
    
    return true;
}

/**
 * @brief Helper function to get boolean from a content object
 */
bool MCP_ContentGetBoolean(const MCP_Content* content, const char* key, bool* value) {
    if (content == NULL || key == NULL || value == NULL) {
        return false;
    }
    
    // Default value
    *value = false;
    
    return true;
}

/**
 * @brief Helper function to get number from a content object
 */
bool MCP_ContentGetNumber(const MCP_Content* content, const char* key, double* value) {
    if (content == NULL || key == NULL || value == NULL) {
        return false;
    }
    
    // Default value
    *value = 0.0;
    
    return true;
}

/**
 * @brief Helper function to get object from a content object
 */
bool MCP_ContentGetObject(const MCP_Content* content, const char* key, MCP_Content** value) {
    if (content == NULL || key == NULL || value == NULL) {
        return false;
    }
    
    // In a real implementation, this would properly extract from the JSON structure
    // For now, just return success but set value to NULL
    *value = NULL;
    
    return true;
}

/**
 * @brief Helper function to get array from a content object
 */
bool MCP_ContentGetArray(const MCP_Content* content, const char* key, MCP_Content** array) {
    if (content == NULL || key == NULL || array == NULL) {
        return false;
    }
    
    // In a real implementation, this would properly extract from the JSON structure
    // For now, just return success but set array to NULL
    *array = NULL;
    
    return true;
}

/**
 * @brief Get string from array at specified index
 */
bool MCP_ContentGetArrayStringAt(const MCP_Content* array, uint32_t index, const char** value) {
    if (array == NULL || value == NULL) {
        return false;
    }
    
    // In a real implementation, this would properly extract from the JSON structure
    // For now, just return a stub value
    *value = "array_item";
    
    return true;
}

/**
 * @brief Get array length
 */
uint32_t MCP_ContentGetArrayLength(const MCP_Content* array) {
    if (array == NULL) {
        return 0;
    }
    
    // In a real implementation, this would properly get the array length
    // For now, just return a stub value
    return 0;
}

#endif // MCP_PLATFORM_HOST || MCP_OS_ESP32 || MCP_OS_ARDUINO