/**
 * @file device_info.c
 * @brief Device information and capabilities system implementation
 */
#include "device_info.h"
#include "../../logging.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef ESP32
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "esp_flash.h"
#endif

#ifdef ARDUINO
#include <Arduino.h>
#endif

#ifdef MBED
#include "mbed.h"
#endif

// External JSON utility functions
extern char* json_get_string_field(const char* json, const char* field);
extern bool json_validate_schema(const char* json, const char* schema);

// Static device information
static MCP_DeviceInfo s_deviceInfo = {0};
static bool s_initialized = false;

// Maximum number of device components
#define MAX_IO_PORTS 64
#define MAX_NETWORK_INTERFACES 4
#define MAX_SENSORS 16
#define MAX_STORAGE_DEVICES 4

// Component arrays
static MCP_IOPortInfo s_ioPorts[MAX_IO_PORTS] = {0};
static MCP_NetworkInfo s_networkInterfaces[MAX_NETWORK_INTERFACES] = {0};
static MCP_SensorInfo s_sensors[MAX_SENSORS] = {0};
static MCP_StorageInfo s_storageDevices[MAX_STORAGE_DEVICES] = {0};

// Forward declarations
static void populateSystemInfo(void);
static void populateProcessorInfo(void);
static void populateMemoryInfo(void);
static void populateIOPortInfo(void);
static void populateNetworkInfo(void);
static void populateSensorInfo(void);
static void populateStorageInfo(void);

/**
 * @brief Initialize device information system
 */
int MCP_DeviceInfoInit(void) {
    if (s_initialized) {
        return -1; // Already initialized
    }
    
    // Initialize device info structure
    memset(&s_deviceInfo, 0, sizeof(MCP_DeviceInfo));
    
    // Set up component arrays
    s_deviceInfo.ioPorts = s_ioPorts;
    s_deviceInfo.ioPortCount = 0;
    
    s_deviceInfo.networkInterfaces = s_networkInterfaces;
    s_deviceInfo.networkInterfaceCount = 0;
    
    s_deviceInfo.sensors = s_sensors;
    s_deviceInfo.sensorCount = 0;
    
    s_deviceInfo.storageDevices = s_storageDevices;
    s_deviceInfo.storageCount = 0;
    
    // Populate static information
    populateSystemInfo();
    populateProcessorInfo();
    
    // Update dynamic information
    MCP_DeviceInfoUpdate();
    
    s_initialized = true;
    return 0;
}

/**
 * @brief Get current device information
 */
const MCP_DeviceInfo* MCP_DeviceInfoGet(void) {
    if (!s_initialized) {
        MCP_DeviceInfoInit();
    }
    
    return &s_deviceInfo;
}

/**
 * @brief Update device information (refreshes dynamic data)
 */
int MCP_DeviceInfoUpdate(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // Update dynamic information
    populateMemoryInfo();
    populateIOPortInfo();
    populateNetworkInfo();
    populateSensorInfo();
    populateStorageInfo();
    
    // Update system uptime
    static time_t startTime = 0;
    if (startTime == 0) {
        startTime = time(NULL);
    }
    
    s_deviceInfo.system.uptime = (uint32_t)(time(NULL) - startTime);
    
    return 0;
}

/**
 * @brief Convert device information to JSON
 */
char* MCP_DeviceInfoToJSON(bool compact) {
    if (!s_initialized) {
        MCP_DeviceInfoInit();
    }
    
    // Update dynamic information
    MCP_DeviceInfoUpdate();
    
    // Estimate JSON size
    size_t jsonSize = 4096; // Base size
    
    // Add space for IO ports
    jsonSize += s_deviceInfo.ioPortCount * 256;
    
    // Add space for network interfaces
    jsonSize += s_deviceInfo.networkInterfaceCount * 256;
    
    // Add space for sensors
    jsonSize += s_deviceInfo.sensorCount * 256;
    
    // Add space for storage devices
    jsonSize += s_deviceInfo.storageCount * 256;
    
    // Allocate buffer
    char* json = (char*)malloc(jsonSize);
    if (json == NULL) {
        return NULL;
    }
    
    // Start building JSON
    int offset = 0;
    
    // Open main object
    offset += snprintf(json + offset, jsonSize - offset, "{");
    
    // System information
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"system\":{");
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"deviceName\":\"%s\",", s_deviceInfo.system.deviceName);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"firmwareVersion\":\"%s\",", s_deviceInfo.system.firmwareVersion);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"buildDate\":\"%s\",", s_deviceInfo.system.buildDate);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"buildTime\":\"%s\",", s_deviceInfo.system.buildTime);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"platformName\":\"%s\",", s_deviceInfo.system.platformName);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"boardModel\":\"%s\",", s_deviceInfo.system.boardModel);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"uptime\":%u,", s_deviceInfo.system.uptime);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"resetCount\":%u,", s_deviceInfo.system.resetCount);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"lastResetReason\":\"%s\",", s_deviceInfo.system.lastResetReason);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"isDebugMode\":%s", s_deviceInfo.system.isDebugMode ? "true" : "false");
    offset += snprintf(json + offset, jsonSize - offset, "},"); // Close system
    
    // Processor information
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"processor\":{");
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"model\":\"%s\",", s_deviceInfo.processor.model);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"clockSpeedMHz\":%u,", s_deviceInfo.processor.clockSpeed / 1000000);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"coreCount\":%u,", s_deviceInfo.processor.coreCount);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"bitWidth\":%u,", s_deviceInfo.processor.bitWidth);
    
    if (s_deviceInfo.processor.temperatureC >= 0) {
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"temperatureC\":%.1f,", s_deviceInfo.processor.temperatureC);
    } else {
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"temperatureC\":null,");
    }
    
    if (s_deviceInfo.processor.loadPercent != 255) {
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"loadPercent\":%u", s_deviceInfo.processor.loadPercent);
    } else {
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"loadPercent\":null");
    }
    
    offset += snprintf(json + offset, jsonSize - offset, "},"); // Close processor
    
    // Memory information
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"memory\":{");
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"totalRamKB\":%u,", s_deviceInfo.memory.totalRam / 1024);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"freeRamKB\":%u,", s_deviceInfo.memory.freeRam / 1024);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"usedRamKB\":%u,", (s_deviceInfo.memory.totalRam - s_deviceInfo.memory.freeRam) / 1024);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"totalFlashKB\":%u,", s_deviceInfo.memory.totalFlash / 1024);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"freeFlashKB\":%u,", s_deviceInfo.memory.freeFlash / 1024);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"stackSizeKB\":%u,", s_deviceInfo.memory.stackSize / 1024);
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"heapSizeKB\":%u", s_deviceInfo.memory.heapSize / 1024);
    offset += snprintf(json + offset, jsonSize - offset, "},"); // Close memory
    
    // IO ports
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"ioPorts\":[");
    
    for (uint16_t i = 0; i < s_deviceInfo.ioPortCount; i++) {
        const MCP_IOPortInfo* port = &s_deviceInfo.ioPorts[i];
        
        offset += snprintf(json + offset, jsonSize - offset, "{");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"name\":\"%s\",", port->name);
        
        const char* typeStr = "unknown";
        switch (port->portType) {
            case 0: typeStr = "digital"; break;
            case 1: typeStr = "analog"; break;
            case 2: typeStr = "pwm"; break;
            case 3: typeStr = "serial"; break;
            case 4: typeStr = "i2c"; break;
            case 5: typeStr = "spi"; break;
            default: typeStr = "unknown"; break;
        }
        
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"type\":\"%s\",", typeStr);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"isInput\":%s,", port->isInput ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"isOutput\":%s,", port->isOutput ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"capabilities\":{");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"pullUp\":%s,", port->isPullUp ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"pullDown\":%s,", port->isPullDown ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"analog\":%s,", port->isAnalog ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"pwm\":%s,", port->isPWM ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"i2c\":%s,", port->isI2C ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"spi\":%s,", port->isSPI ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"uart\":%s,", port->isUART ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"interrupt\":%s", port->isInterrupt ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset, "},"); // Close capabilities
        
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"state\":{");
        
        if (port->portType == 0) { // Digital
            offset += snprintf(json + offset, jsonSize - offset,
                              "\"digital\":%s", port->currentState ? "true" : "false");
        } else if (port->portType == 1) { // Analog
            offset += snprintf(json + offset, jsonSize - offset,
                              "\"analog\":%u", port->analogValue);
        }
        
        offset += snprintf(json + offset, jsonSize - offset, "},"); // Close state
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"isReserved\":%s", port->isReserved ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset, "}");
        
        if (i < s_deviceInfo.ioPortCount - 1) {
            offset += snprintf(json + offset, jsonSize - offset, ",");
        }
    }
    
    offset += snprintf(json + offset, jsonSize - offset, "],"); // Close ioPorts
    
    // Network interfaces
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"networkInterfaces\":[");
    
    for (uint8_t i = 0; i < s_deviceInfo.networkInterfaceCount; i++) {
        const MCP_NetworkInfo* net = &s_deviceInfo.networkInterfaces[i];
        
        offset += snprintf(json + offset, jsonSize - offset, "{");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"name\":\"%s\",", net->name);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"macAddress\":\"%s\",", net->macAddress);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"ipAddress\":\"%s\",", net->ipAddress);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"isConnected\":%s,", net->isConnected ? "true" : "false");
        
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"type\":{");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"wifi\":%s,", net->isWifi ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"ethernet\":%s,", net->isEthernet ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"bluetooth\":%s", net->isBluetooth ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset, "},"); // Close type
        
        if (net->signalStrength != 255) {
            offset += snprintf(json + offset, jsonSize - offset,
                              "\"signalStrength\":%u", net->signalStrength);
        } else {
            offset += snprintf(json + offset, jsonSize - offset,
                              "\"signalStrength\":null");
        }
        
        offset += snprintf(json + offset, jsonSize - offset, "}");
        
        if (i < s_deviceInfo.networkInterfaceCount - 1) {
            offset += snprintf(json + offset, jsonSize - offset, ",");
        }
    }
    
    offset += snprintf(json + offset, jsonSize - offset, "],"); // Close networkInterfaces
    
    // Sensors
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"sensors\":[");
    
    for (uint8_t i = 0; i < s_deviceInfo.sensorCount; i++) {
        const MCP_SensorInfo* sensor = &s_deviceInfo.sensors[i];
        
        offset += snprintf(json + offset, jsonSize - offset, "{");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"name\":\"%s\",", sensor->name);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"type\":\"%s\",", sensor->type);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"units\":\"%s\",", sensor->units);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"range\":{");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"min\":%.2f,", sensor->minValue);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"max\":%.2f", sensor->maxValue);
        offset += snprintf(json + offset, jsonSize - offset, "},"); // Close range
        
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"currentValue\":%.2f,", sensor->currentValue);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"isConnected\":%s,", sensor->isConnected ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"bus\":{");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"type\":\"%s\",", sensor->busType);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"address\":%u", sensor->address);
        offset += snprintf(json + offset, jsonSize - offset, "}"); // Close bus
        
        offset += snprintf(json + offset, jsonSize - offset, "}");
        
        if (i < s_deviceInfo.sensorCount - 1) {
            offset += snprintf(json + offset, jsonSize - offset, ",");
        }
    }
    
    offset += snprintf(json + offset, jsonSize - offset, "],"); // Close sensors
    
    // Storage devices
    offset += snprintf(json + offset, jsonSize - offset,
                       "\"storageDevices\":[");
    
    for (uint8_t i = 0; i < s_deviceInfo.storageCount; i++) {
        const MCP_StorageInfo* storage = &s_deviceInfo.storageDevices[i];
        
        offset += snprintf(json + offset, jsonSize - offset, "{");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"name\":\"%s\",", storage->name);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"type\":\"%s\",", storage->type);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"totalSizeKB\":%u,", storage->totalSize / 1024);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"freeSpaceKB\":%u,", storage->freeSpace / 1024);
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"isWriteProtected\":%s,", storage->isWriteProtected ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"isRemovable\":%s,", storage->isRemovable ? "true" : "false");
        offset += snprintf(json + offset, jsonSize - offset,
                          "\"isPresent\":%s", storage->isPresent ? "true" : "false");
        
        offset += snprintf(json + offset, jsonSize - offset, "}");
        
        if (i < s_deviceInfo.storageCount - 1) {
            offset += snprintf(json + offset, jsonSize - offset, ",");
        }
    }
    
    offset += snprintf(json + offset, jsonSize - offset, "]"); // Close storageDevices
    
    // Add capabilities if available
    if (s_deviceInfo.capabilities != NULL) {
        offset += snprintf(json + offset, jsonSize - offset, ",");
        offset += snprintf(json + offset, jsonSize - offset, 
                           "\"capabilities\":%s", s_deviceInfo.capabilities);
    }
    
    // Close main object
    offset += snprintf(json + offset, jsonSize - offset, "}");
    
    return json;
}

/**
 * @brief Register IO port with device info system
 */
int MCP_DeviceInfoRegisterIOPort(const MCP_IOPortInfo* portInfo) {
    if (!s_initialized || portInfo == NULL) {
        return -1;
    }
    
    // Check if we have space
    if (s_deviceInfo.ioPortCount >= MAX_IO_PORTS) {
        return -2; // No space
    }
    
    // Copy port info
    memcpy(&s_deviceInfo.ioPorts[s_deviceInfo.ioPortCount], portInfo, sizeof(MCP_IOPortInfo));
    s_deviceInfo.ioPortCount++;
    
    return 0;
}

/**
 * @brief Register sensor with device info system
 */
int MCP_DeviceInfoRegisterSensor(const MCP_SensorInfo* sensorInfo) {
    if (!s_initialized || sensorInfo == NULL) {
        return -1;
    }
    
    // Check if we have space
    if (s_deviceInfo.sensorCount >= MAX_SENSORS) {
        return -2; // No space
    }
    
    // Copy sensor info
    memcpy(&s_deviceInfo.sensors[s_deviceInfo.sensorCount], sensorInfo, sizeof(MCP_SensorInfo));
    s_deviceInfo.sensorCount++;
    
    return 0;
}

/**
 * @brief Register storage device with device info system
 */
int MCP_DeviceInfoRegisterStorage(const MCP_StorageInfo* storageInfo) {
    if (!s_initialized || storageInfo == NULL) {
        return -1;
    }
    
    // Check if we have space
    if (s_deviceInfo.storageCount >= MAX_STORAGE_DEVICES) {
        return -2; // No space
    }
    
    // Copy storage info
    memcpy(&s_deviceInfo.storageDevices[s_deviceInfo.storageCount], storageInfo, sizeof(MCP_StorageInfo));
    s_deviceInfo.storageCount++;
    
    return 0;
}

/**
 * @brief Register network interface with device info system
 */
int MCP_DeviceInfoRegisterNetwork(const MCP_NetworkInfo* networkInfo) {
    if (!s_initialized || networkInfo == NULL) {
        return -1;
    }
    
    // Check if we have space
    if (s_deviceInfo.networkInterfaceCount >= MAX_NETWORK_INTERFACES) {
        return -2; // No space
    }
    
    // Copy network info
    memcpy(&s_deviceInfo.networkInterfaces[s_deviceInfo.networkInterfaceCount], 
           networkInfo, sizeof(MCP_NetworkInfo));
    s_deviceInfo.networkInterfaceCount++;
    
    return 0;
}

/**
 * @brief Set system capabilities JSON
 */
int MCP_DeviceInfoSetCapabilities(const char* capabilities) {
    if (!s_initialized || capabilities == NULL) {
        return -1;
    }
    
    // Free existing capabilities if any
    if (s_deviceInfo.capabilities != NULL) {
        free(s_deviceInfo.capabilities);
        s_deviceInfo.capabilities = NULL;
    }
    
    // Copy capabilities
    s_deviceInfo.capabilities = strdup(capabilities);
    if (s_deviceInfo.capabilities == NULL) {
        return -2; // Memory allocation failed
    }
    
    return 0;
}

/**
 * @brief Tool handler for system.getDeviceInfo
 */
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
    MCP_ToolResult result = {0};
    result.status = 0; // Success
    result.resultJson = deviceInfoJson;
    
    return result;
}

// ===== Platform-specific implementations =====

/**
 * @brief Populate system information
 */
static void populateSystemInfo(void) {
    // Set device name
#ifdef ESP32
    strncpy(s_deviceInfo.system.deviceName, "ESP32 Device", sizeof(s_deviceInfo.system.deviceName) - 1);
#elif defined(ARDUINO)
    strncpy(s_deviceInfo.system.deviceName, "Arduino Device", sizeof(s_deviceInfo.system.deviceName) - 1);
#elif defined(MBED)
    strncpy(s_deviceInfo.system.deviceName, "Mbed Device", sizeof(s_deviceInfo.system.deviceName) - 1);
#else
    strncpy(s_deviceInfo.system.deviceName, "MCP Embedded Device", sizeof(s_deviceInfo.system.deviceName) - 1);
#endif

    // Set firmware version
    strncpy(s_deviceInfo.system.firmwareVersion, "1.0.0", sizeof(s_deviceInfo.system.firmwareVersion) - 1);

    // Set build date and time
    strncpy(s_deviceInfo.system.buildDate, __DATE__, sizeof(s_deviceInfo.system.buildDate) - 1);
    strncpy(s_deviceInfo.system.buildTime, __TIME__, sizeof(s_deviceInfo.system.buildTime) - 1);

    // Set platform name
#ifdef ESP32
    strncpy(s_deviceInfo.system.platformName, "ESP32", sizeof(s_deviceInfo.system.platformName) - 1);
#elif defined(ARDUINO)
    strncpy(s_deviceInfo.system.platformName, "Arduino", sizeof(s_deviceInfo.system.platformName) - 1);
#elif defined(MBED)
    strncpy(s_deviceInfo.system.platformName, "Mbed", sizeof(s_deviceInfo.system.platformName) - 1);
#elif defined(RPI_MODEL_1) || defined(RPI_MODEL_2) || defined(RPI_MODEL_3) || defined(RPI_MODEL_4) || defined(RPI_MODEL_400) || defined(RPI_MODEL_5)
    strncpy(s_deviceInfo.system.platformName, "Raspberry Pi", sizeof(s_deviceInfo.system.platformName) - 1);
#else
    strncpy(s_deviceInfo.system.platformName, "Generic", sizeof(s_deviceInfo.system.platformName) - 1);
#endif

    // Set board model
#ifdef ESP32
    strncpy(s_deviceInfo.system.boardModel, "ESP32 Dev Board", sizeof(s_deviceInfo.system.boardModel) - 1);
#elif defined(ARDUINO)
    #ifdef ARDUINO_BOARD
    strncpy(s_deviceInfo.system.boardModel, ARDUINO_BOARD, sizeof(s_deviceInfo.system.boardModel) - 1);
    #else
    strncpy(s_deviceInfo.system.boardModel, "Arduino Board", sizeof(s_deviceInfo.system.boardModel) - 1);
    #endif
#elif defined(MBED)
    strncpy(s_deviceInfo.system.boardModel, "Mbed Board", sizeof(s_deviceInfo.system.boardModel) - 1);
#elif defined(RPI_MODEL_1)
    strncpy(s_deviceInfo.system.boardModel, "Raspberry Pi 1", sizeof(s_deviceInfo.system.boardModel) - 1);
#elif defined(RPI_MODEL_2)
    strncpy(s_deviceInfo.system.boardModel, "Raspberry Pi 2", sizeof(s_deviceInfo.system.boardModel) - 1);
#elif defined(RPI_MODEL_3)
    strncpy(s_deviceInfo.system.boardModel, "Raspberry Pi 3", sizeof(s_deviceInfo.system.boardModel) - 1);
#elif defined(RPI_MODEL_4)
    strncpy(s_deviceInfo.system.boardModel, "Raspberry Pi 4", sizeof(s_deviceInfo.system.boardModel) - 1);
#elif defined(RPI_MODEL_400)
    strncpy(s_deviceInfo.system.boardModel, "Raspberry Pi 400", sizeof(s_deviceInfo.system.boardModel) - 1);
#elif defined(RPI_MODEL_5)
    strncpy(s_deviceInfo.system.boardModel, "Raspberry Pi 5", sizeof(s_deviceInfo.system.boardModel) - 1);
#else
    strncpy(s_deviceInfo.system.boardModel, "Generic Board", sizeof(s_deviceInfo.system.boardModel) - 1);
#endif

    // Set reset count (initial value)
    s_deviceInfo.system.resetCount = 1;

    // Set last reset reason
#ifdef ESP32
    esp_reset_reason_t reset_reason = esp_reset_reason();
    switch (reset_reason) {
        case ESP_RST_POWERON: 
            strncpy(s_deviceInfo.system.lastResetReason, "Power-on reset", sizeof(s_deviceInfo.system.lastResetReason) - 1);
            break;
        case ESP_RST_SW:
            strncpy(s_deviceInfo.system.lastResetReason, "Software reset", sizeof(s_deviceInfo.system.lastResetReason) - 1);
            break;
        case ESP_RST_PANIC:
            strncpy(s_deviceInfo.system.lastResetReason, "Exception/panic", sizeof(s_deviceInfo.system.lastResetReason) - 1);
            break;
        case ESP_RST_WDT:
            strncpy(s_deviceInfo.system.lastResetReason, "Watchdog reset", sizeof(s_deviceInfo.system.lastResetReason) - 1);
            break;
        default:
            strncpy(s_deviceInfo.system.lastResetReason, "Unknown", sizeof(s_deviceInfo.system.lastResetReason) - 1);
            break;
    }
#else
    strncpy(s_deviceInfo.system.lastResetReason, "Power-on", sizeof(s_deviceInfo.system.lastResetReason) - 1);
#endif

    // Set debug mode
#ifdef DEBUG
    s_deviceInfo.system.isDebugMode = true;
#else
    s_deviceInfo.system.isDebugMode = false;
#endif
}

/**
 * @brief Populate processor information
 */
static void populateProcessorInfo(void) {
#ifdef ESP32
    // Get ESP32 chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    // Set processor model
    switch (chip_info.model) {
        case CHIP_ESP32:
            strncpy(s_deviceInfo.processor.model, "ESP32", sizeof(s_deviceInfo.processor.model) - 1);
            break;
        case CHIP_ESP32S2:
            strncpy(s_deviceInfo.processor.model, "ESP32-S2", sizeof(s_deviceInfo.processor.model) - 1);
            break;
        case CHIP_ESP32S3:
            strncpy(s_deviceInfo.processor.model, "ESP32-S3", sizeof(s_deviceInfo.processor.model) - 1);
            break;
        case CHIP_ESP32C3:
            strncpy(s_deviceInfo.processor.model, "ESP32-C3", sizeof(s_deviceInfo.processor.model) - 1);
            break;
        default:
            strncpy(s_deviceInfo.processor.model, "ESP32 Family", sizeof(s_deviceInfo.processor.model) - 1);
            break;
    }
    
    // Set clock speed
    s_deviceInfo.processor.clockSpeed = 240 * 1000 * 1000; // 240 MHz
    
    // Set core count
    s_deviceInfo.processor.coreCount = chip_info.cores;
    
    // Set bit width
    s_deviceInfo.processor.bitWidth = 32;
    
    // Set temperature (not available on all ESP32)
    s_deviceInfo.processor.temperatureC = -1; // Not available
    
    // Set CPU load (not easily available on ESP32)
    s_deviceInfo.processor.loadPercent = 255; // Not available
    
#elif defined(ARDUINO)
    // Arduino processor info
    #if defined(ARDUINO_ARCH_AVR)
        strncpy(s_deviceInfo.processor.model, "AVR", sizeof(s_deviceInfo.processor.model) - 1);
        s_deviceInfo.processor.bitWidth = 8;
    #elif defined(ARDUINO_ARCH_SAM)
        strncpy(s_deviceInfo.processor.model, "ARM SAM", sizeof(s_deviceInfo.processor.model) - 1);
        s_deviceInfo.processor.bitWidth = 32;
    #elif defined(ARDUINO_ARCH_SAMD)
        strncpy(s_deviceInfo.processor.model, "ARM SAMD", sizeof(s_deviceInfo.processor.model) - 1);
        s_deviceInfo.processor.bitWidth = 32;
    #elif defined(ARDUINO_ARCH_STM32)
        strncpy(s_deviceInfo.processor.model, "STM32", sizeof(s_deviceInfo.processor.model) - 1);
        s_deviceInfo.processor.bitWidth = 32;
    #else
        strncpy(s_deviceInfo.processor.model, "Unknown Arduino", sizeof(s_deviceInfo.processor.model) - 1);
        s_deviceInfo.processor.bitWidth = 8;
    #endif
    
    // Clock speed (varies by board)
    #ifdef F_CPU
    s_deviceInfo.processor.clockSpeed = F_CPU;
    #else
    s_deviceInfo.processor.clockSpeed = 16 * 1000 * 1000; // Assume 16 MHz
    #endif
    
    // Core count (most Arduinos are single core)
    s_deviceInfo.processor.coreCount = 1;
    
    // Temp and load not available
    s_deviceInfo.processor.temperatureC = -1;
    s_deviceInfo.processor.loadPercent = 255;
    
#elif defined(MBED)
    // Mbed processor info
    strncpy(s_deviceInfo.processor.model, "ARM Cortex-M", sizeof(s_deviceInfo.processor.model) - 1);
    s_deviceInfo.processor.clockSpeed = 120 * 1000 * 1000; // Assume 120 MHz
    s_deviceInfo.processor.coreCount = 1;
    s_deviceInfo.processor.bitWidth = 32;
    s_deviceInfo.processor.temperatureC = -1;
    s_deviceInfo.processor.loadPercent = 255;

#elif defined(RPI_MODEL_1)
    // Raspberry Pi 1 (ARM1176JZF-S)
    strncpy(s_deviceInfo.processor.model, "ARM1176JZF-S", sizeof(s_deviceInfo.processor.model) - 1);
    s_deviceInfo.processor.clockSpeed = 700 * 1000 * 1000; // 700 MHz
    s_deviceInfo.processor.coreCount = 1;
    s_deviceInfo.processor.bitWidth = 32;
    s_deviceInfo.processor.temperatureC = -1; // Would come from HAL in real implementation
    s_deviceInfo.processor.loadPercent = 255; // Not available

#elif defined(RPI_MODEL_2)
    // Raspberry Pi 2 (Cortex-A7)
    strncpy(s_deviceInfo.processor.model, "ARM Cortex-A7", sizeof(s_deviceInfo.processor.model) - 1);
    s_deviceInfo.processor.clockSpeed = 900 * 1000 * 1000; // 900 MHz
    s_deviceInfo.processor.coreCount = 4;
    s_deviceInfo.processor.bitWidth = 32;
    s_deviceInfo.processor.temperatureC = -1; // Would come from HAL in real implementation
    s_deviceInfo.processor.loadPercent = 255; // Not available

#elif defined(RPI_MODEL_3)
    // Raspberry Pi 3 (Cortex-A53)
    strncpy(s_deviceInfo.processor.model, "ARM Cortex-A53", sizeof(s_deviceInfo.processor.model) - 1);
    s_deviceInfo.processor.clockSpeed = 1200 * 1000 * 1000; // 1.2 GHz
    s_deviceInfo.processor.coreCount = 4;
    s_deviceInfo.processor.bitWidth = 64;
    s_deviceInfo.processor.temperatureC = -1; // Would come from HAL in real implementation
    s_deviceInfo.processor.loadPercent = 255; // Not available

#elif defined(RPI_MODEL_4) || defined(RPI_MODEL_400)
    // Raspberry Pi 4 (Cortex-A72)
    strncpy(s_deviceInfo.processor.model, "ARM Cortex-A72", sizeof(s_deviceInfo.processor.model) - 1);
    s_deviceInfo.processor.clockSpeed = 1500 * 1000 * 1000; // 1.5 GHz
    s_deviceInfo.processor.coreCount = 4;
    s_deviceInfo.processor.bitWidth = 64;
    s_deviceInfo.processor.temperatureC = -1; // Would come from HAL in real implementation
    s_deviceInfo.processor.loadPercent = 255; // Not available

#elif defined(RPI_MODEL_5)
    // Raspberry Pi 5 (Cortex-A76)
    strncpy(s_deviceInfo.processor.model, "ARM Cortex-A76", sizeof(s_deviceInfo.processor.model) - 1);
    s_deviceInfo.processor.clockSpeed = 2400 * 1000 * 1000; // 2.4 GHz
    s_deviceInfo.processor.coreCount = 4;
    s_deviceInfo.processor.bitWidth = 64;
    s_deviceInfo.processor.temperatureC = -1; // Would come from HAL in real implementation
    s_deviceInfo.processor.loadPercent = 255; // Not available

#else
    // Generic processor info
    strncpy(s_deviceInfo.processor.model, "Generic CPU", sizeof(s_deviceInfo.processor.model) - 1);
    s_deviceInfo.processor.clockSpeed = 100 * 1000 * 1000; // Assume 100 MHz
    s_deviceInfo.processor.coreCount = 1;
    s_deviceInfo.processor.bitWidth = 32;
    s_deviceInfo.processor.temperatureC = -1;
    s_deviceInfo.processor.loadPercent = 255;
#endif
}

/**
 * @brief Populate memory information
 */
static void populateMemoryInfo(void) {
#ifdef ESP32
    // ESP32 memory info
    s_deviceInfo.memory.totalRam = esp_heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    s_deviceInfo.memory.freeRam = esp_heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    
    // Get flash size
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    s_deviceInfo.memory.totalFlash = flash_size;
    s_deviceInfo.memory.freeFlash = 0; // Not easily available
    
    // Stack and heap
    s_deviceInfo.memory.stackSize = 8 * 1024; // Typical ESP32 stack size
    s_deviceInfo.memory.heapSize = s_deviceInfo.memory.totalRam; // Approximate

#elif defined(ARDUINO)
    // Arduino memory info (varies by board)
    #if defined(ARDUINO_ARCH_AVR)
        s_deviceInfo.memory.totalRam = 2 * 1024; // Typical ATmega328 (Uno)
        s_deviceInfo.memory.freeRam = freeMemory(); // Requires MemoryFree library
        s_deviceInfo.memory.totalFlash = 32 * 1024; // Typical ATmega328
    #elif defined(ARDUINO_ARCH_SAM)
        s_deviceInfo.memory.totalRam = 96 * 1024; // Typical SAM3X8E (Due)
        s_deviceInfo.memory.freeRam = freeMemory();
        s_deviceInfo.memory.totalFlash = 512 * 1024; // Typical SAM3X8E
    #elif defined(ARDUINO_ARCH_SAMD)
        s_deviceInfo.memory.totalRam = 32 * 1024; // Typical SAMD21 (Zero)
        s_deviceInfo.memory.freeRam = freeMemory();
        s_deviceInfo.memory.totalFlash = 256 * 1024; // Typical SAMD21
    #else
        s_deviceInfo.memory.totalRam = 8 * 1024; // Generic estimate
        s_deviceInfo.memory.freeRam = 4 * 1024; // Generic estimate
        s_deviceInfo.memory.totalFlash = 64 * 1024; // Generic estimate
    #endif
    
    // Free flash not easily available
    s_deviceInfo.memory.freeFlash = 0;
    
    // Stack and heap
    s_deviceInfo.memory.stackSize = 2 * 1024; // Generic estimate
    s_deviceInfo.memory.heapSize = s_deviceInfo.memory.totalRam / 2; // Generic estimate

#elif defined(MBED)
    // Mbed memory info
    s_deviceInfo.memory.totalRam = 128 * 1024; // Generic estimate
    s_deviceInfo.memory.freeRam = 64 * 1024; // Generic estimate
    s_deviceInfo.memory.totalFlash = 512 * 1024; // Generic estimate
    s_deviceInfo.memory.freeFlash = 0; // Not easily available
    s_deviceInfo.memory.stackSize = 4 * 1024; // Generic estimate
    s_deviceInfo.memory.heapSize = 64 * 1024; // Generic estimate

#elif defined(RPI_MODEL_1)
    // Raspberry Pi 1 memory info
    s_deviceInfo.memory.totalRam = 512 * 1024 * 1024; // 512 MB
    s_deviceInfo.memory.freeRam = 256 * 1024 * 1024; // Example
    s_deviceInfo.memory.totalFlash = 0; // Uses SD card, no built-in flash
    s_deviceInfo.memory.freeFlash = 0;
    s_deviceInfo.memory.stackSize = 8 * 1024 * 1024; // 8 MB estimate
    s_deviceInfo.memory.heapSize = 256 * 1024 * 1024; // 256 MB estimate

#elif defined(RPI_MODEL_2)
    // Raspberry Pi 2 memory info
    s_deviceInfo.memory.totalRam = 1024 * 1024 * 1024; // 1 GB
    s_deviceInfo.memory.freeRam = 512 * 1024 * 1024; // Example
    s_deviceInfo.memory.totalFlash = 0; // Uses SD card, no built-in flash
    s_deviceInfo.memory.freeFlash = 0;
    s_deviceInfo.memory.stackSize = 8 * 1024 * 1024; // 8 MB estimate
    s_deviceInfo.memory.heapSize = 512 * 1024 * 1024; // 512 MB estimate

#elif defined(RPI_MODEL_3)
    // Raspberry Pi 3 memory info
    s_deviceInfo.memory.totalRam = 1024 * 1024 * 1024; // 1 GB
    s_deviceInfo.memory.freeRam = 512 * 1024 * 1024; // Example
    s_deviceInfo.memory.totalFlash = 0; // Uses SD card, no built-in flash
    s_deviceInfo.memory.freeFlash = 0;
    s_deviceInfo.memory.stackSize = 8 * 1024 * 1024; // 8 MB estimate
    s_deviceInfo.memory.heapSize = 512 * 1024 * 1024; // 512 MB estimate

#elif defined(RPI_MODEL_4) || defined(RPI_MODEL_400)
    // Raspberry Pi 4 memory info (varies: 2GB, 4GB, 8GB models)
    s_deviceInfo.memory.totalRam = 4 * 1024 * 1024 * 1024ULL; // 4 GB (example)
    s_deviceInfo.memory.freeRam = 2 * 1024 * 1024 * 1024ULL; // Example
    s_deviceInfo.memory.totalFlash = 0; // Uses SD card, no built-in flash
    s_deviceInfo.memory.freeFlash = 0;
    s_deviceInfo.memory.stackSize = 16 * 1024 * 1024; // 16 MB estimate
    s_deviceInfo.memory.heapSize = 2 * 1024 * 1024 * 1024ULL; // 2 GB estimate

#elif defined(RPI_MODEL_5)
    // Raspberry Pi 5 memory info (4GB or 8GB models)
    s_deviceInfo.memory.totalRam = 8 * 1024 * 1024 * 1024ULL; // 8 GB (example)
    s_deviceInfo.memory.freeRam = 4 * 1024 * 1024 * 1024ULL; // Example
    s_deviceInfo.memory.totalFlash = 0; // Uses SD card, no built-in flash
    s_deviceInfo.memory.freeFlash = 0;
    s_deviceInfo.memory.stackSize = 16 * 1024 * 1024; // 16 MB estimate
    s_deviceInfo.memory.heapSize = 4 * 1024 * 1024 * 1024ULL; // 4 GB estimate

#else
    // Generic memory info
    s_deviceInfo.memory.totalRam = 256 * 1024;
    s_deviceInfo.memory.freeRam = 128 * 1024;
    s_deviceInfo.memory.totalFlash = 1024 * 1024;
    s_deviceInfo.memory.freeFlash = 512 * 1024;
    s_deviceInfo.memory.stackSize = 8 * 1024;
    s_deviceInfo.memory.heapSize = 128 * 1024;
#endif
}

/**
 * @brief Populate IO port information
 */
static void populateIOPortInfo(void) {
    // Skip if ports are already registered
    if (s_deviceInfo.ioPortCount > 0) {
        return;
    }

#ifdef ESP32
    // ESP32 has 40 GPIO pins (0-39, but not all are available)
    // Register common GPIO pins
    for (int i = 0; i < 40; i++) {
        // Skip pins that are usually reserved or unavailable
        if (i == 6 || i == 7 || i == 8 || i == 9 || i == 10 || i == 11 || 
            (i >= 20 && i <= 24) || i > 39) {
            continue;
        }
        
        MCP_IOPortInfo portInfo = {0};
        snprintf(portInfo.name, sizeof(portInfo.name), "GPIO%d", i);
        portInfo.portType = 0; // Digital
        
        // All ESP32 pins can be inputs
        portInfo.isInput = true;
        
        // All ESP32 pins except input-only can be outputs
        if (i < 34) {
            portInfo.isOutput = true;
        }
        
        // Capabilities
        portInfo.isPullUp = (i < 34); // Input-only pins don't have pull-up
        portInfo.isPullDown = (i < 34); // Input-only pins don't have pull-down
        portInfo.isAnalog = (i >= 32 && i <= 39); // ADC pins
        portInfo.isPWM = (i < 34); // All GPIO except input-only can do PWM
        portInfo.isI2C = (i < 34); // Any GPIO can be I2C
        portInfo.isSPI = (i < 34); // Any GPIO can be SPI
        portInfo.isUART = (i < 34); // Any GPIO can be UART
        portInfo.isInterrupt = true; // All pins can generate interrupts
        
        // Current state (would read from GPIO)
        portInfo.currentState = 0;
        portInfo.analogValue = 0;
        
        // Reserved pins
        if (i == 0 || i == 1 || i == 2 || i == 3) {
            portInfo.isReserved = true; // Boot or UART pins
        }
        
        // Register port
        MCP_DeviceInfoRegisterIOPort(&portInfo);
    }
#elif defined(ARDUINO)
    // Arduino pins depend on board
    // Simplified example for Uno-like boards
    for (int i = 0; i < 20; i++) {
        MCP_IOPortInfo portInfo = {0};
        
        if (i < 14) {
            // Digital pins
            snprintf(portInfo.name, sizeof(portInfo.name), "D%d", i);
            portInfo.portType = 0; // Digital
            portInfo.isInput = true;
            portInfo.isOutput = true;
            portInfo.isPullUp = true;
            portInfo.isPullDown = false;
            portInfo.isAnalog = false;
            portInfo.isPWM = (i == 3 || i == 5 || i == 6 || i == 9 || i == 10 || i == 11);
            portInfo.isI2C = (i == 18 || i == 19); // A4/A5 on Uno
            portInfo.isSPI = (i == 10 || i == 11 || i == 12 || i == 13);
            portInfo.isUART = (i == 0 || i == 1);
            portInfo.isInterrupt = (i == 2 || i == 3);
            portInfo.isReserved = (i == 0 || i == 1); // UART pins
        } else {
            // Analog pins (A0-A5)
            snprintf(portInfo.name, sizeof(portInfo.name), "A%d", i - 14);
            portInfo.portType = 1; // Analog
            portInfo.isInput = true;
            portInfo.isOutput = true;
            portInfo.isPullUp = true;
            portInfo.isPullDown = false;
            portInfo.isAnalog = true;
            portInfo.isPWM = false;
            portInfo.isI2C = (i == 18 || i == 19); // A4/A5 on Uno
            portInfo.isSPI = false;
            portInfo.isUART = false;
            portInfo.isInterrupt = false;
            portInfo.isReserved = false;
        }
        
        // Register port
        MCP_DeviceInfoRegisterIOPort(&portInfo);
    }
#elif defined(MBED)
    // Mbed pins vary by board
    // Generic example for typical mbed board
    const char* pinNames[] = {
        "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10", "D11", "D12", "D13", "D14", "D15",
        "A0", "A1", "A2", "A3", "A4", "A5"
    };
    
    for (int i = 0; i < sizeof(pinNames) / sizeof(pinNames[0]); i++) {
        MCP_IOPortInfo portInfo = {0};
        strncpy(portInfo.name, pinNames[i], sizeof(portInfo.name) - 1);
        
        if (pinNames[i][0] == 'D') {
            // Digital pins
            portInfo.portType = 0; // Digital
            portInfo.isInput = true;
            portInfo.isOutput = true;
            portInfo.isPullUp = true;
            portInfo.isPullDown = true;
            portInfo.isAnalog = false;
            portInfo.isPWM = (i == 3 || i == 5 || i == 6 || i == 9);
            portInfo.isI2C = (i == 14 || i == 15);
            portInfo.isSPI = (i == 10 || i == 11 || i == 12 || i == 13);
            portInfo.isUART = (i == 0 || i == 1);
            portInfo.isInterrupt = true;
            portInfo.isReserved = false;
        } else {
            // Analog pins
            portInfo.portType = 1; // Analog
            portInfo.isInput = true;
            portInfo.isOutput = false;
            portInfo.isPullUp = false;
            portInfo.isPullDown = false;
            portInfo.isAnalog = true;
            portInfo.isPWM = false;
            portInfo.isI2C = false;
            portInfo.isSPI = false;
            portInfo.isUART = false;
            portInfo.isInterrupt = false;
            portInfo.isReserved = false;
        }
        
        // Register port
        MCP_DeviceInfoRegisterIOPort(&portInfo);
    }
#else
    // Generic 
    MCP_IOPortInfo portInfo = {0};
    
    // Add a few sample pins for testing
    strncpy(portInfo.name, "D0", sizeof(portInfo.name) - 1);
    portInfo.portType = 0; // Digital
    portInfo.isInput = true;
    portInfo.isOutput = true;
    portInfo.isPullUp = true;
    portInfo.isPullDown = true;
    portInfo.isAnalog = false;
    portInfo.isPWM = false;
    portInfo.isI2C = false;
    portInfo.isSPI = false;
    portInfo.isUART = true;
    portInfo.isInterrupt = false;
    portInfo.isReserved = true; // Reserved for UART
    MCP_DeviceInfoRegisterIOPort(&portInfo);
    
    strncpy(portInfo.name, "D1", sizeof(portInfo.name) - 1);
    portInfo.portType = 0; // Digital
    portInfo.isInput = true;
    portInfo.isOutput = true;
    portInfo.isPullUp = true;
    portInfo.isPullDown = true;
    portInfo.isAnalog = false;
    portInfo.isPWM = false;
    portInfo.isI2C = false;
    portInfo.isSPI = false;
    portInfo.isUART = true;
    portInfo.isInterrupt = false;
    portInfo.isReserved = true; // Reserved for UART
    MCP_DeviceInfoRegisterIOPort(&portInfo);
    
    strncpy(portInfo.name, "A0", sizeof(portInfo.name) - 1);
    portInfo.portType = 1; // Analog
    portInfo.isInput = true;
    portInfo.isOutput = false;
    portInfo.isPullUp = false;
    portInfo.isPullDown = false;
    portInfo.isAnalog = true;
    portInfo.isPWM = false;
    portInfo.isI2C = false;
    portInfo.isSPI = false;
    portInfo.isUART = false;
    portInfo.isInterrupt = false;
    portInfo.isReserved = false;
    MCP_DeviceInfoRegisterIOPort(&portInfo);
#endif
}

/**
 * @brief Populate network information
 */
static void populateNetworkInfo(void) {
    // Skip if interfaces are already registered
    if (s_deviceInfo.networkInterfaceCount > 0) {
        return;
    }

#ifdef ESP32
    // ESP32 WiFi
    MCP_NetworkInfo wifiInfo = {0};
    strncpy(wifiInfo.name, "WiFi", sizeof(wifiInfo.name) - 1);
    
    // Get MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(wifiInfo.macAddress, sizeof(wifiInfo.macAddress), 
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Get IP address (would depend on connection state)
    strncpy(wifiInfo.ipAddress, "192.168.1.100", sizeof(wifiInfo.ipAddress) - 1);
    
    wifiInfo.isConnected = true; // Would check actual connection state
    wifiInfo.isWifi = true;
    wifiInfo.isEthernet = false;
    wifiInfo.isBluetooth = false;
    
    // Get signal strength (would depend on connection state)
    wifiInfo.signalStrength = 80; // Example value
    
    // Register WiFi interface
    MCP_DeviceInfoRegisterNetwork(&wifiInfo);
    
    // ESP32 also has Bluetooth
    MCP_NetworkInfo btInfo = {0};
    strncpy(btInfo.name, "Bluetooth", sizeof(btInfo.name) - 1);
    strncpy(btInfo.macAddress, wifiInfo.macAddress, sizeof(btInfo.macAddress) - 1); // Often shares MAC with WiFi
    strncpy(btInfo.ipAddress, "", sizeof(btInfo.ipAddress) - 1); // No IP for Bluetooth
    
    btInfo.isConnected = false; // Would check actual connection state
    btInfo.isWifi = false;
    btInfo.isEthernet = false;
    btInfo.isBluetooth = true;
    btInfo.signalStrength = 255; // Not applicable
    
    // Register Bluetooth interface
    MCP_DeviceInfoRegisterNetwork(&btInfo);
#elif defined(ARDUINO)
    // Arduino network capabilities vary by board and shields
    // Example for Ethernet shield
    MCP_NetworkInfo ethInfo = {0};
    strncpy(ethInfo.name, "Ethernet", sizeof(ethInfo.name) - 1);
    strncpy(ethInfo.macAddress, "DE:AD:BE:EF:FE:ED", sizeof(ethInfo.macAddress) - 1); // Example MAC
    strncpy(ethInfo.ipAddress, "192.168.1.177", sizeof(ethInfo.ipAddress) - 1); // Example IP
    
    ethInfo.isConnected = true; // Would check actual connection state
    ethInfo.isWifi = false;
    ethInfo.isEthernet = true;
    ethInfo.isBluetooth = false;
    ethInfo.signalStrength = 255; // Not applicable
    
    // Register Ethernet interface
    MCP_DeviceInfoRegisterNetwork(&ethInfo);
#elif defined(MBED)
    // Mbed network capabilities vary by board
    // Example for Ethernet
    MCP_NetworkInfo ethInfo = {0};
    strncpy(ethInfo.name, "Ethernet", sizeof(ethInfo.name) - 1);
    strncpy(ethInfo.macAddress, "00:11:22:33:44:55", sizeof(ethInfo.macAddress) - 1); // Example MAC
    strncpy(ethInfo.ipAddress, "192.168.1.200", sizeof(ethInfo.ipAddress) - 1); // Example IP
    
    ethInfo.isConnected = true; // Would check actual connection state
    ethInfo.isWifi = false;
    ethInfo.isEthernet = true;
    ethInfo.isBluetooth = false;
    ethInfo.signalStrength = 255; // Not applicable
    
    // Register Ethernet interface
    MCP_DeviceInfoRegisterNetwork(&ethInfo);
#else
    // Generic
    MCP_NetworkInfo wifiInfo = {0};
    strncpy(wifiInfo.name, "WiFi", sizeof(wifiInfo.name) - 1);
    strncpy(wifiInfo.macAddress, "00:11:22:33:44:55", sizeof(wifiInfo.macAddress) - 1);
    strncpy(wifiInfo.ipAddress, "192.168.1.100", sizeof(wifiInfo.ipAddress) - 1);
    wifiInfo.isConnected = true;
    wifiInfo.isWifi = true;
    wifiInfo.isEthernet = false;
    wifiInfo.isBluetooth = false;
    wifiInfo.signalStrength = 75;
    
    // Register WiFi interface
    MCP_DeviceInfoRegisterNetwork(&wifiInfo);
#endif
}

/**
 * @brief Populate sensor information
 */
static void populateSensorInfo(void) {
    // Skip if sensors are already registered
    if (s_deviceInfo.sensorCount > 0) {
        return;
    }

    // Example sensors - these would be populated based on actual connected hardware
    MCP_SensorInfo tempSensor = {0};
    strncpy(tempSensor.name, "Internal Temperature", sizeof(tempSensor.name) - 1);
    strncpy(tempSensor.type, "temperature", sizeof(tempSensor.type) - 1);
    strncpy(tempSensor.units, "°C", sizeof(tempSensor.units) - 1);
    tempSensor.minValue = -40.0f;
    tempSensor.maxValue = 85.0f;
    tempSensor.currentValue = 23.5f; // Example value
    tempSensor.isConnected = true;
    strncpy(tempSensor.busType, "i2c", sizeof(tempSensor.busType) - 1);
    tempSensor.address = 0x48; // Example I2C address
    
    // Register temperature sensor
    MCP_DeviceInfoRegisterSensor(&tempSensor);
    
    // Humidity sensor
    MCP_SensorInfo humiditySensor = {0};
    strncpy(humiditySensor.name, "Humidity Sensor", sizeof(humiditySensor.name) - 1);
    strncpy(humiditySensor.type, "humidity", sizeof(humiditySensor.type) - 1);
    strncpy(humiditySensor.units, "%RH", sizeof(humiditySensor.units) - 1);
    humiditySensor.minValue = 0.0f;
    humiditySensor.maxValue = 100.0f;
    humiditySensor.currentValue = 42.8f; // Example value
    humiditySensor.isConnected = true;
    strncpy(humiditySensor.busType, "i2c", sizeof(humiditySensor.busType) - 1);
    humiditySensor.address = 0x40; // Example I2C address
    
    // Register humidity sensor
    MCP_DeviceInfoRegisterSensor(&humiditySensor);
    
    // Light sensor
    MCP_SensorInfo lightSensor = {0};
    strncpy(lightSensor.name, "Light Sensor", sizeof(lightSensor.name) - 1);
    strncpy(lightSensor.type, "light", sizeof(lightSensor.type) - 1);
    strncpy(lightSensor.units, "lux", sizeof(lightSensor.units) - 1);
    lightSensor.minValue = 0.0f;
    lightSensor.maxValue = 65535.0f;
    lightSensor.currentValue = 320.0f; // Example value
    lightSensor.isConnected = true;
    strncpy(lightSensor.busType, "analog", sizeof(lightSensor.busType) - 1);
    lightSensor.address = 0; // Not applicable
    
    // Register light sensor
    MCP_DeviceInfoRegisterSensor(&lightSensor);
}

/**
 * @brief Populate storage information
 */
static void populateStorageInfo(void) {
    // Skip if storage devices are already registered
    if (s_deviceInfo.storageCount > 0) {
        return;
    }

#ifdef ESP32
    // ESP32 internal flash
    MCP_StorageInfo flashInfo = {0};
    strncpy(flashInfo.name, "Internal Flash", sizeof(flashInfo.name) - 1);
    strncpy(flashInfo.type, "flash", sizeof(flashInfo.type) - 1);
    
    // Get flash size
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    flashInfo.totalSize = flash_size;
    flashInfo.freeSpace = flash_size / 2; // Approximation
    
    flashInfo.isWriteProtected = false;
    flashInfo.isRemovable = false;
    flashInfo.isPresent = true;
    
    // Register flash storage
    MCP_DeviceInfoRegisterStorage(&flashInfo);
    
    // SD Card (if present)
    MCP_StorageInfo sdInfo = {0};
    strncpy(sdInfo.name, "SD Card", sizeof(sdInfo.name) - 1);
    strncpy(sdInfo.type, "sd", sizeof(sdInfo.type) - 1);
    sdInfo.totalSize = 8 * 1024 * 1024 * 1024ULL; // 8 GB example
    sdInfo.freeSpace = 6 * 1024 * 1024 * 1024ULL; // 6 GB example
    sdInfo.isWriteProtected = false;
    sdInfo.isRemovable = true;
    sdInfo.isPresent = true; // Would check actual presence
    
    // Register SD card storage
    MCP_DeviceInfoRegisterStorage(&sdInfo);
#elif defined(ARDUINO)
    // Arduino EEPROM
    MCP_StorageInfo eepromInfo = {0};
    strncpy(eepromInfo.name, "EEPROM", sizeof(eepromInfo.name) - 1);
    strncpy(eepromInfo.type, "eeprom", sizeof(eepromInfo.type) - 1);
    
    // EEPROM size varies by board
    #if defined(ARDUINO_ARCH_AVR)
    eepromInfo.totalSize = 1024; // 1KB on ATmega328
    #elif defined(ARDUINO_ARCH_SAM)
    eepromInfo.totalSize = 4 * 1024; // 4KB emulated on Due
    #else 
    eepromInfo.totalSize = 512; // Generic estimate
    #endif
    
    eepromInfo.freeSpace = eepromInfo.totalSize; // Approximation
    eepromInfo.isWriteProtected = false;
    eepromInfo.isRemovable = false;
    eepromInfo.isPresent = true;
    
    // Register EEPROM storage
    MCP_DeviceInfoRegisterStorage(&eepromInfo);
    
    // SD Card (if present)
    MCP_StorageInfo sdInfo = {0};
    strncpy(sdInfo.name, "SD Card", sizeof(sdInfo.name) - 1);
    strncpy(sdInfo.type, "sd", sizeof(sdInfo.type) - 1);
    sdInfo.totalSize = 4 * 1024 * 1024 * 1024ULL; // 4 GB example
    sdInfo.freeSpace = 3 * 1024 * 1024 * 1024ULL; // 3 GB example
    sdInfo.isWriteProtected = false;
    sdInfo.isRemovable = true;
    sdInfo.isPresent = false; // Would check actual presence
    
    // Register SD card storage
    MCP_DeviceInfoRegisterStorage(&sdInfo);
#elif defined(MBED)
    // Mbed flash
    MCP_StorageInfo flashInfo = {0};
    strncpy(flashInfo.name, "Internal Flash", sizeof(flashInfo.name) - 1);
    strncpy(flashInfo.type, "flash", sizeof(flashInfo.type) - 1);
    flashInfo.totalSize = 512 * 1024; // 512 KB example
    flashInfo.freeSpace = 256 * 1024; // 256 KB example
    flashInfo.isWriteProtected = false;
    flashInfo.isRemovable = false;
    flashInfo.isPresent = true;
    
    // Register flash storage
    MCP_DeviceInfoRegisterStorage(&flashInfo);
#else
    // Generic storage
    MCP_StorageInfo flashInfo = {0};
    strncpy(flashInfo.name, "Internal Flash", sizeof(flashInfo.name) - 1);
    strncpy(flashInfo.type, "flash", sizeof(flashInfo.type) - 1);
    flashInfo.totalSize = 1024 * 1024; // 1 MB
    flashInfo.freeSpace = 512 * 1024; // 512 KB
    flashInfo.isWriteProtected = false;
    flashInfo.isRemovable = false;
    flashInfo.isPresent = true;
    
    // Register flash storage
    MCP_DeviceInfoRegisterStorage(&flashInfo);
    
    // EEPROM
    MCP_StorageInfo eepromInfo = {0};
    strncpy(eepromInfo.name, "EEPROM", sizeof(eepromInfo.name) - 1);
    strncpy(eepromInfo.type, "eeprom", sizeof(eepromInfo.type) - 1);
    eepromInfo.totalSize = 4 * 1024; // 4 KB
    eepromInfo.freeSpace = 2 * 1024; // 2 KB
    eepromInfo.isWriteProtected = false;
    eepromInfo.isRemovable = false;
    eepromInfo.isPresent = true;
    
    // Register EEPROM storage
    MCP_DeviceInfoRegisterStorage(&eepromInfo);
#endif
}