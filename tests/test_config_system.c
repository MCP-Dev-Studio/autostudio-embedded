#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Define a platform for testing (we'll use RPi for this test)
#define PLATFORM_RPI

#include "../src/core/mcp/config/mcp_config.h"
#include "../src/core/mcp/config/platform_config.h"
#include "../src/hal/rpi/mcp_rpi.h"

// Test configuration initialization
static void test_config_init() {
    printf("Testing configuration initialization...\n");
    
    MCP_PLATFORM_CONFIG config;
    int result = MCP_ConfigInit(&config);
    assert(result == 0);
    
    // Verify default values
    assert(strcmp(config.common.device_name, "MCP Device") == 0);
    assert(strcmp(config.common.firmware_version, "1.0.0") == 0);
    assert(config.common.debug_enabled == false);
    assert(config.common.server_enabled == true);
    assert(config.common.server_port == 8080);
    assert(config.common.auto_start_server == true);
    
    printf("Configuration initialization test passed!\n\n");
}

// Test JSON serialization
static void test_json_serialization() {
    printf("Testing JSON serialization...\n");
    
    MCP_PLATFORM_CONFIG config;
    MCP_ConfigInit(&config);
    
    // Modify some values
    strncpy(config.common.device_name, "Test Device", sizeof(config.common.device_name) - 1);
    config.common.debug_enabled = true;
    config.common.server_port = 9090;
    
    // Serialize to JSON
    char json_buffer[8192];
    int json_length = MCP_ConfigSerializeToJSON(&config, json_buffer, sizeof(json_buffer));
    assert(json_length > 0);
    
    // Verify JSON contains our settings
    assert(strstr(json_buffer, "\"name\": \"Test Device\"") != NULL);
    assert(strstr(json_buffer, "\"debug_enabled\": true") != NULL);
    assert(strstr(json_buffer, "\"port\": 9090") != NULL);
    
    printf("JSON serialization test passed!\n\n");
}

// Test JSON deserialization
static void test_json_deserialization() {
    printf("Testing JSON deserialization...\n");
    
    MCP_PLATFORM_CONFIG config;
    MCP_ConfigInit(&config);
    
    // Simple JSON with a few key settings
    const char* test_json = "{"
        "\"device\": {"
        "  \"name\": \"JSON Test Device\","
        "  \"firmware_version\": \"2.0.0\","
        "  \"debug_enabled\": true"
        "},"
        "\"server\": {"
        "  \"port\": 7070"
        "},"
        "\"network\": {"
        "  \"wifi\": {"
        "    \"enabled\": true,"
        "    \"ssid\": \"TestNetwork\","
        "    \"password\": \"TestPassword\""
        "  }"
        "}"
    "}";
    
    // Update config from JSON
    int result = MCP_ConfigUpdateFromJSON(&config, test_json);
    assert(result == 0);
    
    // Verify values were updated
    assert(strcmp(config.common.device_name, "JSON Test Device") == 0);
    assert(strcmp(config.common.firmware_version, "2.0.0") == 0);
    assert(config.common.debug_enabled == true);
    assert(config.common.server_port == 7070);
    assert(config.common.network.enabled == true);
    assert(strcmp(config.common.network.ssid, "TestNetwork") == 0);
    assert(strcmp(config.common.network.password, "TestPassword") == 0);
    
    printf("JSON deserialization test passed!\n\n");
}

// Test platform configuration mapping
static void test_platform_config_mapping() {
    printf("Testing platform configuration mapping...\n");
    
    // Create a platform-specific configuration
    MCP_RPiConfig rpi_config = {
        .deviceName = "Platform Test Device",
        .version = "3.0.0",
        .enableDebug = true,
        .enablePersistence = true,
        .heapSize = 2048000,
        .configFile = "/tmp/test_config.json",
        .enableServer = true,
        .serverPort = 5050,
        .autoStartServer = false,
        .enableWifi = true,
        .ssid = "PlatformNetwork",
        .password = "PlatformPassword",
        .enableCamera = true,
        .cameraResolution = 1080
    };
    
    // Convert to common config
    MCP_CommonConfig common_config;
    int result = PlatformConfigToCommonConfig(&rpi_config, &common_config);
    assert(result == 0);
    
    // Verify mapping
    assert(strcmp(common_config.device_name, "Platform Test Device") == 0);
    assert(strcmp(common_config.firmware_version, "3.0.0") == 0);
    assert(common_config.debug_enabled == true);
    assert(common_config.enable_persistence == true);
    assert(common_config.heap_size == 2048000);
    assert(strcmp(common_config.config_file_path, "/tmp/test_config.json") == 0);
    assert(common_config.server_enabled == true);
    assert(common_config.server_port == 5050);
    assert(common_config.auto_start_server == false);
    assert(common_config.network.enabled == true);
    assert(strcmp(common_config.network.ssid, "PlatformNetwork") == 0);
    assert(strcmp(common_config.network.password, "PlatformPassword") == 0);
    
    // Now convert back to platform config
    MCP_RPiConfig new_rpi_config = {0};
    result = CommonConfigToPlatformConfig(&common_config, &new_rpi_config);
    assert(result == 0);
    
    // Verify reverse mapping
    assert(strcmp(new_rpi_config.deviceName, "Platform Test Device") == 0);
    assert(strcmp(new_rpi_config.version, "3.0.0") == 0);
    assert(new_rpi_config.enableDebug == true);
    assert(new_rpi_config.enablePersistence == true);
    assert(new_rpi_config.heapSize == 2048000);
    assert(strcmp(new_rpi_config.configFile, "/tmp/test_config.json") == 0);
    assert(new_rpi_config.enableServer == true);
    assert(new_rpi_config.serverPort == 5050);
    assert(new_rpi_config.autoStartServer == false);
    assert(new_rpi_config.enableWifi == true);
    assert(strcmp(new_rpi_config.ssid, "PlatformNetwork") == 0);
    assert(strcmp(new_rpi_config.password, "PlatformPassword") == 0);
    
    printf("Platform configuration mapping test passed!\n\n");
}

// Test file I/O (save and load)
static void test_file_io() {
    printf("Testing configuration file I/O...\n");
    
    // Create a test configuration
    MCP_PLATFORM_CONFIG config;
    MCP_ConfigInit(&config);
    
    // Set some unique values for testing
    strncpy(config.common.device_name, "File IO Test Device", sizeof(config.common.device_name) - 1);
    config.common.debug_enabled = true;
    config.common.server_port = 6060;
    strncpy(config.common.network.ssid, "FileIONetwork", sizeof(config.common.network.ssid) - 1);
    strncpy(config.common.network.password, "FileIOPassword", sizeof(config.common.network.password) - 1);
    
    // Set a test file path
    strncpy(config.common.config_file_path, "/tmp/mcp_test_config.json", sizeof(config.common.config_file_path) - 1);
    
    // Save configuration
    int result = MCP_ConfigSave(&config, NULL);
    assert(result == 0);
    
    // Create a new configuration with defaults
    MCP_PLATFORM_CONFIG loaded_config;
    MCP_ConfigInit(&loaded_config);
    
    // Set the same file path
    strncpy(loaded_config.common.config_file_path, "/tmp/mcp_test_config.json", sizeof(loaded_config.common.config_file_path) - 1);
    
    // Load configuration
    result = MCP_ConfigLoad(&loaded_config, NULL);
    assert(result == 0);
    
    // Verify loaded values match original values
    assert(strcmp(loaded_config.common.device_name, "File IO Test Device") == 0);
    assert(loaded_config.common.debug_enabled == true);
    assert(loaded_config.common.server_port == 6060);
    assert(strcmp(loaded_config.common.network.ssid, "FileIONetwork") == 0);
    assert(strcmp(loaded_config.common.network.password, "FileIOPassword") == 0);
    
    printf("Configuration file I/O test passed!\n\n");
}

// Test platform-independent API
static void test_platform_independent_api() {
    printf("Testing platform-independent API...\n");
    
    // Reset the global configuration
    MCP_PLATFORM_CONFIG config;
    MCP_ConfigInit(&config);
    
    // Use the platform-independent API to update configuration
    const char* test_json = "{"
        "\"device\": {"
        "  \"name\": \"API Test Device\","
        "  \"firmware_version\": \"4.0.0\""
        "},"
        "\"server\": {"
        "  \"port\": 8888"
        "}"
    "}";
    
    int result = MCP_SetConfiguration(test_json);
    assert(result == 0);
    
    // Get configuration using platform-independent API
    char buffer[4096];
    int len = MCP_GetConfiguration(buffer, sizeof(buffer));
    assert(len > 0);
    
    // Verify buffer contains our updates
    assert(strstr(buffer, "\"name\": \"API Test Device\"") != NULL);
    assert(strstr(buffer, "\"firmware_version\": \"4.0.0\"") != NULL);
    assert(strstr(buffer, "\"port\": 8888") != NULL);
    
    printf("Platform-independent API test passed!\n\n");
}

// Main test function
int main() {
    printf("=== MCP Configuration System Test ===\n\n");
    
    test_config_init();
    test_json_serialization();
    test_json_deserialization();
    test_platform_config_mapping();
    test_file_io();
    test_platform_independent_api();
    
    printf("All tests passed successfully!\n");
    return 0;
}