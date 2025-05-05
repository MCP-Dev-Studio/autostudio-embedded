/**
 * @file test_device_info.c
 * @brief Test device information system
 */
#include "core/device/device_info.h"
#include "core/tool_system/tool_registry.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>

// Example device capabilities
const char* DEVICE_CAPABILITIES = "{"
    "\"features\": ["
        "\"gpio\","
        "\"i2c\","
        "\"spi\","
        "\"uart\","
        "\"adc\","
        "\"pwm\""
    "],"
    "\"maxTools\": 64,"
    "\"supportedToolTypes\": ["
        "\"native\","
        "\"composite\","
        "\"script\""
    "],"
    "\"supportedProtocols\": ["
        "\"mqtt\","
        "\"http\","
        "\"websocket\""
    "],"
    "\"maxConnections\": 5,"
    "\"securityFeatures\": ["
        "\"tls\","
        "\"authentication\""
    "]"
"}";

/**
 * @brief Test accessing device info directly
 */
void testDirectAccess(void) {
    printf("\n==== Testing Direct Access ====\n");
    
    // Get device information
    const MCP_DeviceInfo* deviceInfo = MCP_DeviceInfoGet();
    if (deviceInfo == NULL) {
        printf("Failed to get device information\n");
        return;
    }
    
    // Print system information
    printf("Device Name: %s\n", deviceInfo->system.deviceName);
    printf("Platform: %s\n", deviceInfo->system.platformName);
    printf("Firmware Version: %s\n", deviceInfo->system.firmwareVersion);
    printf("Build Date: %s %s\n", deviceInfo->system.buildDate, deviceInfo->system.buildTime);
    printf("Uptime: %u seconds\n", deviceInfo->system.uptime);
    
    // Print processor information
    printf("\nProcessor:\n");
    printf("  Model: %s\n", deviceInfo->processor.model);
    printf("  Clock Speed: %u MHz\n", deviceInfo->processor.clockSpeed / 1000000);
    printf("  Cores: %u\n", deviceInfo->processor.coreCount);
    
    // Print memory information
    printf("\nMemory:\n");
    printf("  Total RAM: %u KB\n", deviceInfo->memory.totalRam / 1024);
    printf("  Free RAM: %u KB\n", deviceInfo->memory.freeRam / 1024);
    printf("  Total Flash: %u KB\n", deviceInfo->memory.totalFlash / 1024);
    
    // Print IO port count
    printf("\nIO Ports: %u\n", deviceInfo->ioPortCount);
    
    // Print network interface count
    printf("Network Interfaces: %u\n", deviceInfo->networkInterfaceCount);
    
    // Print sensor count
    printf("Sensors: %u\n", deviceInfo->sensorCount);
    
    // Print storage device count
    printf("Storage Devices: %u\n", deviceInfo->storageCount);
}

/**
 * @brief Test registering custom components
 */
void testRegisterComponents(void) {
    printf("\n==== Testing Component Registration ====\n");
    
    // Register a custom sensor
    MCP_SensorInfo pressureSensor = {0};
    strncpy(pressureSensor.name, "Pressure Sensor", sizeof(pressureSensor.name) - 1);
    strncpy(pressureSensor.type, "pressure", sizeof(pressureSensor.type) - 1);
    strncpy(pressureSensor.units, "hPa", sizeof(pressureSensor.units) - 1);
    pressureSensor.minValue = 300.0f;
    pressureSensor.maxValue = 1100.0f;
    pressureSensor.currentValue = 1013.25f;
    pressureSensor.isConnected = true;
    strncpy(pressureSensor.busType, "i2c", sizeof(pressureSensor.busType) - 1);
    pressureSensor.address = 0x76;
    
    if (MCP_DeviceInfoRegisterSensor(&pressureSensor) == 0) {
        printf("Successfully registered pressure sensor\n");
    } else {
        printf("Failed to register pressure sensor\n");
    }
    
    // Set device capabilities
    if (MCP_DeviceInfoSetCapabilities(DEVICE_CAPABILITIES) == 0) {
        printf("Successfully set device capabilities\n");
    } else {
        printf("Failed to set device capabilities\n");
    }
    
    // Update device information
    MCP_DeviceInfoUpdate();
}

/**
 * @brief Test JSON serialization
 */
void testJsonSerialization(void) {
    printf("\n==== Testing JSON Serialization ====\n");
    
    // Get device info as JSON
    char* json = MCP_DeviceInfoToJSON(false);
    if (json == NULL) {
        printf("Failed to serialize device info to JSON\n");
        return;
    }
    
    // Print JSON (truncated for readability)
    printf("Device Info JSON (truncated):\n");
    // Only print the first 500 characters to avoid flooding console
    int jsonLen = strlen(json);
    if (jsonLen > 500) {
        char truncatedJson[501];
        strncpy(truncatedJson, json, 500);
        truncatedJson[500] = '\0';
        printf("%s...\n(truncated, full length: %d characters)\n", truncatedJson, jsonLen);
    } else {
        printf("%s\n", json);
    }
    
    // Free JSON
    free(json);
    
    // Get device info as compact JSON
    json = MCP_DeviceInfoToJSON(true);
    if (json == NULL) {
        printf("Failed to serialize device info to compact JSON\n");
        return;
    }
    
    // Print JSON (truncated for readability)
    printf("\nCompact Device Info JSON (truncated):\n");
    // Only print the first 500 characters to avoid flooding console
    jsonLen = strlen(json);
    if (jsonLen > 500) {
        char truncatedJson[501];
        strncpy(truncatedJson, json, 500);
        truncatedJson[500] = '\0';
        printf("%s...\n(truncated, full length: %d characters)\n", truncatedJson, jsonLen);
    } else {
        printf("%s\n", json);
    }
    
    // Free JSON
    free(json);
}

/**
 * @brief Test tool interface
 */
void testToolInterface(void) {
    printf("\n==== Testing Tool Interface ====\n");
    
    // Initialize tool registry
    if (MCP_ToolRegistryInit(32) != 0) {
        printf("Failed to initialize tool registry\n");
        return;
    }
    
    // Execute getDeviceInfo tool
    printf("Executing system.getDeviceInfo tool...\n");
    
    // Create tool execution JSON
    const char* toolJson = "{\"tool\":\"system.getDeviceInfo\",\"params\":{\"format\":\"compact\"}}";
    
    // Execute tool
    MCP_ToolResult result = MCP_ToolExecute(toolJson, strlen(toolJson));
    
    // Check result
    if (result.status != 0) {
        printf("Tool execution failed, status: %d\n", result.status);
        return;
    }
    
    // Print result JSON (truncated for readability)
    printf("Tool result JSON (truncated):\n");
    // Only print the first 500 characters to avoid flooding console
    int jsonLen = strlen(result.resultJson);
    if (jsonLen > 500) {
        char truncatedJson[501];
        strncpy(truncatedJson, result.resultJson, 500);
        truncatedJson[500] = '\0';
        printf("%s...\n(truncated, full length: %d characters)\n", truncatedJson, jsonLen);
    } else {
        printf("%s\n", result.resultJson);
    }
    
    // Free result
    if (result.resultJson != NULL) {
        free((void*)result.resultJson);
    }
}

/**
 * @brief Main function
 */
int main(void) {
    printf("=== Device Information System Test ===\n");
    
    // Initialize device info
    MCP_DeviceInfoInit();
    
    // Test direct access to device info
    testDirectAccess();
    
    // Test registering components
    testRegisterComponents();
    
    // Test JSON serialization
    testJsonSerialization();
    
    // Test tool interface
    testToolInterface();
    
    printf("\n=== Device Information System Test Completed ===\n");
    return 0;
}