/**
 * @file json_mock.c
 * @brief Mock implementations of JSON helper functions
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

// Mock implementation for json_array_length
size_t json_array_length(const void* array) {
    (void)array; // Unused parameter
    return 1; // Mock implementation always returns 1
}

// Mock implementation for json_get_array_field
void* json_get_array_field(const char* json, const char* field) {
    (void)json; // Unused parameter
    (void)field; // Unused parameter
    static char mockArray[] = "[]"; // Empty array
    return mockArray;
}

// Mock implementation for json_get_object_field
void* json_get_object_field(const char* json, const char* field) {
    (void)json; // Unused parameter
    (void)field; // Unused parameter
    static char mockObject[] = "{}"; // Empty object
    return mockObject;
}

// Mock implementation for json_array_get_object
void* json_array_get_object(const void* array, size_t index) {
    (void)array; // Unused parameter
    (void)index; // Unused parameter
    static char mockObject[] = "{}"; // Empty object
    return mockObject;
}

// Mock implementation for json_get_bool_field
bool json_get_bool_field(const char* json, const char* field, bool defaultValue) {
    (void)json; // Unused parameter
    (void)field; // Unused parameter
    return defaultValue;
}

// Mock implementation for json_validate_schema
bool json_validate_schema(const char* schema, const char* json) {
    (void)json; // Unused parameter
    (void)schema; // Unused parameter
    return true; // Always valid in mock
}
