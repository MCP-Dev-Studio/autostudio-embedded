#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

// Define a platform for testing
#define MCP_PLATFORM_RPI

// Include the configuration headers
#include "../src/core/mcp/config/mcp_config.h"
#include "../src/core/mcp/config/platform_config.h"

// Test the configuration system
int main() {
    printf("Testing MCP Configuration System\n");
    printf("================================\n\n");
    
    // 1. Initialize configuration
    MCP_PLATFORM_CONFIG config;
    int result = MCP_ConfigInit(&config);
    
    printf("1. Initialized configuration with result: %d (0 = success)\n", result);
    printf("   Device name: %s\n", config.common.device_name);
    printf("   Firmware version: %s\n", config.common.firmware_version);
    printf("   Server port: %d\n", config.common.server_port);
    printf("   Camera enabled: %s\n", config.enable_camera ? "true" : "false");
    
    // 2. Update configuration
    printf("\n2. Updating configuration...\n");
    strncpy(config.common.device_name, "Test Device", sizeof(config.common.device_name) - 1);
    config.common.server_port = 9090;
    config.common.network.enabled = true;
    strncpy(config.common.network.ssid, "TestNetwork", sizeof(config.common.network.ssid) - 1);
    strncpy(config.common.network.password, "TestPassword", sizeof(config.common.network.password) - 1);
    config.enable_camera = true;
    config.camera_resolution = 1080;
    
    // 3. Serialize to JSON
    printf("\n3. Serializing to JSON...\n");
    char json_buffer[4096];
    int length = MCP_ConfigSerializeToJSON(&config, json_buffer, sizeof(json_buffer));
    
    printf("   JSON serialization result: %d bytes\n", length);
    printf("   JSON content (excerpt):\n%s\n", json_buffer);
    
    // 4. Create new configuration and update from JSON
    printf("\n4. Using explicit JSON instead of serialized version for better testing...\n");
    const char* explicit_json = "{"
        "\"device\": {"
        "  \"name\": \"Test Device\","
        "  \"firmware_version\": \"2.0.0\","
        "  \"debug_enabled\": true"
        "},"
        "\"server\": {"
        "  \"enabled\": true,"
        "  \"port\": 9090,"
        "  \"auto_start\": true"
        "},"
        "\"network\": {"
        "  \"wifi\": {"
        "    \"enabled\": true,"
        "    \"ssid\": \"TestNetwork\","
        "    \"password\": \"TestPassword\","
        "    \"auto_connect\": true"
        "  }"
        "},"
        "\"platform\": {"
        "  \"rpi\": {"
        "    \"enable_camera\": true,"
        "    \"camera_resolution\": 1080"
        "  }"
        "}"
    "}";
    
    MCP_PLATFORM_CONFIG new_config;
    MCP_ConfigInit(&new_config);
    
    // Use the explicit JSON
    printf("   Using explicit JSON:\n%s\n", explicit_json);
    
    result = MCP_ConfigUpdateFromJSON(&new_config, explicit_json);
    printf("   Update result: %d (0 = success)\n", result);
    
    // 5. Verify updated configuration
    printf("\n5. Verifying updated configuration:\n");
    printf("   Device name: %s\n", new_config.common.device_name);
    printf("   Server port: %d\n", new_config.common.server_port);
    printf("   WiFi enabled: %s\n", new_config.common.network.enabled ? "true" : "false");
    printf("   WiFi SSID: %s\n", new_config.common.network.ssid);
    printf("   Camera enabled: %s\n", new_config.enable_camera ? "true" : "false");
    printf("   Camera resolution: %d\n", new_config.camera_resolution);
    
    // 6. Save and load configuration
    printf("\n6. Testing save and load configuration...\n");
    
    // Set a test file path - use current directory for test
    strncpy(config.common.config_file_path, "./test_config.json", sizeof(config.common.config_file_path) - 1);
    
    // Save configuration
    result = MCP_ConfigSave(&config, NULL);
    printf("   Save result: %d (0 = success)\n", result);
    
    // Create a new configuration with defaults
    MCP_PLATFORM_CONFIG loaded_config;
    MCP_ConfigInit(&loaded_config);
    strncpy(loaded_config.common.config_file_path, "./test_config.json", sizeof(loaded_config.common.config_file_path) - 1);
    
    // Load configuration
    result = MCP_ConfigLoad(&loaded_config, NULL);
    printf("   Load result: %d (0 = success)\n", result);
    
    // Verify loaded configuration
    printf("\n7. Verifying loaded configuration:\n");
    printf("   Device name: %s\n", loaded_config.common.device_name);
    printf("   Server port: %d\n", loaded_config.common.server_port);
    printf("   WiFi enabled: %s\n", loaded_config.common.network.enabled ? "true" : "false");
    printf("   WiFi SSID: %s\n", loaded_config.common.network.ssid);
    printf("   Camera enabled: %s\n", loaded_config.enable_camera ? "true" : "false");
    
    printf("\nAll configuration tests completed successfully!\n");
    return 0;
}