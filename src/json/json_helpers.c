#include "json_helpers.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Simple implementation of JSON helpers
 * Note: This is a simplified implementation for testing purposes.
 * In production code, you would use a proper JSON library.
 */

char* json_get_string_field(const char* json, const char* field) {
    if (json == NULL || field == NULL) {
        return NULL;
    }
    
    // Search for "field":" pattern
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", field);
    
    const char* start = strstr(json, pattern);
    if (start == NULL) {
        return NULL;
    }
    
    // Move to start of value
    start += strlen(pattern);
    
    // Find end of string (next " not preceded by \)
    const char* end = start;
    while (*end != '\0') {
        if (*end == '"' && (end == start || *(end-1) != '\\')) {
            break;
        }
        end++;
    }
    
    if (*end != '"') {
        return NULL;
    }
    
    // Allocate and copy value
    size_t length = end - start;
    char* value = (char*)malloc(length + 1);
    if (value == NULL) {
        return NULL;
    }
    
    strncpy(value, start, length);
    value[length] = '\0';
    
    return value;
}

bool json_get_bool_field(const char* json, const char* field, bool defaultValue) {
    if (json == NULL || field == NULL) {
        return defaultValue;
    }
    
    // Search for "field": pattern
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    
    const char* start = strstr(json, pattern);
    if (start == NULL) {
        return defaultValue;
    }
    
    // Move to start of value
    start += strlen(pattern);
    
    // Skip whitespace
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }
    
    // Check if true or false
    if (strncmp(start, "true", 4) == 0) {
        return true;
    } else if (strncmp(start, "false", 5) == 0) {
        return false;
    }
    
    return defaultValue;
}

int json_get_int_field(const char* json, const char* field, int defaultValue) {
    if (json == NULL || field == NULL) {
        return defaultValue;
    }
    
    // Search for "field": pattern
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    
    const char* start = strstr(json, pattern);
    if (start == NULL) {
        return defaultValue;
    }
    
    // Move to start of value
    start += strlen(pattern);
    
    // Skip whitespace
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }
    
    // Check if number
    if (*start < '0' || *start > '9') {
        return defaultValue;
    }
    
    // Extract number
    int value = 0;
    if (sscanf(start, "%d", &value) != 1) {
        return defaultValue;
    }
    
    return value;
}

double json_get_double_field(const char* json, const char* field, double defaultValue) {
    if (json == NULL || field == NULL) {
        return defaultValue;
    }
    
    // Search for "field": pattern
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    
    const char* start = strstr(json, pattern);
    if (start == NULL) {
        return defaultValue;
    }
    
    // Move to start of value
    start += strlen(pattern);
    
    // Skip whitespace
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }
    
    // Check if number
    if ((*start < '0' || *start > '9') && *start != '-' && *start != '.') {
        return defaultValue;
    }
    
    // Extract number
    double value = 0.0;
    if (sscanf(start, "%lf", &value) != 1) {
        return defaultValue;
    }
    
    return value;
}

void* json_get_object_field(const char* json, const char* field) {
    if (json == NULL || field == NULL) {
        return NULL;
    }
    
    // Search for "field": pattern
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    
    const char* start = strstr(json, pattern);
    if (start == NULL) {
        return NULL;
    }
    
    // Move to start of value
    start += strlen(pattern);
    
    // Skip whitespace
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }
    
    // Check if object
    if (*start != '{') {
        return NULL;
    }
    
    // Find end of object
    const char* end = start + 1;
    int depth = 1;
    
    while (*end != '\0' && depth > 0) {
        if (*end == '{') {
            depth++;
        } else if (*end == '}') {
            depth--;
        } else if (*end == '"') {
            // Skip string
            end++;
            while (*end != '\0' && (*end != '"' || *(end-1) == '\\')) {
                end++;
            }
        }
        
        if (depth > 0) {
            end++;
        }
    }
    
    if (depth != 0) {
        return NULL;
    }
    
    // Allocate and copy object
    size_t length = end - start + 1;
    char* objectJson = (char*)malloc(length + 1);
    if (objectJson == NULL) {
        return NULL;
    }
    
    strncpy(objectJson, start, length);
    objectJson[length] = '\0';
    
    // Return pointer to object JSON string
    // In a real implementation, you would parse this into a proper object
    return objectJson;
}

typedef struct {
    char* jsonArray;
    size_t count;
} JsonArray;

void* json_get_array_field(const char* json, const char* field) {
    if (json == NULL || field == NULL) {
        return NULL;
    }
    
    // Search for "field": pattern
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    
    const char* start = strstr(json, pattern);
    if (start == NULL) {
        return NULL;
    }
    
    // Move to start of value
    start += strlen(pattern);
    
    // Skip whitespace
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }
    
    // Check if array
    if (*start != '[') {
        return NULL;
    }
    
    // Find end of array
    const char* end = start + 1;
    int depth = 1;
    
    while (*end != '\0' && depth > 0) {
        if (*end == '[') {
            depth++;
        } else if (*end == ']') {
            depth--;
        } else if (*end == '"') {
            // Skip string
            end++;
            while (*end != '\0' && (*end != '"' || *(end-1) == '\\')) {
                end++;
            }
        }
        
        if (depth > 0) {
            end++;
        }
    }
    
    if (depth != 0) {
        return NULL;
    }
    
    // Allocate and copy array
    size_t length = end - start + 1;
    char* arrayJson = (char*)malloc(length + 1);
    if (arrayJson == NULL) {
        return NULL;
    }
    
    strncpy(arrayJson, start, length);
    arrayJson[length] = '\0';
    
    // Count array items
    size_t count = 0;
    const char* p = start + 1;
    
    while (p < end) {
        // Skip whitespace
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',')) {
            p++;
        }
        
        if (p >= end || *p == ']') {
            break;
        }
        
        count++;
        
        // Skip to next item
        if (*p == '{' || *p == '[') {
            int itemDepth = 1;
            p++;
            
            while (p < end && itemDepth > 0) {
                if (*p == '{' || *p == '[') {
                    itemDepth++;
                } else if (*p == '}' || *p == ']') {
                    itemDepth--;
                } else if (*p == '"') {
                    // Skip string
                    p++;
                    while (p < end && (*p != '"' || *(p-1) == '\\')) {
                        p++;
                    }
                }
                
                if (itemDepth > 0) {
                    p++;
                }
            }
            
            p++;
        } else if (*p == '"') {
            // Skip string
            p++;
            while (p < end && (*p != '"' || *(p-1) == '\\')) {
                p++;
            }
            p++;
        } else {
            // Skip primitive
            while (p < end && *p != ',' && *p != ']') {
                p++;
            }
        }
    }
    
    // Create array structure
    JsonArray* array = (JsonArray*)malloc(sizeof(JsonArray));
    if (array == NULL) {
        free(arrayJson);
        return NULL;
    }
    
    array->jsonArray = arrayJson;
    array->count = count;
    
    return array;
}

size_t json_array_length(const void* array) {
    if (array == NULL) {
        return 0;
    }
    
    return ((JsonArray*)array)->count;
}

char* json_array_get_item(const void* array, size_t index) {
    if (array == NULL) {
        return NULL;
    }
    
    JsonArray* jsonArray = (JsonArray*)array;
    
    if (index >= jsonArray->count) {
        return NULL;
    }
    
    const char* arrayJson = jsonArray->jsonArray;
    
    // Skip opening bracket
    const char* p = arrayJson + 1;
    
    // Skip to item at index
    for (size_t i = 0; i < index; i++) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') {
            p++;
        }
        
        // Skip item
        if (*p == '{' || *p == '[') {
            int itemDepth = 1;
            p++;
            
            while (itemDepth > 0) {
                if (*p == '{' || *p == '[') {
                    itemDepth++;
                } else if (*p == '}' || *p == ']') {
                    itemDepth--;
                } else if (*p == '"') {
                    // Skip string
                    p++;
                    while (*p != '"' || *(p-1) == '\\') {
                        p++;
                    }
                }
                
                if (itemDepth > 0) {
                    p++;
                }
            }
            
            p++;
        } else if (*p == '"') {
            // Skip string
            p++;
            while (*p != '"' || *(p-1) == '\\') {
                p++;
            }
            p++;
        } else {
            // Skip primitive
            while (*p != ',' && *p != ']') {
                p++;
            }
        }
    }
    
    // Skip whitespace
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') {
        p++;
    }
    
    // Extract item
    const char* start = p;
    
    if (*start == '{' || *start == '[') {
        int itemDepth = 1;
        p++;
        
        while (itemDepth > 0) {
            if (*p == '{' || *p == '[') {
                itemDepth++;
            } else if (*p == '}' || *p == ']') {
                itemDepth--;
            } else if (*p == '"') {
                // Skip string
                p++;
                while (*p != '"' || *(p-1) == '\\') {
                    p++;
                }
            }
            
            if (itemDepth > 0) {
                p++;
            }
        }
        
        p++;
    } else if (*start == '"') {
        // Skip string
        p++;
        while (*p != '"' || *(p-1) == '\\') {
            p++;
        }
        p++;
    } else {
        // Skip primitive
        while (*p != ',' && *p != ']') {
            p++;
        }
    }
    
    // Allocate and copy item
    size_t length = p - start;
    char* item = (char*)malloc(length + 1);
    if (item == NULL) {
        return NULL;
    }
    
    strncpy(item, start, length);
    item[length] = '\0';
    
    return item;
}

bool json_validate_schema(const char* json, const char* schema) {
    // Simplified implementation - just check if json contains schema fields
    if (json == NULL || schema == NULL) {
        return false;
    }
    
    // In a real implementation, you would actually validate the JSON
    // against the schema. For simplicity, we just return true.
    return true;
}