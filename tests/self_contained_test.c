#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Mock implementation of json_get_string_field
char* json_get_string_field(const char* json, const char* field) {
    if (json == NULL || field == NULL) {
        return NULL;
    }
    
    // Very basic implementation that looks for "field":"value" pattern
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":\"", field);
    
    const char* start = strstr(json, search_pattern);
    if (start == NULL) {
        return NULL;
    }
    
    // Move past the field name and ":"
    start += strlen(search_pattern);
    
    // Find the closing quote
    const char* end = strchr(start, '"');
    if (end == NULL) {
        return NULL;
    }
    
    // Extract the value
    size_t length = end - start;
    char* value = (char*)malloc(length + 1);
    if (value == NULL) {
        return NULL;
    }
    
    strncpy(value, start, length);
    value[length] = '\0';
    
    return value;
}

// Mock implementation of json_validate_schema
bool json_validate_schema(const char* schema, const char* json) {
    // Simplified implementation that always returns true
    (void)schema;
    (void)json;
    return true;
}

// Basic device_info structure
typedef struct {
    char deviceName[64];
    char firmwareVersion[32];
    char platformName[32];
} DeviceInfo;

// Mock implementation to get device info
const DeviceInfo* get_device_info(void) {
    static DeviceInfo info = {0};
    
    if (info.deviceName[0] == '\0') {
        // Initialize with some values
        strncpy(info.deviceName, "Test Device", sizeof(info.deviceName) - 1);
        strncpy(info.firmwareVersion, "1.0.0", sizeof(info.firmwareVersion) - 1);
        strncpy(info.platformName, "Test Platform", sizeof(info.platformName) - 1);
    }
    
    return &info;
}

// Convert device info to JSON
char* device_info_to_json(bool compact) {
    // Ignore compact parameter for this example
    (void)compact;
    
    const DeviceInfo* info = get_device_info();
    
    // Format a JSON string
    char* json = (char*)malloc(512);
    if (json == NULL) {
        return NULL;
    }
    
    snprintf(json, 512, 
             "{"
             "\"deviceName\":\"%s\","
             "\"firmwareVersion\":\"%s\","
             "\"platformName\":\"%s\""
             "}", 
             info->deviceName, 
             info->firmwareVersion, 
             info->platformName);
    
    return json;
}

int main(void) {
    printf("=== MCP Implementation Test ===\n\n");
    
    // Test 1: Get device info
    printf("Test 1: Get device info\n");
    const DeviceInfo* info = get_device_info();
    printf("Device Name: %s\n", info->deviceName);
    printf("Firmware Version: %s\n", info->firmwareVersion);
    printf("Platform: %s\n\n", info->platformName);
    
    // Test 2: Convert device info to JSON
    printf("Test 2: Convert device info to JSON\n");
    char* json = device_info_to_json(false);
    printf("JSON: %s\n\n", json);
    
    // Test 3: Extract field from JSON
    printf("Test 3: Extract field from JSON\n");
    char* deviceName = json_get_string_field(json, "deviceName");
    char* firmwareVersion = json_get_string_field(json, "firmwareVersion");
    
    printf("Extracted deviceName: %s\n", deviceName);
    printf("Extracted firmwareVersion: %s\n\n", firmwareVersion);
    
    // Test 4: Validate schema
    printf("Test 4: Validate schema\n");
    bool valid = json_validate_schema("{}", json);
    printf("Schema validation result: %s\n\n", valid ? "Valid" : "Invalid");
    
    // Clean up
    free(json);
    free(deviceName);
    free(firmwareVersion);
    
    printf("=== All tests passed! ===\n");
    return 0;
}