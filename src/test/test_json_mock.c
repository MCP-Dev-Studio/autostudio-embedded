// Test mock JSON helpers with specialized implementations for test_dynamic_tools
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "test_mocks.h"

// These are the functions required by tool_registry.c but not defined in test_dynamic_tools.c
size_t json_array_length(const void* array) {
    (void)array;
    return 1;
}

void* json_get_array_field(const char* json, const char* field) {
    (void)json;
    (void)field;
    static char mockArray[] = "[]";
    return mockArray;
}

void* json_get_object_field(const char* json, const char* field) {
    (void)json;
    (void)field;
    static char mockObject[] = "{}";
    return mockObject;
}

void* json_array_get_object(const void* array, size_t index) {
    (void)array;
    (void)index;
    static char mockObject[] = "{}";
    return mockObject;
}

bool json_get_bool_field(const char* json, const char* field, bool defaultValue) {
    (void)json;
    (void)field;
    return defaultValue;
}

bool json_validate_schema(const char* schema, const char* json) {
    (void)schema;
    (void)json;
    return true;
}
