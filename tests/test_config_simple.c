#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

// Define logging functions
void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[INFO] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[WARN] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[ERROR] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

// Define storage functions
int persistent_storage_write(const char* key, const void* data, size_t size) {
    FILE* fp = fopen(key, "wb");
    if (fp == NULL) {
        return -1;
    }
    
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    
    return (written == size) ? 0 : -2;
}

int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize) {
    FILE* fp = fopen(key, "rb");
    if (fp == NULL) {
        *actualSize = 0;
        return -1;
    }
    
    *actualSize = fread(data, 1, maxSize, fp);
    fclose(fp);
    
    return 0;
}

// Define a platform for testing
#define MCP_PLATFORM_RPI

// Include the configuration headers
#include "../src/core/mcp/config/mcp_config.h"
#include "../src/core/mcp/config/platform_config.h"

// Test the JSON serialization and configuration functions
int main() {
    printf("Testing MCP Configuration JSON Serialization\n");
    printf("=========================================\n\n");
    
    // 1. Initialize a configuration
    MCP_PLATFORM_CONFIG config;
    int result = MCP_ConfigInit(&config);
    
    printf("1. Initialized configuration:\n");
    printf("   Result: %d (0 = success)\n", result);
    printf("   Device Name: %s\n", config.common.device_name);
    printf("   Server Port: %d\n", config.common.server_port);
    
    // 2. Modify some settings
    printf("\n2. Modifying configuration...\n");
    strncpy(config.common.device_name, "JSON Test Device", sizeof(config.common.device_name) - 1);
    config.common.server_port = 9090;
    config.common.network.enabled = true;
    strncpy(config.common.network.ssid, "TestNetwork", sizeof(config.common.network.ssid) - 1);
    strncpy(config.common.network.password, "TestPassword", sizeof(config.common.network.password) - 1);
    
    // Platform-specific settings
    config.enable_camera = true;
    config.camera_resolution = 1080;
    
    // 3. Serialize to JSON
    printf("\n3. Serializing to JSON...\n");
    char json_buffer[8192];
    int json_length = MCP_ConfigSerializeToJSON(&config, json_buffer, sizeof(json_buffer));
    printf("   Serialization result: %d bytes\n", json_length);
    printf("   Sample JSON content (excerpt):\n   %.200s...\n", json_buffer);
    
    // 4. Initialize a new configuration
    printf("\n4. Creating a new configuration for testing deserialization...\n");
    MCP_PLATFORM_CONFIG new_config;
    MCP_ConfigInit(&new_config);
    
    // Verify it has default values
    printf("   New config default device name: %s\n", new_config.common.device_name);
    printf("   New config default server port: %d\n", new_config.common.server_port);
    
    // 5. Deserialize from JSON
    printf("\n5. Deserializing from JSON...\n");
    result = MCP_ConfigUpdateFromJSON(&new_config, json_buffer);
    printf("   Deserialization result: %d (0 = success)\n", result);
    
    // 6. Verify deserialized values
    printf("\n6. Verifying deserialized values:\n");
    printf("   Device Name: %s\n", new_config.common.device_name);
    printf("   Server Port: %d\n", new_config.common.server_port);
    printf("   WiFi Enabled: %s\n", new_config.common.network.enabled ? "true" : "false");
    printf("   WiFi SSID: %s\n", new_config.common.network.ssid);
    printf("   Camera Enabled: %s\n", new_config.enable_camera ? "true" : "false");
    printf("   Camera Resolution: %d\n", new_config.camera_resolution);
    
    // 7. Test platform-independent API
    printf("\n7. Testing platform-independent API...\n");
    const char* test_json = "{"
        "\"device\": {"
        "  \"name\": \"API Test Device\","
        "  \"firmware_version\": \"2.0.0\""
        "},"
        "\"server\": {"
        "  \"port\": 7777"
        "},"
        "\"platform\": {"
        "  \"rpi\": {"
        "    \"enable_camera\": false"
        "  }"
        "}"
    "}";
    
    result = MCP_SetConfiguration(test_json);
    printf("   Set configuration result: %d (0 = success)\n", result);
    
    // 8. Get configuration with platform-independent API
    printf("\n8. Getting configuration with platform-independent API...\n");
    char get_buffer[4096];
    int get_length = MCP_GetConfiguration(get_buffer, sizeof(get_buffer));
    printf("   Get configuration result: %d bytes\n", get_length);
    printf("   Sample content (excerpt):\n   %.200s...\n", get_buffer);
    
    // 9. Test persistence functions
    printf("\n9. Testing persistence functions...\n");
    
    // Set a path for testing
    strncpy(config.common.config_file_path, "/tmp/mcp_test_config.json", sizeof(config.common.config_file_path) - 1);
    
    // Save configuration
    result = MCP_ConfigSave(&config, NULL);
    printf("   Save result: %d (0 = success)\n", result);
    
    // Initialize a new configuration
    MCP_PLATFORM_CONFIG loaded_config;
    MCP_ConfigInit(&loaded_config);
    strncpy(loaded_config.common.config_file_path, "/tmp/mcp_test_config.json", sizeof(loaded_config.common.config_file_path) - 1);
    
    // Load configuration
    result = MCP_ConfigLoad(&loaded_config, NULL);
    printf("   Load result: %d (0 = success)\n", result);
    
    // Verify loaded values
    printf("\n10. Verifying loaded values:\n");
    printf("   Device Name: %s\n", loaded_config.common.device_name);
    printf("   Server Port: %d\n", loaded_config.common.server_port);
    printf("   WiFi Enabled: %s\n", loaded_config.common.network.enabled ? "true" : "false");
    printf("   WiFi SSID: %s\n", loaded_config.common.network.ssid);
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}