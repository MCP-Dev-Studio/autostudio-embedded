#include "device_info_structs.h"
#include "mock_tool_registry.h"
#include <string.h>
#include <stdio.h>

// Declare the JSON utility functions
char* json_get_string_field(const char* json, const char* field);
bool json_validate_schema(const char* json, const char* schema);

// Static device info
static MCP_DeviceInfo s_deviceInfo = {0};
static bool s_initialized = false;

// Component arrays (static allocation)
static MCP_IOPortInfo s_ioPorts[64] = {0};
static MCP_NetworkInfo s_networkInterfaces[4] = {0};
static MCP_SensorInfo s_sensors[16] = {0};
static MCP_StorageInfo s_storageDevices[4] = {0};

// Device info initialization
int MCP_DeviceInfoInit(void) {
    if (s_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize with some basic data
    strncpy(s_deviceInfo.system.deviceName, "Test Device", sizeof(s_deviceInfo.system.deviceName) - 1);
    strncpy(s_deviceInfo.system.firmwareVersion, "1.0.0", sizeof(s_deviceInfo.system.firmwareVersion) - 1);
    strncpy(s_deviceInfo.system.buildDate, __DATE__, sizeof(s_deviceInfo.system.buildDate) - 1);
    strncpy(s_deviceInfo.system.buildTime, __TIME__, sizeof(s_deviceInfo.system.buildTime) - 1);
    strncpy(s_deviceInfo.system.platformName, "Test Platform", sizeof(s_deviceInfo.system.platformName) - 1);
    
    strncpy(s_deviceInfo.processor.model, "Test CPU", sizeof(s_deviceInfo.processor.model) - 1);
    s_deviceInfo.processor.clockSpeed = 100 * 1000 * 1000; // 100 MHz
    s_deviceInfo.processor.coreCount = 1;
    s_deviceInfo.processor.bitWidth = 32;
    
    s_deviceInfo.memory.totalRam = 64 * 1024; // 64 KB
    s_deviceInfo.memory.freeRam = 32 * 1024; // 32 KB
    s_deviceInfo.memory.totalFlash = 256 * 1024; // 256 KB
    s_deviceInfo.memory.freeFlash = 128 * 1024; // 128 KB
    
    // Set up component arrays
    s_deviceInfo.ioPorts = s_ioPorts;
    s_deviceInfo.ioPortCount = 0;
    s_deviceInfo.networkInterfaces = s_networkInterfaces;
    s_deviceInfo.networkInterfaceCount = 0;
    s_deviceInfo.sensors = s_sensors;
    s_deviceInfo.sensorCount = 0;
    s_deviceInfo.storageDevices = s_storageDevices;
    s_deviceInfo.storageCount = 0;
    
    // Add a sample IO port
    MCP_IOPortInfo portInfo = {0};
    strncpy(portInfo.name, "D0", sizeof(portInfo.name) - 1);
    portInfo.portType = 0; // Digital
    portInfo.isInput = true;
    portInfo.isOutput = true;
    memcpy(&s_ioPorts[0], &portInfo, sizeof(MCP_IOPortInfo));
    s_deviceInfo.ioPortCount = 1;
    
    // Add a sample network interface
    MCP_NetworkInfo netInfo = {0};
    strncpy(netInfo.name, "WiFi", sizeof(netInfo.name) - 1);
    strncpy(netInfo.macAddress, "00:11:22:33:44:55", sizeof(netInfo.macAddress) - 1);
    strncpy(netInfo.ipAddress, "192.168.1.100", sizeof(netInfo.ipAddress) - 1);
    netInfo.isConnected = true;
    netInfo.isWifi = true;
    memcpy(&s_networkInterfaces[0], &netInfo, sizeof(MCP_NetworkInfo));
    s_deviceInfo.networkInterfaceCount = 1;
    
    s_initialized = true;
    return 0;
}

// Get device info
const MCP_DeviceInfo* MCP_DeviceInfoGet(void) {
    if (!s_initialized) {
        MCP_DeviceInfoInit();
    }
    return &s_deviceInfo;
}

// Convert device info to JSON (simplified)
char* MCP_DeviceInfoToJSON(bool compact) {
    (void)compact; // Unused parameter
    
    if (!s_initialized) {
        MCP_DeviceInfoInit();
    }
    
    // Simple static JSON (just enough for tests)
    char* json = strdup("{\"system\":{\"deviceName\":\"Test Device\",\"firmwareVersion\":\"1.0.0\"},\"processor\":{\"model\":\"Test CPU\",\"clockSpeedMHz\":100,\"coreCount\":1,\"bitWidth\":32},\"memory\":{\"totalRamKB\":64,\"freeRamKB\":32}}");
    return json;
}

// Device info tool handler
MCP_ToolResult MCP_DeviceInfoToolHandler(const char* json, size_t length) {
    // Check if format parameter is present
    bool compact = false;
    
    if (json != NULL && length > 0) {
        char* formatStr = json_get_string_field(json, "format");
        if (formatStr != NULL) {
            if (strcmp(formatStr, "compact") == 0) {
                compact = true;
            }
            free(formatStr);
        }
    }
    
    // Get device info as JSON
    char* deviceInfoJson = MCP_DeviceInfoToJSON(compact);
    if (deviceInfoJson == NULL) {
        return MCP_ToolCreateErrorResult(1, "Failed to generate device info");
    }
    
    // Create result
    MCP_ToolResult result;
    result.status = MCP_TOOL_RESULT_SUCCESS;
    result.resultJson = deviceInfoJson;
    result.resultData = NULL;
    result.resultDataSize = 0;
    
    return result;
}
