#ifndef MCP_DRIVER_MANAGER_H
#define MCP_DRIVER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Driver type enumeration
 */
typedef enum {
    MCP_DRIVER_TYPE_SENSOR,    // Sensor driver
    MCP_DRIVER_TYPE_ACTUATOR,  // Actuator driver
    MCP_DRIVER_TYPE_INTERFACE, // Interface driver (e.g., I2C, SPI)
    MCP_DRIVER_TYPE_STORAGE,   // Storage driver
    MCP_DRIVER_TYPE_NETWORK,   // Network driver
    MCP_DRIVER_TYPE_CUSTOM     // Custom driver
} MCP_DriverType;

/**
 * @brief Driver interface structure
 * Contains function pointers for standard driver operations
 */
typedef struct {
    // Initialization function
    int (*init)(const void* config);
    
    // Deinitialization function
    int (*deinit)(void);
    
    // Read function
    int (*read)(void* data, size_t maxSize, size_t* actualSize);
    
    // Write function
    int (*write)(const void* data, size_t size);
    
    // Control function
    int (*control)(uint32_t command, void* arg);
    
    // Get status function
    int (*getStatus)(void* status, size_t maxSize);
} MCP_DriverInterface;

/**
 * @brief Driver information structure
 */
typedef struct {
    char* id;                   // Driver ID
    char* name;                 // Driver name
    char* version;              // Driver version
    MCP_DriverType type;        // Driver type
    MCP_DriverInterface iface;  // Driver interface
    bool initialized;           // Driver initialization state
    char* configSchema;         // JSON schema for configuration
} MCP_DriverInfo;

/**
 * @brief Initialize the driver manager
 * 
 * @param maxDrivers Maximum number of drivers
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverManagerInit(uint16_t maxDrivers);

/**
 * @brief Register a driver
 * 
 * @param info Driver information structure
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverRegister(const MCP_DriverInfo* info);

/**
 * @brief Unregister a driver
 * 
 * @param id Driver ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverUnregister(const char* id);

/**
 * @brief Find a driver by ID
 * 
 * @param id Driver ID
 * @return MCP_DriverInfo* Driver information or NULL if not found
 */
const MCP_DriverInfo* MCP_DriverFind(const char* id);

/**
 * @brief Get list of drivers by type
 * 
 * @param type Driver type to filter (or -1 for all)
 * @param drivers Array to store driver pointers
 * @param maxDrivers Maximum number of drivers to return
 * @return int Number of drivers found or negative error code
 */
int MCP_DriverGetByType(int type, const MCP_DriverInfo** drivers, uint16_t maxDrivers);

/**
 * @brief Initialize a driver
 * 
 * @param id Driver ID
 * @param config Driver configuration (JSON string)
 * @param configLength Length of configuration string
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverInitialize(const char* id, const char* config, size_t configLength);

/**
 * @brief Deinitialize a driver
 * 
 * @param id Driver ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverDeinitialize(const char* id);

/**
 * @brief Read data from a driver
 * 
 * @param id Driver ID
 * @param data Buffer to store read data
 * @param maxSize Maximum size of buffer
 * @param actualSize Actual size read
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverRead(const char* id, void* data, size_t maxSize, size_t* actualSize);

/**
 * @brief Write data to a driver
 * 
 * @param id Driver ID
 * @param data Data to write
 * @param size Size of data
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverWrite(const char* id, const void* data, size_t size);

/**
 * @brief Send control command to a driver
 * 
 * @param id Driver ID
 * @param command Command code
 * @param arg Command argument
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverControl(const char* id, uint32_t command, void* arg);

/**
 * @brief Get status from a driver
 * 
 * @param id Driver ID
 * @param status Buffer to store status
 * @param maxSize Maximum size of buffer
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverGetStatus(const char* id, void* status, size_t maxSize);

/**
 * @brief Export driver configuration as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_DriverExportConfig(char* buffer, size_t bufferSize);

#endif /* MCP_DRIVER_MANAGER_H */