#ifndef MCP_SENSOR_MANAGER_H
#define MCP_SENSOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Sensor value types
 */
typedef enum {
    MCP_SENSOR_VALUE_TYPE_BOOL,
    MCP_SENSOR_VALUE_TYPE_INT,
    MCP_SENSOR_VALUE_TYPE_FLOAT,
    MCP_SENSOR_VALUE_TYPE_STRING,
    MCP_SENSOR_VALUE_TYPE_OBJECT
} MCP_SensorValueType;

/**
 * @brief Sensor value union
 */
typedef union {
    bool boolValue;
    int32_t intValue;
    float floatValue;
    char* stringValue;
    void* objectValue;
} MCP_SensorValueData;

/**
 * @brief Sensor value structure
 */
typedef struct {
    MCP_SensorValueType type;
    MCP_SensorValueData value;
    uint32_t timestamp;
} MCP_SensorValue;

/**
 * @brief Sensor types
 */
typedef enum {
    MCP_SENSOR_TYPE_TEMPERATURE,
    MCP_SENSOR_TYPE_HUMIDITY,
    MCP_SENSOR_TYPE_PRESSURE,
    MCP_SENSOR_TYPE_MOTION,
    MCP_SENSOR_TYPE_LIGHT,
    MCP_SENSOR_TYPE_SOUND,
    MCP_SENSOR_TYPE_DISTANCE,
    MCP_SENSOR_TYPE_GAS,
    MCP_SENSOR_TYPE_VOLTAGE,
    MCP_SENSOR_TYPE_CURRENT,
    MCP_SENSOR_TYPE_POWER,
    MCP_SENSOR_TYPE_CUSTOM
} MCP_SensorType;

/**
 * @brief Sensor interface types
 */
typedef enum {
    MCP_SENSOR_INTERFACE_ANALOG,
    MCP_SENSOR_INTERFACE_DIGITAL,
    MCP_SENSOR_INTERFACE_I2C,
    MCP_SENSOR_INTERFACE_SPI,
    MCP_SENSOR_INTERFACE_UART,
    MCP_SENSOR_INTERFACE_ONEWIRE,
    MCP_SENSOR_INTERFACE_CUSTOM
} MCP_SensorInterface;

/**
 * @brief Sensor configuration structure
 */
typedef struct {
    char* id;                   // Sensor ID
    char* name;                 // Sensor name
    MCP_SensorType type;        // Sensor type
    MCP_SensorInterface iface;  // Sensor interface
    char* pin;                  // Pin name
    char* driverId;             // Driver ID
    char* configJson;           // Sensor-specific configuration
    uint32_t sampleInterval;    // Sample interval in milliseconds
} MCP_SensorConfig;

/**
 * @brief Sensor status structure
 */
typedef struct {
    char* id;                   // Sensor ID
    bool connected;             // Is sensor connected
    bool enabled;               // Is sensor enabled
    uint32_t lastSampleTime;    // Last sample timestamp
    uint32_t sampleCount;       // Number of samples taken
    MCP_SensorValue lastValue;  // Last sensor value
} MCP_SensorStatus;

/**
 * @brief Initialize the sensor manager
 * 
 * @param maxSensors Maximum number of sensors to manage
 * @return int 0 on success, negative error code on failure
 */
int MCP_SensorManagerInit(uint16_t maxSensors);

/**
 * @brief Register a sensor
 * 
 * @param config Sensor configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_SensorRegister(const MCP_SensorConfig* config);

/**
 * @brief Unregister a sensor
 * 
 * @param id Sensor ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_SensorUnregister(const char* id);

/**
 * @brief Find a sensor by ID
 * 
 * @param id Sensor ID
 * @return MCP_SensorConfig* Sensor configuration or NULL if not found
 */
const MCP_SensorConfig* MCP_SensorFind(const char* id);

/**
 * @brief Get list of sensors by type
 * 
 * @param type Sensor type to filter (or -1 for all)
 * @param sensors Array to store sensor pointers
 * @param maxSensors Maximum number of sensors to return
 * @return int Number of sensors found or negative error code
 */
int MCP_SensorGetByType(int type, const MCP_SensorConfig** sensors, uint16_t maxSensors);

/**
 * @brief Enable a sensor
 * 
 * @param id Sensor ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_SensorEnable(const char* id);

/**
 * @brief Disable a sensor
 * 
 * @param id Sensor ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_SensorDisable(const char* id);

/**
 * @brief Read a sensor value
 * 
 * @param id Sensor ID
 * @param value Pointer to store sensor value
 * @return int 0 on success, negative error code on failure
 */
int MCP_SensorRead(const char* id, MCP_SensorValue* value);

/**
 * @brief Get sensor status
 * 
 * @param id Sensor ID
 * @param status Pointer to store sensor status
 * @return int 0 on success, negative error code on failure
 */
int MCP_SensorGetStatus(const char* id, MCP_SensorStatus* status);

/**
 * @brief Process all sensors (sample as needed)
 * 
 * @param currentTimeMs Current system time in milliseconds
 * @return int Number of sensors processed or negative error code
 */
int MCP_SensorProcess(uint32_t currentTimeMs);

/**
 * @brief Set sensor config value
 * 
 * @param id Sensor ID
 * @param configKey Configuration key
 * @param value Configuration value (JSON format)
 * @return int 0 on success, negative error code on failure
 */
int MCP_SensorSetConfig(const char* id, const char* configKey, const char* value);

/**
 * @brief Create sensor value from different types
 */
MCP_SensorValue MCP_SensorCreateBoolValue(bool value);
MCP_SensorValue MCP_SensorCreateIntValue(int32_t value);
MCP_SensorValue MCP_SensorCreateFloatValue(float value);
MCP_SensorValue MCP_SensorCreateStringValue(const char* value);

/**
 * @brief Free sensor value resources if needed
 * 
 * @param value Sensor value to free
 */
void MCP_SensorFreeValue(MCP_SensorValue* value);

#endif /* MCP_SENSOR_MANAGER_H */