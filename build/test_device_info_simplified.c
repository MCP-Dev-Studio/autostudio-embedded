#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device_info_structs.h"

// Use our mock implementations
extern int MCP_DeviceInfoInit(void);
extern const MCP_DeviceInfo* MCP_DeviceInfoGet(void);
extern char* MCP_DeviceInfoToJSON(bool compact);

int main(void) {
    printf("=== Device Information System Test (Simplified) ===\n\n");
    
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
    
    // Clean up
    free(json);
    
    printf("=== All tests passed successfully! ===\n");
    return 0;
}
