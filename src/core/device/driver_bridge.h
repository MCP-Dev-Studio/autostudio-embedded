#ifndef MCP_DRIVER_BRIDGE_H
#define MCP_DRIVER_BRIDGE_H

#include "driver_bytecode.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize the driver bridge system
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverBridgeInit(void);

/**
 * @brief Initialize the driver bridge tool handlers
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverBridgeToolsInit(void);

/**
 * @brief Register a standard driver with the bytecode driver system
 * 
 * @param id Unique driver ID
 * @param name Driver name
 * @param type Driver type (sensor, actuator, etc.)
 * @param deviceType Device-specific type (LED, temperature, etc.)
 * @param configSchema JSON schema for configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverBridgeRegister(const char* id, const char* name, 
                            MCP_DriverType type, int deviceType,
                            const char* configSchema);

/**
 * @brief Unregister a standard driver
 * 
 * @param id Driver ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverBridgeUnregister(const char* id);

/**
 * @brief Map bytecode function to native driver function
 * 
 * @param driverId Driver ID
 * @param function Function name (init, read, etc.)
 * @param nativeFunction Native function pointer
 * @return int 0 on success, negative error code on failure
 */
int MCP_DriverBridgeMapFunction(const char* driverId, const char* function, void* nativeFunction);

/**
 * @brief Create an LED driver with the bridge system
 * 
 * @param id Unique driver ID
 * @param name Driver name
 * @param type LED type
 * @param pin GPIO pin number
 * @return int 0 on success, negative error code on failure
 */
int MCP_CreateLEDDriver(const char* id, const char* name, int type, uint8_t pin);

/**
 * @brief Create a DS18B20 temperature sensor driver with the bridge system
 * 
 * @param id Unique driver ID
 * @param name Driver name
 * @param pin OneWire pin number
 * @return int 0 on success, negative error code on failure
 */
int MCP_CreateDS18B20Driver(const char* id, const char* name, uint8_t pin);

/**
 * @brief Device types for bridge interface
 */
typedef enum {
    // Actuator types
    DEVICE_TYPE_LED_SIMPLE = 1000,
    DEVICE_TYPE_LED_PWM,
    DEVICE_TYPE_LED_RGB,
    DEVICE_TYPE_LED_RGBW,
    DEVICE_TYPE_LED_ADDRESSABLE,
    DEVICE_TYPE_SERVO,
    DEVICE_TYPE_RELAY,
    
    // Sensor types
    DEVICE_TYPE_TEMPERATURE_DS18B20 = 2000,
    DEVICE_TYPE_HUMIDITY_DHT22,
    DEVICE_TYPE_MOTION_PIR,
    
    // Other device types
    DEVICE_TYPE_CUSTOM = 5000
} DeviceType;

#endif /* MCP_DRIVER_BRIDGE_H */