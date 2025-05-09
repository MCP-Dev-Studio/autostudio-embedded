/**
 * @file config_example.c
 * @brief Example of using the MCP configuration system
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Include appropriate platform header
#if defined(MCP_PLATFORM_RPI) || defined(RASPBERRY_PI)
#include "hal/rpi/mcp_rpi.h"
#elif defined(MCP_PLATFORM_ESP32) || defined(ESP32)
#include "hal/esp32/mcp_esp32.h"
#elif defined(MCP_PLATFORM_ARDUINO) || defined(ARDUINO)
#include "hal/arduino/mcp_arduino.h"
#elif defined(MCP_PLATFORM_MBED) || defined(MBED)
#include "hal/mbed/mcp_mbed.h"
#endif

int main(void) {
    printf("MCP Configuration System Example\n");
    printf("================================\n\n");
    
    // 1. Initialize with default configuration
    printf("1. Initializing with default configuration...\n");
    
#if defined(MCP_PLATFORM_RPI) || defined(RASPBERRY_PI)
    // Raspberry Pi specific initialization
    MCP_RPiConfig config = {
        .deviceName = "My RPi Device",
        .version = "1.0.0",
        .enableDebug = true,
        .enablePersistence = true,
        .configFile = "/etc/mcp/config.json",
        .enableServer = true,
        .serverPort = 8080,
        .autoStartServer = true,
        .enableWifi = true,
        .ssid = "MyNetwork",
        .password = "MyPassword",
        .enableCamera = true,
        .cameraResolution = 720
    };
    MCP_SystemInit(&config);
    
#elif defined(MCP_PLATFORM_ESP32) || defined(ESP32)
    // ESP32 specific initialization
    MCP_ESP32Config config = {
        .deviceName = "My ESP32 Device",
        .version = "1.0.0",
        .enableDebug = true,
        .enablePersistence = true,
        .configFile = "/config.json",
        .enableServer = true,
        .serverPort = 8080,
        .autoStartServer = true,
        .enableWifi = true,
        .ssid = "MyNetwork",
        .password = "MyPassword",
        .enableOTA = true,
        .enableWebServer = true,
        .webServerPort = 80
    };
    MCP_SystemInit(&config);
    
#elif defined(MCP_PLATFORM_ARDUINO) || defined(ARDUINO)
    // Arduino specific initialization
    MCP_ArduinoConfig config = {
        .deviceName = "My Arduino Device",
        .version = "1.0.0",
        .enableDebug = true,
        .enablePersistence = true,
        .configFile = "/config.json",
        .enableServer = true,
        .serverPort = 8080,
        .autoStartServer = true,
        .enableWifi = true,
        .ssid = "MyNetwork",
        .password = "MyPassword",
        .analogReference = 0,
        .enableWatchdog = true
    };
    MCP_SystemInit(&config);
    
#elif defined(MCP_PLATFORM_MBED) || defined(MBED)
    // Mbed specific initialization
    MCP_MbedConfig config = {
        .deviceName = "My Mbed Device",
        .version = "1.0.0",
        .enableDebug = true,
        .enablePersistence = true,
        .configFile = "/config.json",
        .enableServer = true,
        .serverPort = 8080,
        .autoStartServer = true,
        .enableRTOS = true,
        .taskStackSize = 4096
    };
    MCP_SystemInit(&config);
    
#else
    // Generic initialization without platform-specific features
    printf("No specific platform defined, using generic initialization\n");
    MCP_SystemInit(NULL);
#endif
    
    // 2. Get and display current configuration
    printf("\n2. Current configuration:\n");
    char config_json[4096];
    int len = MCP_GetConfiguration(config_json, sizeof(config_json));
    if (len > 0) {
        printf("%s\n", config_json);
    } else {
        printf("Failed to get configuration\n");
    }
    
    // 3. Update configuration via JSON
    printf("\n3. Updating configuration via JSON...\n");
    const char* update_json = "{"
        "\"device\": {"
        "  \"name\": \"Updated Device Name\","
        "  \"debug_enabled\": true"
        "},"
        "\"server\": {"
        "  \"port\": 9090"
        "},"
        "\"network\": {"
        "  \"wifi\": {"
        "    \"ssid\": \"NewNetwork\","
        "    \"password\": \"NewPassword\""
        "  }"
        "}"
    "}";
    
    int result = MCP_SetConfiguration(update_json);
    if (result == 0) {
        printf("Configuration updated successfully\n");
    } else {
        printf("Failed to update configuration: %d\n", result);
    }
    
    // 4. Get and display updated configuration
    printf("\n4. Updated configuration:\n");
    len = MCP_GetConfiguration(config_json, sizeof(config_json));
    if (len > 0) {
        printf("%s\n", config_json);
    } else {
        printf("Failed to get configuration\n");
    }
    
    // 5. Get system status
    printf("\n5. System status:\n");
    char status_json[1024];
    len = MCP_SystemGetStatus(status_json, sizeof(status_json));
    if (len > 0) {
        printf("%s\n", status_json);
    } else {
        printf("Failed to get system status\n");
    }
    
    // 6. Save configuration to persistent storage
    printf("\n6. Saving configuration to persistent storage...\n");
    result = MCP_SavePersistentState();
    if (result == 0) {
        printf("Configuration saved successfully\n");
    } else {
        printf("Failed to save configuration: %d\n", result);
    }
    
    printf("\nConfiguration example completed\n");
    return 0;
}