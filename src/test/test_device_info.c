/**
 * @file test_device_info.c
 * @brief Test device information system
 */
#include "core/device/device_info.h"
#include "logging.h"
#include "build_fix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    printf("=== Device Information System Test ===\n\n");
    
    // Initialize logging
    LogConfig logConfig = { .level = LOG_LEVEL_INFO };
    log_init(&logConfig);
    
    // Initialize device info system
    if (MCP_DeviceInfoInit() != 0) {
        printf("Failed to initialize device info system\n");
        return 1;
    }
    
    // Get device info
    const MCP_DeviceInfo* deviceInfo = MCP_DeviceInfoGet();
    if (deviceInfo == NULL) {
        printf("Failed to get device info\n");
        return 1;
    }
    
    // Print device info
    printf("Device Name: %s\n", deviceInfo->system.deviceName);
    printf("Firmware Version: %s\n", deviceInfo->system.firmwareVersion);
    printf("Platform: %s\n", deviceInfo->system.platformName);
    printf("\n");
    
    printf("Processor: %s\n", deviceInfo->processor.model);
    printf("Clock Speed: %d MHz\n", (int)(deviceInfo->processor.clockSpeed / (1000 * 1000)));
    printf("Core Count: %d\n", deviceInfo->processor.coreCount);
    printf("Bit Width: %d\n", deviceInfo->processor.bitWidth);
    printf("\n");
    
    printf("Total RAM: %d KB\n", (int)(deviceInfo->memory.totalRam / 1024));
    printf("Free RAM: %d KB\n", (int)(deviceInfo->memory.freeRam / 1024));
    printf("Total Flash: %d KB\n", (int)(deviceInfo->memory.totalFlash / 1024));
    printf("\n");
    
    // Get device info as JSON
    char* json = MCP_DeviceInfoToJSON(false);
    if (json == NULL) {
        printf("Failed to get device info as JSON\n");
        return 1;
    }
    
    printf("Device Info JSON:\n%s\n\n", json);
    
    // Test device info tool handler
    printf("Testing device info tool handler...\n");
    MCP_ToolResult result = MCP_DeviceInfoToolHandler("{\"format\":\"full\"}", 16);
    
    if (result.status != MCP_TOOL_RESULT_SUCCESS) {
        printf("Tool handler failed, status: %d\n", result.status);
        return 1;
    }
    
    printf("Tool handler succeeded, result:\n%s\n\n", result.resultJson);
    
    // Clean up
    free(json);
    if (result.resultJson) {
        free((void*)result.resultJson);
    }
    
    printf("=== All tests passed successfully! ===\n");
    return 0;
}
