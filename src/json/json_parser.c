/**
 * @file json_parser.c
 * @brief JSON parser implementation for MCP
 * 
 * This file provides a lightweight JSON parser implementation
 * optimized for embedded systems with limited resources.
 */

// Define that this file implements the JSON helper functions
#define MCP_IMPL_JSON_FUNCTIONS

#include "json_parser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "../util/build_config.h"

// Forward declarations for parser functions
static JSONValue* parse_value(const char** json);
static JSONValue* parse_object(const char** json);
static JSONValue* parse_array(const char** json);
static JSONValue* parse_string(const char** json);
static JSONValue* parse_number(const char** json);
static JSONValue* parse_boolean(const char** json);
static JSONValue* parse_null(const char** json);

// Helper function to skip whitespace
static void skip_whitespace(const char** json) {
    const char* j = *json;
    
    while (*j != '\0' && isspace(*j)) {
        j++;
    }
    
    *json = j;
}

// Helper function to check for a specific token
static bool match_token(const char** json, const char* token) {
    const char* j = *json;
    size_t len = strlen(token);
    
    if (strncmp(j, token, len) == 0) {
        *json = j + len;
        return true;
    }
    
    return false;
}

// Helper function to check for end of object/array
static bool check_end(const char** json, char endChar) {
    skip_whitespace(json);
    
    if (**json == endChar) {
        (*json)++;
        return true;
    }
    
    return false;
}

// Parse a JSON value
static JSONValue* parse_value(const char** json) {
    skip_whitespace(json);
    
    switch (**json) {
        case '{': return parse_object(json);
        case '[': return parse_array(json);
        case '"': return parse_string(json);
        case 't': case 'f': return parse_boolean(json);
        case 'n': return parse_null(json);
        case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number(json);
        default: return NULL; // Error
    }
}

// Parse a JSON object
static JSONValue* parse_object(const char** json) {
    (*json)++; // Skip '{'
    
    // Create object
    JSONValue* obj = json_create_object();
    if (obj == NULL) {
        return NULL;
    }
    
    // Check for empty object
    if (check_end(json, '}')) {
        return obj;
    }
    
    // Parse key-value pairs
    while (true) {
        skip_whitespace(json);
        
        // Parse key (must be a string)
        JSONValue* key = parse_string(json);
        if (key == NULL) {
            json_free(obj);
            return NULL;
        }
        
        skip_whitespace(json);
        
        // Check for colon
        if (**json != ':') {
            json_free(key);
            json_free(obj);
            return NULL;
        }
        (*json)++; // Skip ':'
        
        // Parse value
        JSONValue* value = parse_value(json);
        if (value == NULL) {
            json_free(key);
            json_free(obj);
            return NULL;
        }
        
        // Add key-value pair to object
        char* keyStr = key->value.stringValue;
        key->value.stringValue = NULL; // Transfer ownership
        json_free(key);
        
        // Allocate or expand keys array
        if (obj->value.objectValue.count == 0) {
            obj->value.objectValue.keys = (char**)malloc(sizeof(char*));
            obj->value.objectValue.values = (JSONValue*)malloc(sizeof(JSONValue));
        } else {
            obj->value.objectValue.keys = (char**)realloc(obj->value.objectValue.keys, 
                (obj->value.objectValue.count + 1) * sizeof(char*));
            obj->value.objectValue.values = (JSONValue*)realloc(obj->value.objectValue.values, 
                (obj->value.objectValue.count + 1) * sizeof(JSONValue));
        }
        
        if (obj->value.objectValue.keys == NULL || obj->value.objectValue.values == NULL) {
            free(keyStr);
            json_free(value);
            json_free(obj);
            return NULL;
        }
        
        // Store key-value pair
        obj->value.objectValue.keys[obj->value.objectValue.count] = keyStr;
        obj->value.objectValue.values[obj->value.objectValue.count] = *value;
        free(value); // Free only the container, not the contents
        obj->value.objectValue.count++;
        
        skip_whitespace(json);
        
        // Check for end of object or next pair
        if (**json == '}') {
            (*json)++;
            return obj;
        }
        
        if (**json != ',') {
            json_free(obj);
            return NULL;
        }
        
        (*json)++; // Skip ','
    }
}

// Parse a JSON array
static JSONValue* parse_array(const char** json) {
    (*json)++; // Skip '['
    
    // Create array
    JSONValue* arr = json_create_array();
    if (arr == NULL) {
        return NULL;
    }
    
    // Check for empty array
    if (check_end(json, ']')) {
        return arr;
    }
    
    // Parse values
    while (true) {
        // Parse value
        JSONValue* value = parse_value(json);
        if (value == NULL) {
            json_free(arr);
            return NULL;
        }
        
        // Allocate or expand values array
        if (arr->value.arrayValue.count == 0) {
            arr->value.arrayValue.values = (JSONValue*)malloc(sizeof(JSONValue));
        } else {
            arr->value.arrayValue.values = (JSONValue*)realloc(arr->value.arrayValue.values, 
                (arr->value.arrayValue.count + 1) * sizeof(JSONValue));
        }
        
        if (arr->value.arrayValue.values == NULL) {
            json_free(value);
            json_free(arr);
            return NULL;
        }
        
        // Store value
        arr->value.arrayValue.values[arr->value.arrayValue.count] = *value;
        free(value); // Free only the container, not the contents
        arr->value.arrayValue.count++;
        
        skip_whitespace(json);
        
        // Check for end of array or next value
        if (**json == ']') {
            (*json)++;
            return arr;
        }
        
        if (**json != ',') {
            json_free(arr);
            return NULL;
        }
        
        (*json)++; // Skip ','
    }
}

// Parse a JSON string
static JSONValue* parse_string(const char** json) {
    const char* j = *json;
    j++; // Skip opening quote
    
    const char* start = j;
    
    // Find end of string (unescaped quote)
    bool escaped = false;
    while (*j != '\0' && (escaped || *j != '"')) {
        if (*j == '\\' && !escaped) {
            escaped = true;
        } else {
            escaped = false;
        }
        j++;
    }
    
    if (*j != '"') {
        return NULL; // Unterminated string
    }
    
    // Calculate string length
    size_t len = j - start;
    
    // Allocate string
    char* str = (char*)malloc(len + 1);
    if (str == NULL) {
        return NULL;
    }
    
    // Copy string content (without processing escapes for simplicity)
    memcpy(str, start, len);
    str[len] = '\0';
    
    // Create string value
    JSONValue* value = json_create_string(str);
    free(str); // json_create_string makes a copy
    
    *json = j + 1; // Skip closing quote
    
    return value;
}

// Parse a JSON number
static JSONValue* parse_number(const char** json) {
    const char* j = *json;
    
    // Use strtod to parse number
    char* end;
    double value = strtod(j, &end);
    
    if (end == j) {
        return NULL; // Not a number
    }
    
    *json = end;
    
    return json_create_number(value);
}

// Parse a JSON boolean
static JSONValue* parse_boolean(const char** json) {
    if (match_token(json, "true")) {
        return json_create_bool(true);
    }
    
    if (match_token(json, "false")) {
        return json_create_bool(false);
    }
    
    return NULL; // Not a boolean
}

// Parse a JSON null
static JSONValue* parse_null(const char** json) {
    if (match_token(json, "null")) {
        return json_create_null();
    }
    
    return NULL; // Not null
}

// Public API implementation

JSONValue* json_parse(const char* json, size_t length) {
    if (json == NULL || length == 0) {
        return NULL;
    }
    
    // Add null terminator if needed
    char* jsonCopy = NULL;
    if (json[length - 1] != '\0') {
        jsonCopy = (char*)malloc(length + 1);
        if (jsonCopy == NULL) {
            return NULL;
        }
        
        memcpy(jsonCopy, json, length);
        jsonCopy[length] = '\0';
        json = jsonCopy;
    }
    
    const char* j = json;
    JSONValue* result = parse_value(&j);
    
    if (jsonCopy != NULL) {
        free(jsonCopy);
    }
    
    return result;
}

void json_free(JSONValue* value) {
    if (value == NULL) {
        return;
    }
    
    // Free based on type
    switch (value->type) {
        case JSON_VALUE_STRING:
            free(value->value.stringValue);
            break;
            
        case JSON_VALUE_ARRAY:
            // Free array elements
            for (size_t i = 0; i < value->value.arrayValue.count; i++) {
                json_free(&value->value.arrayValue.values[i]);
            }
            free(value->value.arrayValue.values);
            break;
            
        case JSON_VALUE_OBJECT:
            // Free keys and values
            for (size_t i = 0; i < value->value.objectValue.count; i++) {
                free(value->value.objectValue.keys[i]);
                json_free(&value->value.objectValue.values[i]);
            }
            free(value->value.objectValue.keys);
            free(value->value.objectValue.values);
            break;
            
        default:
            break; // Other types don't have allocated memory
    }
    
    // Free value container
    free(value);
}

// Helper function to write indent
static void write_indent(char* buffer, size_t* offset, size_t bufferSize, int indent, bool pretty) {
    if (!pretty) {
        return;
    }
    
    if (*offset + indent + 1 >= bufferSize) {
        return; // Buffer too small
    }
    
    for (int i = 0; i < indent; i++) {
        buffer[(*offset)++] = ' ';
    }
    
    buffer[*offset] = '\0';
}

// Helper function to write string
static void write_string(char* buffer, size_t* offset, size_t bufferSize, const char* str) {
    if (str == NULL) {
        if (*offset + 6 >= bufferSize) {
            return; // Buffer too small
        }
        
        strcpy(buffer + *offset, "null");
        *offset += 4;
        return;
    }
    
    if (*offset + 3 >= bufferSize) {
        return; // Buffer too small
    }
    
    buffer[(*offset)++] = '"';
    
    // Copy string with escaping
    while (*str != '\0') {
        if (*offset + 2 >= bufferSize) {
            return; // Buffer too small
        }
        
        char c = *str++;
        
        // Escape special characters
        if (c == '"' || c == '\\') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = c;
        } else if (c == '\b') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = 'b';
        } else if (c == '\f') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = 'f';
        } else if (c == '\n') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = 'n';
        } else if (c == '\r') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = 'r';
        } else if (c == '\t') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = 't';
        } else {
            buffer[(*offset)++] = c;
        }
    }
    
    if (*offset + 2 >= bufferSize) {
        return; // Buffer too small
    }
    
    buffer[(*offset)++] = '"';
    buffer[*offset] = '\0';
}

// Helper function to stringify JSON value
static void stringify_value(const JSONValue* value, char* buffer, size_t* offset, size_t bufferSize, 
                          int indent, bool pretty) {
    if (value == NULL) {
        if (*offset + 5 >= bufferSize) {
            return; // Buffer too small
        }
        
        strcpy(buffer + *offset, "null");
        *offset += 4;
        return;
    }
    
    switch (value->type) {
        case JSON_VALUE_NULL:
            if (*offset + 5 >= bufferSize) {
                return; // Buffer too small
            }
            
            strcpy(buffer + *offset, "null");
            *offset += 4;
            break;
            
        case JSON_VALUE_BOOL:
            if (*offset + 6 >= bufferSize) {
                return; // Buffer too small
            }
            
            if (value->value.boolValue) {
                strcpy(buffer + *offset, "true");
                *offset += 4;
            } else {
                strcpy(buffer + *offset, "false");
                *offset += 5;
            }
            break;
            
        case JSON_VALUE_NUMBER: {
            if (*offset + 32 >= bufferSize) {
                return; // Buffer too small
            }
            
            int written = snprintf(buffer + *offset, bufferSize - *offset, "%g", value->value.numberValue);
            if (written > 0) {
                *offset += written;
            }
            break;
        }
            
        case JSON_VALUE_STRING:
            write_string(buffer, offset, bufferSize, value->value.stringValue);
            break;
            
        case JSON_VALUE_ARRAY: {
            if (*offset + 2 >= bufferSize) {
                return; // Buffer too small
            }
            
            buffer[(*offset)++] = '[';
            
            if (pretty) {
                buffer[(*offset)++] = '\n';
            }
            
            for (size_t i = 0; i < value->value.arrayValue.count; i++) {
                // Add indent
                write_indent(buffer, offset, bufferSize, indent + 2, pretty);
                
                // Add value
                stringify_value(&value->value.arrayValue.values[i], buffer, offset, 
                              bufferSize, indent + 2, pretty);
                
                // Add comma if not last element
                if (i < value->value.arrayValue.count - 1) {
                    if (*offset + 2 >= bufferSize) {
                        return; // Buffer too small
                    }
                    
                    buffer[(*offset)++] = ',';
                    
                    if (pretty) {
                        buffer[(*offset)++] = '\n';
                    }
                } else if (pretty) {
                    buffer[(*offset)++] = '\n';
                }
            }
            
            // Add closing bracket
            write_indent(buffer, offset, bufferSize, indent, pretty);
            
            if (*offset + 2 >= bufferSize) {
                return; // Buffer too small
            }
            
            buffer[(*offset)++] = ']';
            break;
        }
            
        case JSON_VALUE_OBJECT: {
            if (*offset + 2 >= bufferSize) {
                return; // Buffer too small
            }
            
            buffer[(*offset)++] = '{';
            
            if (pretty) {
                buffer[(*offset)++] = '\n';
            }
            
            for (size_t i = 0; i < value->value.objectValue.count; i++) {
                // Add indent
                write_indent(buffer, offset, bufferSize, indent + 2, pretty);
                
                // Add key
                write_string(buffer, offset, bufferSize, value->value.objectValue.keys[i]);
                
                if (*offset + 2 >= bufferSize) {
                    return; // Buffer too small
                }
                
                buffer[(*offset)++] = ':';
                
                if (pretty) {
                    buffer[(*offset)++] = ' ';
                }
                
                // Add value
                stringify_value(&value->value.objectValue.values[i], buffer, offset, 
                              bufferSize, indent + 2, pretty);
                
                // Add comma if not last element
                if (i < value->value.objectValue.count - 1) {
                    if (*offset + 2 >= bufferSize) {
                        return; // Buffer too small
                    }
                    
                    buffer[(*offset)++] = ',';
                    
                    if (pretty) {
                        buffer[(*offset)++] = '\n';
                    }
                } else if (pretty) {
                    buffer[(*offset)++] = '\n';
                }
            }
            
            // Add closing brace
            write_indent(buffer, offset, bufferSize, indent, pretty);
            
            if (*offset + 2 >= bufferSize) {
                return; // Buffer too small
            }
            
            buffer[(*offset)++] = '}';
            break;
        }
    }
    
    buffer[*offset] = '\0';
}

int json_stringify(const JSONValue* value, char* buffer, size_t bufferSize, bool pretty) {
    if (buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    size_t offset = 0;
    stringify_value(value, buffer, &offset, bufferSize, 0, pretty);
    
    return offset;
}

char* json_get_string_field(const char* json, const char* field) {
    if (json == NULL || field == NULL) {
        return NULL;
    }
    
    // Parse JSON
    size_t len = strlen(json);
    JSONValue* root = json_parse(json, len);
    if (root == NULL || root->type != JSON_VALUE_OBJECT) {
        json_free(root);
        return NULL;
    }
    
    // Find field
    char* result = NULL;
    for (size_t i = 0; i < root->value.objectValue.count; i++) {
        if (strcmp(root->value.objectValue.keys[i], field) == 0) {
            JSONValue* value = &root->value.objectValue.values[i];
            if (value->type == JSON_VALUE_STRING && value->value.stringValue != NULL) {
                result = strdup(value->value.stringValue);
            }
            break;
        }
    }
    
    json_free(root);
    return result;
}

int json_get_int_field(const char* json, const char* field, int defaultValue) {
    if (json == NULL || field == NULL) {
        return defaultValue;
    }
    
    // Parse JSON
    size_t len = strlen(json);
    JSONValue* root = json_parse(json, len);
    if (root == NULL || root->type != JSON_VALUE_OBJECT) {
        json_free(root);
        return defaultValue;
    }
    
    // Find field
    int result = defaultValue;
    for (size_t i = 0; i < root->value.objectValue.count; i++) {
        if (strcmp(root->value.objectValue.keys[i], field) == 0) {
            JSONValue* value = &root->value.objectValue.values[i];
            if (value->type == JSON_VALUE_NUMBER) {
                result = (int)value->value.numberValue;
            }
            break;
        }
    }
    
    json_free(root);
    return result;
}

bool json_get_bool_field(const char* json, const char* field, bool defaultValue) {
    if (json == NULL || field == NULL) {
        return defaultValue;
    }
    
    // Parse JSON
    size_t len = strlen(json);
    JSONValue* root = json_parse(json, len);
    if (root == NULL || root->type != JSON_VALUE_OBJECT) {
        json_free(root);
        return defaultValue;
    }
    
    // Find field
    bool result = defaultValue;
    for (size_t i = 0; i < root->value.objectValue.count; i++) {
        if (strcmp(root->value.objectValue.keys[i], field) == 0) {
            JSONValue* value = &root->value.objectValue.values[i];
            if (value->type == JSON_VALUE_BOOL) {
                result = value->value.boolValue;
            }
            break;
        }
    }
    
    json_free(root);
    return result;
}

float json_get_float_field(const char* json, const char* field, float defaultValue) {
    if (json == NULL || field == NULL) {
        return defaultValue;
    }
    
    // Parse JSON
    size_t len = strlen(json);
    JSONValue* root = json_parse(json, len);
    if (root == NULL || root->type != JSON_VALUE_OBJECT) {
        json_free(root);
        return defaultValue;
    }
    
    // Find field
    float result = defaultValue;
    for (size_t i = 0; i < root->value.objectValue.count; i++) {
        if (strcmp(root->value.objectValue.keys[i], field) == 0) {
            JSONValue* value = &root->value.objectValue.values[i];
            if (value->type == JSON_VALUE_NUMBER) {
                result = (float)value->value.numberValue;
            }
            break;
        }
    }
    
    json_free(root);
    return result;
}

void* json_get_object_field(const char* json, const char* field) {
    if (json == NULL || field == NULL) {
        return NULL;
    }
    
    // Parse JSON
    size_t len = strlen(json);
    JSONValue* root = json_parse(json, len);
    if (root == NULL || root->type != JSON_VALUE_OBJECT) {
        json_free(root);
        return NULL;
    }
    
    // Find field
    void* result = NULL;
    for (size_t i = 0; i < root->value.objectValue.count; i++) {
        if (strcmp(root->value.objectValue.keys[i], field) == 0) {
            JSONValue* value = &root->value.objectValue.values[i];
            if (value->type == JSON_VALUE_OBJECT) {
                // Return a copy of the object
                // In a real implementation, you'd need a way to manage memory
                // For now, we just return a non-NULL placeholder
                result = value;
            }
            break;
        }
    }
    
    // Note: We don't free root here, as we're returning a reference to it
    return result;
}

void* json_get_array_field(const char* json, const char* field) {
    if (json == NULL || field == NULL) {
        return NULL;
    }
    
    // Parse JSON
    size_t len = strlen(json);
    JSONValue* root = json_parse(json, len);
    if (root == NULL || root->type != JSON_VALUE_OBJECT) {
        json_free(root);
        return NULL;
    }
    
    // Find field
    void* result = NULL;
    for (size_t i = 0; i < root->value.objectValue.count; i++) {
        if (strcmp(root->value.objectValue.keys[i], field) == 0) {
            JSONValue* value = &root->value.objectValue.values[i];
            if (value->type == JSON_VALUE_ARRAY) {
                // Return a copy of the array
                // In a real implementation, you'd need a way to manage memory
                // For now, we just return a non-NULL placeholder
                result = value;
            }
            break;
        }
    }
    
    // Note: We don't free root here, as we're returning a reference to it
    return result;
}

bool json_field_exists(const char* json, const char* field) {
    if (json == NULL || field == NULL) {
        return false;
    }
    
    // Parse JSON
    size_t len = strlen(json);
    JSONValue* root = json_parse(json, len);
    if (root == NULL || root->type != JSON_VALUE_OBJECT) {
        json_free(root);
        return false;
    }
    
    // Find field
    bool result = false;
    for (size_t i = 0; i < root->value.objectValue.count; i++) {
        if (strcmp(root->value.objectValue.keys[i], field) == 0) {
            result = true;
            break;
        }
    }
    
    json_free(root);
    return result;
}

size_t json_array_length(const void* array) {
    if (array == NULL) {
        return 0;
    }
    
    const JSONValue* arr = (const JSONValue*)array;
    if (arr->type != JSON_VALUE_ARRAY) {
        return 0;
    }
    
    return arr->value.arrayValue.count;
}

char* json_array_get_string(const void* array, size_t index) {
    if (array == NULL) {
        return NULL;
    }
    
    const JSONValue* arr = (const JSONValue*)array;
    if (arr->type != JSON_VALUE_ARRAY || index >= arr->value.arrayValue.count) {
        return NULL;
    }
    
    const JSONValue* value = &arr->value.arrayValue.values[index];
    if (value->type != JSON_VALUE_STRING || value->value.stringValue == NULL) {
        return NULL;
    }
    
    return strdup(value->value.stringValue);
}

int json_array_get_int(const void* array, size_t index, int defaultValue) {
    if (array == NULL) {
        return defaultValue;
    }
    
    const JSONValue* arr = (const JSONValue*)array;
    if (arr->type != JSON_VALUE_ARRAY || index >= arr->value.arrayValue.count) {
        return defaultValue;
    }
    
    const JSONValue* value = &arr->value.arrayValue.values[index];
    if (value->type != JSON_VALUE_NUMBER) {
        return defaultValue;
    }
    
    return (int)value->value.numberValue;
}

bool json_array_get_bool(const void* array, size_t index, bool defaultValue) {
    if (array == NULL) {
        return defaultValue;
    }
    
    const JSONValue* arr = (const JSONValue*)array;
    if (arr->type != JSON_VALUE_ARRAY || index >= arr->value.arrayValue.count) {
        return defaultValue;
    }
    
    const JSONValue* value = &arr->value.arrayValue.values[index];
    if (value->type != JSON_VALUE_BOOL) {
        return defaultValue;
    }
    
    return value->value.boolValue;
}

float json_array_get_float(const void* array, size_t index, float defaultValue) {
    if (array == NULL) {
        return defaultValue;
    }
    
    const JSONValue* arr = (const JSONValue*)array;
    if (arr->type != JSON_VALUE_ARRAY || index >= arr->value.arrayValue.count) {
        return defaultValue;
    }
    
    const JSONValue* value = &arr->value.arrayValue.values[index];
    if (value->type != JSON_VALUE_NUMBER) {
        return defaultValue;
    }
    
    return (float)value->value.numberValue;
}

void* json_array_get_object(const void* array, size_t index) {
    if (array == NULL) {
        return NULL;
    }
    
    const JSONValue* arr = (const JSONValue*)array;
    if (arr->type != JSON_VALUE_ARRAY || index >= arr->value.arrayValue.count) {
        return NULL;
    }
    
    const JSONValue* value = &arr->value.arrayValue.values[index];
    if (value->type != JSON_VALUE_OBJECT) {
        return NULL;
    }
    
    return (void*)value;
}

JSONValue* json_create_object(void) {
    JSONValue* obj = (JSONValue*)malloc(sizeof(JSONValue));
    if (obj == NULL) {
        return NULL;
    }
    
    obj->type = JSON_VALUE_OBJECT;
    obj->value.objectValue.keys = NULL;
    obj->value.objectValue.values = NULL;
    obj->value.objectValue.count = 0;
    
    return obj;
}

JSONValue* json_create_array(void) {
    JSONValue* arr = (JSONValue*)malloc(sizeof(JSONValue));
    if (arr == NULL) {
        return NULL;
    }
    
    arr->type = JSON_VALUE_ARRAY;
    arr->value.arrayValue.values = NULL;
    arr->value.arrayValue.count = 0;
    
    return arr;
}

JSONValue* json_create_string(const char* value) {
    JSONValue* str = (JSONValue*)malloc(sizeof(JSONValue));
    if (str == NULL) {
        return NULL;
    }
    
    str->type = JSON_VALUE_STRING;
    str->value.stringValue = value ? strdup(value) : NULL;
    
    return str;
}

JSONValue* json_create_number(double value) {
    JSONValue* num = (JSONValue*)malloc(sizeof(JSONValue));
    if (num == NULL) {
        return NULL;
    }
    
    num->type = JSON_VALUE_NUMBER;
    num->value.numberValue = value;
    
    return num;
}

JSONValue* json_create_bool(bool value) {
    JSONValue* b = (JSONValue*)malloc(sizeof(JSONValue));
    if (b == NULL) {
        return NULL;
    }
    
    b->type = JSON_VALUE_BOOL;
    b->value.boolValue = value;
    
    return b;
}

JSONValue* json_create_null(void) {
    JSONValue* n = (JSONValue*)malloc(sizeof(JSONValue));
    if (n == NULL) {
        return NULL;
    }
    
    n->type = JSON_VALUE_NULL;
    
    return n;
}

int json_object_add(JSONValue* object, const char* key, JSONValue* value) {
    if (object == NULL || key == NULL || value == NULL) {
        return -1;
    }
    
    if (object->type != JSON_VALUE_OBJECT) {
        return -2;
    }
    
    // Allocate or expand keys and values arrays
    if (object->value.objectValue.count == 0) {
        object->value.objectValue.keys = (char**)malloc(sizeof(char*));
        object->value.objectValue.values = (JSONValue*)malloc(sizeof(JSONValue));
    } else {
        object->value.objectValue.keys = (char**)realloc(object->value.objectValue.keys, 
            (object->value.objectValue.count + 1) * sizeof(char*));
        object->value.objectValue.values = (JSONValue*)realloc(object->value.objectValue.values, 
            (object->value.objectValue.count + 1) * sizeof(JSONValue));
    }
    
    if (object->value.objectValue.keys == NULL || object->value.objectValue.values == NULL) {
        return -3;
    }
    
    // Store key-value pair
    object->value.objectValue.keys[object->value.objectValue.count] = strdup(key);
    object->value.objectValue.values[object->value.objectValue.count] = *value;
    object->value.objectValue.count++;
    
    // Free the value container (but not its contents)
    free(value);
    
    return 0;
}

int json_array_add(JSONValue* array, JSONValue* value) {
    if (array == NULL || value == NULL) {
        return -1;
    }
    
    if (array->type != JSON_VALUE_ARRAY) {
        return -2;
    }
    
    // Allocate or expand values array
    if (array->value.arrayValue.count == 0) {
        array->value.arrayValue.values = (JSONValue*)malloc(sizeof(JSONValue));
    } else {
        array->value.arrayValue.values = (JSONValue*)realloc(array->value.arrayValue.values, 
            (array->value.arrayValue.count + 1) * sizeof(JSONValue));
    }
    
    if (array->value.arrayValue.values == NULL) {
        return -3;
    }
    
    // Store value
    array->value.arrayValue.values[array->value.arrayValue.count] = *value;
    array->value.arrayValue.count++;
    
    // Free the value container (but not its contents)
    free(value);
    
    return 0;
}

bool json_validate_schema(const char* json, const char* schema) {
    // This is a placeholder implementation
    // In a real implementation, you'd need a proper JSON Schema validator
    // For now, we just return true if both are valid JSON
    
    if (json == NULL || schema == NULL) {
        return false;
    }
    
    size_t jsonLen = strlen(json);
    size_t schemaLen = strlen(schema);
    
    JSONValue* jsonVal = json_parse(json, jsonLen);
    JSONValue* schemaVal = json_parse(schema, schemaLen);
    
    bool valid = (jsonVal != NULL && schemaVal != NULL);
    
    json_free(jsonVal);
    json_free(schemaVal);
    
    return valid;
}