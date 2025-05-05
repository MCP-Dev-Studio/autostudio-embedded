#ifndef DEVICE_INFO_STRUCTS_H
#define DEVICE_INFO_STRUCTS_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct MCP_DeviceInfo MCP_DeviceInfo;
typedef struct MCP_SystemInfo MCP_SystemInfo;
typedef struct MCP_ProcessorInfo MCP_ProcessorInfo;
typedef struct MCP_MemoryInfo MCP_MemoryInfo;
typedef struct MCP_IOPortInfo MCP_IOPortInfo;
typedef struct MCP_NetworkInfo MCP_NetworkInfo;
typedef struct MCP_SensorInfo MCP_SensorInfo;
typedef struct MCP_StorageInfo MCP_StorageInfo;

// System information
typedef struct MCP_SystemInfo {
    char deviceName[64];
    char firmwareVersion[32];
    char buildDate[32];
    char buildTime[32];
    char platformName[32];
    char boardModel[64];
    uint32_t uptime;
    uint32_t resetCount;
    char lastResetReason[64];
    bool isDebugMode;
} MCP_SystemInfo;

// Processor information
typedef struct MCP_ProcessorInfo {
    char model[64];
    uint32_t clockSpeed;
    uint8_t coreCount;
    uint8_t bitWidth;
    float temperatureC;
    uint8_t loadPercent;
} MCP_ProcessorInfo;

// Memory information
typedef struct MCP_MemoryInfo {
    uint32_t totalRam;
    uint32_t freeRam;
    uint32_t totalFlash;
    uint32_t freeFlash;
    uint32_t stackSize;
    uint32_t heapSize;
} MCP_MemoryInfo;

// IO port information
typedef struct MCP_IOPortInfo {
    char name[32];
    uint8_t portType; // 0 = digital, 1 = analog, 2 = PWM, 3 = serial, 4 = I2C, 5 = SPI
    bool isInput;
    bool isOutput;
    bool isPullUp;
    bool isPullDown;
    bool isAnalog;
    bool isPWM;
    bool isI2C;
    bool isSPI;
    bool isUART;
    bool isInterrupt;
    bool currentState;
    uint32_t analogValue;
    bool isReserved;
} MCP_IOPortInfo;

// Network interface information
typedef struct MCP_NetworkInfo {
    char name[32];
    char macAddress[32];
    char ipAddress[32];
    bool isConnected;
    bool isWifi;
    bool isEthernet;
    bool isBluetooth;
    uint8_t signalStrength;
} MCP_NetworkInfo;

// Sensor information
typedef struct MCP_SensorInfo {
    char name[64];
    char type[32];
    char units[16];
    float minValue;
    float maxValue;
    float currentValue;
    bool isConnected;
    char busType[16];
    uint8_t address;
} MCP_SensorInfo;

// Storage device information
typedef struct MCP_StorageInfo {
    char name[64];
    char type[32];
    uint32_t totalSize;
    uint32_t freeSpace;
    bool isWriteProtected;
    bool isRemovable;
    bool isPresent;
} MCP_StorageInfo;

// Device information
typedef struct MCP_DeviceInfo {
    MCP_SystemInfo system;
    MCP_ProcessorInfo processor;
    MCP_MemoryInfo memory;
    MCP_IOPortInfo* ioPorts;
    uint16_t ioPortCount;
    MCP_NetworkInfo* networkInterfaces;
    uint8_t networkInterfaceCount;
    MCP_SensorInfo* sensors;
    uint8_t sensorCount;
    MCP_StorageInfo* storageDevices;
    uint8_t storageCount;
    char* capabilities;
} MCP_DeviceInfo;

#endif // DEVICE_INFO_STRUCTS_H
