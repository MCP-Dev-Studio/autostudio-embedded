/**
 * @file json_device_info_mock.c
 * @brief Mock JSON functions for device_info test
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * Mock implementation of json_get_string_field for device_info test
 */
char* json_get_string_field(const char* json, const char* field) {
    if (json == NULL || field == NULL) {
        return NULL;
    }
    
    // Handle the format field used in MCP_DeviceInfoToolHandler
    if (strcmp(field, "format") == 0) {
        if (strstr(json, "\"format\":\"compact\"") != NULL) {
            return strdup("compact");
        } else if (strstr(json, "\"format\":\"full\"") != NULL) {
            return strdup("full");
        }
    }
    
    // Default return for other fields
    return NULL;
}

/**
 * Mock implementation of json_validate_schema
 */
bool json_validate_schema(const char* schema, const char* json) {
    (void)schema;
    (void)json;
    return true;
}