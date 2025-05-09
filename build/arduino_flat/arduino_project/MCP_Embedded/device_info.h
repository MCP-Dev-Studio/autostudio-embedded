/**
 * @file device_info.h
 * @brief Device information and capabilities system
 */
#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device memory information
 */
typedef struct {
    uint32_t totalRam;        // Total RAM in bytes
    uint32_t freeRam;         // Free RAM in bytes
    uint32_t totalFlash;      // Total flash memory in bytes
    uint32_t freeFlash;       // Free flash memory in bytes
    uint32_t stackSize;       // Stack size in bytes
    uint32_t heapSize;        // Heap size in bytes
} MCP_MemoryInfo;

/**
 * @brief Device processor information
 */
typedef struct {
    char model[32];           // Processor model
    uint32_t clockSpeed;      // Clock speed in Hz
    uint8_t coreCount;        // Number of cores
    uint8_t bitWidth;         // Processor bit width (8, 16, 32, 64)
    float temperatureC;       // Current CPU temperature in Celsius (-1 if not available)
    uint8_t loadPercent;      // Current CPU load (0-100, 255 if not available)
} MCP_ProcessorInfo;

/**
 * @brief IO port description
 */
typedef struct {
    char name[16];            // Port name (e.g., "GPIO5", "A0")
    uint8_t portType;         // Port type (digital, analog, PWM, etc.)
    bool isInput;             // True if configured as input
    bool isOutput;            // True if configured as output
    bool isPullUp;            // True if pull-up is enabled
    bool isPullDown;          // True if pull-down is enabled
    bool isAnalog;            // True if analog capable
    bool isPWM;               // True if PWM capable
    bool isI2C;               // True if I2C capable
    bool isSPI;               // True if SPI capable
    bool isUART;              // True if UART capable
    bool isInterrupt;         // True if interrupt capable
    uint8_t currentState;     // Current state (for digital)
    uint16_t analogValue;     // Current value (for analog)
    bool isReserved;          // True if reserved for system use
} MCP_IOPortInfo;

/**
 * @brief Network interface information
 */
typedef struct {
    char name[16];            // Interface name
    char macAddress[18];      // MAC address (XX:XX:XX:XX:XX:XX)
    char ipAddress[16];       // IP address (if available)
    bool isConnected;         // True if connected
    bool isWifi;              // True if WiFi
    bool isEthernet;         // True if Ethernet
    bool isBluetooth;         // True if Bluetooth
    uint8_t signalStrength;   // Signal strength (0-100, 255 if not available)
} MCP_NetworkInfo;

/**
 * @brief Sensor information
 */
typedef struct {
    char name[32];            // Sensor name
    char type[16];            // Sensor type (temperature, humidity, etc.)
    char units[8];            // Measurement units
    float minValue;           // Minimum value
    float maxValue;           // Maximum value
    float currentValue;       // Current reading
    bool isConnected;         // True if connected and functioning
    char busType[8];          // Bus type (I2C, SPI, etc.)
    uint8_t address;          // Bus address (if applicable)
} MCP_SensorInfo;

/**
 * @brief Storage device information
 */
typedef struct {
    char name[32];            // Storage name
    char type[16];            // Storage type (EEPROM, SD, etc.)
    uint32_t totalSize;       // Total size in bytes
    uint32_t freeSpace;       // Free space in bytes
    bool isWriteProtected;    // True if write protected
    bool isRemovable;         // True if removable
    bool isPresent;           // True if present
} MCP_StorageInfo;

/**
 * @brief System information
 */
typedef struct {
    char deviceName[32];      // Device name
    char firmwareVersion[16]; // Firmware version
    char buildDate[16];       // Build date
    char buildTime[16];       // Build time
    char platformName[32];    // Platform name (mbed, Arduino, ESP32, etc.)
    char boardModel[32];      // Board model
    uint32_t uptime;          // Uptime in seconds
    uint32_t resetCount;      // Number of restarts
    char lastResetReason[32]; // Reason for last reset
    bool isDebugMode;         // True if in debug mode
} MCP_SystemInfo;

/**
 * @brief Comprehensive device information
 */
typedef struct {
    MCP_SystemInfo system;            // System information
    MCP_ProcessorInfo processor;      // Processor information
    MCP_MemoryInfo memory;            // Memory information
    uint16_t ioPortCount;             // Number of IO ports
    MCP_IOPortInfo* ioPorts;          // Array of IO port information
    uint8_t networkInterfaceCount;    // Number of network interfaces
    MCP_NetworkInfo* networkInterfaces; // Array of network interface information
    uint8_t sensorCount;              // Number of sensors
    MCP_SensorInfo* sensors;          // Array of sensor information
    uint8_t storageCount;             // Number of storage devices
    MCP_StorageInfo* storageDevices;  // Array of storage device information
    char* capabilities;               // JSON string of device capabilities
} MCP_DeviceInfo;

/**
 * @brief Initialize device information system
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_DeviceInfoInit(void);

/**
 * @brief Get current device information
 * 
 * @return MCP_DeviceInfo* Pointer to device information (do not free)
 */
const MCP_DeviceInfo* MCP_DeviceInfoGet(void);

/**
 * @brief Update device information (refreshes dynamic data)
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_DeviceInfoUpdate(void);

/**
 * @brief Convert device information to JSON
 * 
 * @param compact If true, generate compact JSON
 * @return char* JSON string (caller must free)
 */
char* MCP_DeviceInfoToJSON(bool compact);

/**
 * @brief Register IO port with device info system
 * 
 * @param portInfo Port information
 * @return int 0 on success, negative error code on failure
 */
int MCP_DeviceInfoRegisterIOPort(const MCP_IOPortInfo* portInfo);

/**
 * @brief Register sensor with device info system
 * 
 * @param sensorInfo Sensor information
 * @return int 0 on success, negative error code on failure
 */
int MCP_DeviceInfoRegisterSensor(const MCP_SensorInfo* sensorInfo);

/**
 * @brief Register storage device with device info system
 * 
 * @param storageInfo Storage device information
 * @return int 0 on success, negative error code on failure
 */
int MCP_DeviceInfoRegisterStorage(const MCP_StorageInfo* storageInfo);

/**
 * @brief Register network interface with device info system
 * 
 * @param networkInfo Network interface information
 * @return int 0 on success, negative error code on failure
 */
int MCP_DeviceInfoRegisterNetwork(const MCP_NetworkInfo* networkInfo);

/**
 * @brief Set system capabilities JSON
 * 
 * @param capabilities JSON string of capabilities (will be copied)
 * @return int 0 on success, negative error code on failure
 */
int MCP_DeviceInfoSetCapabilities(const char* capabilities);

/**
 * @brief Tool handler for system.getDeviceInfo
 * 
 * @param json Input JSON
 * @param length Length of input JSON
 * @return MCP_ToolResult Tool result
 */
#include "tool_registry.h"

MCP_ToolResult MCP_DeviceInfoToolHandler(const char* json, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_INFO_H */