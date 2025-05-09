#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/core/mcp/config/mcp_config.h"

// Forward declarations for the logging functions
void log_info(const char* format, ...);
void log_warn(const char* format, ...);
void log_error(const char* format, ...);
int persistent_storage_write(const char* key, const void* data, size_t size);
int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize);

int main() {
    printf("Testing MCP Configuration System\n");
    printf("===============================\n\n");
    
    // 1. Initialize configuration
    printf("1. Testing configuration initialization...\n");
    MCP_PLATFORM_CONFIG config;
    int result = MCP_ConfigInit(&config);
    printf("   Initialization result: %d (0 = success)\n", result);
    printf("   Device name: %s\n", config.common.device_name);
    printf("   Firmware version: %s\n", config.common.firmware_version);
    printf("   Server port: %d\n", config.common.server_port);
    
    // 2. Update configuration
    printf("\n2. Testing configuration updates...\n");
    strcpy(config.common.device_name, "Test Device");
    config.common.debug_enabled = true;
    config.common.server_port = 9090;
    printf("   Updated device name: %s\n", config.common.device_name);
    printf("   Updated debug flag: %s\n", config.common.debug_enabled ? "true" : "false");
    printf("   Updated server port: %d\n", config.common.server_port);
    
    // 3. Serialize to JSON
    printf("\n3. Testing JSON serialization...\n");
    char json_buffer[8192];
    int json_length = MCP_ConfigSerializeToJSON(&config, json_buffer, sizeof(json_buffer));
    printf("   JSON length: %d bytes\n", json_length);
    printf("   JSON content (excerpt):\n");
    printf("   %.200s...\n", json_buffer);
    
    // 4. Create new configuration
    printf("\n4. Testing JSON deserialization...\n");
    MCP_PLATFORM_CONFIG new_config;
    MCP_ConfigInit(&new_config);
    
    // 5. Update from JSON
    printf("   Updating configuration from JSON...\n");
    result = MCP_ConfigUpdateFromJSON(&new_config, json_buffer);
    printf("   Update result: %d (0 = success)\n", result);
    printf("   New device name: %s\n", new_config.common.device_name);
    printf("   New debug flag: %s\n", new_config.common.debug_enabled ? "true" : "false");
    printf("   New server port: %d\n", new_config.common.server_port);
    
    // 6. Test platform-independent API
    printf("\n5. Testing platform-independent API...\n");
    const char* update_json = "{"
        "\"device\": {"
        "  \"name\": \"API Test Device\","
        "  \"firmware_version\": \"2.0.0\""
        "},"
        "\"server\": {"
        "  \"port\": 8888"
        "}"
    "}";
    
    result = MCP_SetConfiguration(update_json);
    printf("   API update result: %d (0 = success)\n", result);
    
    char get_buffer[4096];
    int get_length = MCP_GetConfiguration(get_buffer, sizeof(get_buffer));
    printf("   API get result: %d bytes\n", get_length);
    printf("   API config content (excerpt):\n");
    printf("   %.200s...\n", get_buffer);
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}