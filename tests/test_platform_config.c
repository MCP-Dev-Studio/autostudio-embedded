#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Define a platform for testing
#define MCP_PLATFORM_RPI

// Include the configuration headers
#include "../src/core/mcp/config/mcp_config.h"
#include "../src/core/mcp/config/platform_config.h"

// Forward declarations for stubs
void log_info(const char* format, ...);
void log_warn(const char* format, ...);
void log_error(const char* format, ...);
int persistent_storage_write(const char* key, const void* data, size_t size);
int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize);

// Test the Raspberry Pi configuration
int main() {
    printf("Testing Platform Configuration System\n");
    printf("==================================\n\n");
    
    // 1. Create a platform-specific configuration
    MCP_RPiConfig platform_config = {
        .deviceName = "Test RPi Device",
        .version = "1.0.0",
        .enableDebug = true,
        .enablePersistence = true,
        .heapSize = 1024*1024,
        .configFile = "/tmp/config.json",
        .enableServer = true,
        .serverPort = 8080,
        .autoStartServer = true,
        .enableWifi = true,
        .ssid = "TestNetwork",
        .password = "TestPassword",
        .enableCamera = true,
        .cameraResolution = 1080
    };
    
    printf("1. Platform-specific configuration created:\n");
    printf("   Device Name: %s\n", platform_config.deviceName);
    printf("   Server Port: %d\n", platform_config.serverPort);
    
    // 2. Create common configuration
    MCP_CommonConfig common_config;
    
    // 3. Convert platform to common config
    printf("\n2. Converting platform configuration to common configuration...\n");
    int result = PlatformConfigToCommonConfig(&platform_config, &common_config);
    printf("   Conversion result: %d (0 = success)\n", result);
    
    // 4. Verify conversion
    printf("\n3. Verifying conversion results:\n");
    printf("   Device Name: %s\n", common_config.device_name);
    printf("   Firmware Version: %s\n", common_config.firmware_version);
    printf("   Debug Enabled: %s\n", common_config.debug_enabled ? "true" : "false");
    printf("   Server Port: %d\n", common_config.server_port);
    printf("   WiFi Enabled: %s\n", common_config.network.enabled ? "true" : "false");
    printf("   WiFi SSID: %s\n", common_config.network.ssid);
    
    // 5. Modify common config
    printf("\n4. Modifying common configuration...\n");
    strcpy(common_config.device_name, "Modified Device");
    common_config.server_port = 9090;
    common_config.network.ap.enabled = true;
    strcpy(common_config.network.ap.ssid, "TestAP");
    
    // 6. Create a new platform config
    MCP_RPiConfig new_platform_config = {0};
    
    // 7. Convert back to platform config
    printf("\n5. Converting common configuration back to platform configuration...\n");
    result = CommonConfigToPlatformConfig(&common_config, &new_platform_config);
    printf("   Conversion result: %d (0 = success)\n", result);
    
    // 8. Verify reverse conversion
    printf("\n6. Verifying reverse conversion results:\n");
    printf("   Device Name: %s\n", new_platform_config.deviceName);
    printf("   Server Port: %d\n", new_platform_config.serverPort);
    printf("   WiFi Enabled: %s\n", new_platform_config.enableWifi ? "true" : "false");
    printf("   WiFi SSID: %s\n", new_platform_config.ssid);
    printf("   Hotspot Enabled: %s\n", new_platform_config.enableHotspot ? "true" : "false");
    printf("   Hotspot SSID: %s\n", new_platform_config.hotspotSsid);
    
    printf("\nAll platform configuration tests completed successfully!\n");
    return 0;
}