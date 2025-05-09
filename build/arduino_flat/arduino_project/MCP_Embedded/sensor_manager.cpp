#include "arduino_compat.h"
#include "sensor_manager.h"
#include "driver_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Internal sensor storage
typedef struct {
    MCP_SensorConfig config;
    bool registered;
    bool enabled;
    uint32_t lastSampleTime;
    uint32_t sampleCount;
    MCP_SensorValue lastValue;
} SensorEntry;

// Internal state
static SensorEntry* s_sensors = NULL;
static uint16_t s_maxSensors = 0;
static uint16_t s_sensorCount = 0;
static bool s_initialized = false;

int MCP_SensorManagerInit(uint16_t maxSensors) {
    if (s_initialized) {
        return -1;  // Already initialized
    }
    
    // Allocate sensor array
    s_sensors = (SensorEntry*)calloc(maxSensors, sizeof(SensorEntry));
    if (s_sensors == NULL) {
        return -2;  // Memory allocation failed
    }
    
    s_maxSensors = maxSensors;
    s_sensorCount = 0;
    s_initialized = true;
    
    return 0;
}

int MCP_SensorRegister(const MCP_SensorConfig* config) {
    if (!s_initialized || config == NULL || config->id == NULL) {
        return -1;
    }
    
    // Check if sensor already exists
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (s_sensors[i].registered && strcmp(s_sensors[i].config.id, config->id) == 0) {
            return -2;  // Sensor already registered
        }
    }
    
    // Find free slot
    uint16_t slot = UINT16_MAX;
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (!s_sensors[i].registered) {
            slot = i;
            break;
        }
    }
    
    if (slot == UINT16_MAX) {
        return -3;  // No free slots
    }
    
    // Copy sensor config
    s_sensors[slot].config.id = strdup(config->id);
    if (s_sensors[slot].config.id == NULL) {
        return -4;  // Memory allocation failed
    }
    
    s_sensors[slot].config.name = config->name ? strdup(config->name) : NULL;
    s_sensors[slot].config.type = config->type;
    s_sensors[slot].config.iface = config->iface;
    s_sensors[slot].config.pin = config->pin ? strdup(config->pin) : NULL;
    s_sensors[slot].config.driverId = config->driverId ? strdup(config->driverId) : NULL;
    s_sensors[slot].config.configJson = config->configJson ? strdup(config->configJson) : NULL;
    s_sensors[slot].config.sampleInterval = config->sampleInterval;
    
    // Initialize status
    s_sensors[slot].registered = true;
    s_sensors[slot].enabled = false;
    s_sensors[slot].lastSampleTime = 0;
    s_sensors[slot].sampleCount = 0;
    s_sensors[slot].lastValue.type = MCP_SENSOR_VALUE_TYPE_INT;
    s_sensors[slot].lastValue.value.intValue = 0;
    s_sensors[slot].lastValue.timestamp = 0;
    
    s_sensorCount++;
    
    // Initialize driver if one is specified
    if (config->driverId != NULL) {
        const MCP_DriverInfo* driver = MCP_DriverFind(config->driverId);
        if (driver != NULL && !driver->initialized) {
            MCP_DriverInitialize(config->driverId, config->configJson, 
                              config->configJson ? strlen(config->configJson) : 0);
        }
    }
    
    return 0;
}

int MCP_SensorUnregister(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find sensor
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (s_sensors[i].registered && strcmp(s_sensors[i].config.id, id) == 0) {
            // Disable sensor first
            if (s_sensors[i].enabled) {
                MCP_SensorDisable(id);
            }
            
            // Free strings
            free(s_sensors[i].config.id);
            if (s_sensors[i].config.name) free(s_sensors[i].config.name);
            if (s_sensors[i].config.pin) free(s_sensors[i].config.pin);
            if (s_sensors[i].config.driverId) free(s_sensors[i].config.driverId);
            if (s_sensors[i].config.configJson) free(s_sensors[i].config.configJson);
            
            // Free last value if it's a string
            if (s_sensors[i].lastValue.type == MCP_SENSOR_VALUE_TYPE_STRING && 
                s_sensors[i].lastValue.value.stringValue != NULL) {
                free(s_sensors[i].lastValue.value.stringValue);
            }
            
            // Mark as unregistered
            s_sensors[i].registered = false;
            s_sensorCount--;
            
            return 0;
        }
    }
    
    return -2;  // Sensor not found
}

const MCP_SensorConfig* MCP_SensorFind(const char* id) {
    if (!s_initialized || id == NULL) {
        return NULL;
    }
    
    // Find sensor
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (s_sensors[i].registered && strcmp(s_sensors[i].config.id, id) == 0) {
            return &s_sensors[i].config;
        }
    }
    
    return NULL;  // Sensor not found
}

int MCP_SensorGetByType(int type, const MCP_SensorConfig** sensors, uint16_t maxSensors) {
    if (!s_initialized || sensors == NULL || maxSensors == 0) {
        return -1;
    }
    
    uint16_t count = 0;
    
    // Find sensors of the specified type
    for (uint16_t i = 0; i < s_maxSensors && count < maxSensors; i++) {
        if (s_sensors[i].registered && (type < 0 || (int)s_sensors[i].config.type == type)) {
            sensors[count++] = &s_sensors[i].config;
        }
    }
    
    return count;
}

int MCP_SensorEnable(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find sensor
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (s_sensors[i].registered && strcmp(s_sensors[i].config.id, id) == 0) {
            // Already enabled?
            if (s_sensors[i].enabled) {
                return 0;
            }
            
            // Enable the driver if one is specified
            if (s_sensors[i].config.driverId != NULL) {
                const MCP_DriverInfo* driver = MCP_DriverFind(s_sensors[i].config.driverId);
                if (driver != NULL && !driver->initialized) {
                    int result = MCP_DriverInitialize(s_sensors[i].config.driverId, 
                                                   s_sensors[i].config.configJson,
                                                   s_sensors[i].config.configJson ? 
                                                   strlen(s_sensors[i].config.configJson) : 0);
                    if (result != 0) {
                        return result;
                    }
                }
            }
            
            // Mark as enabled
            s_sensors[i].enabled = true;
            
            return 0;
        }
    }
    
    return -2;  // Sensor not found
}

int MCP_SensorDisable(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find sensor
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (s_sensors[i].registered && strcmp(s_sensors[i].config.id, id) == 0) {
            // Already disabled?
            if (!s_sensors[i].enabled) {
                return 0;
            }
            
            // Mark as disabled
            s_sensors[i].enabled = false;
            
            return 0;
        }
    }
    
    return -2;  // Sensor not found
}

int MCP_SensorRead(const char* id, MCP_SensorValue* value) {
    if (!s_initialized || id == NULL || value == NULL) {
        return -1;
    }
    
    // Find sensor
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (s_sensors[i].registered && strcmp(s_sensors[i].config.id, id) == 0) {
            // Check if enabled
            if (!s_sensors[i].enabled) {
                return -2;  // Sensor disabled
            }
            
            // If the sensor has a driver, read from it
            if (s_sensors[i].config.driverId != NULL) {
                const MCP_DriverInfo* driver = MCP_DriverFind(s_sensors[i].config.driverId);
                if (driver != NULL && driver->initialized && driver->iface.read != NULL) {
                    // Read data from driver
                    MCP_SensorValue driverValue;
                    size_t actualSize = 0;
                    
                    int result = driver->iface.read(&driverValue, sizeof(driverValue), &actualSize);
                    if (result == 0 && actualSize == sizeof(driverValue)) {
                        // Update last value
                        MCP_SensorFreeValue(&s_sensors[i].lastValue);
                        
                        // Copy driver value
                        s_sensors[i].lastValue = driverValue;
                        s_sensors[i].lastValue.timestamp = 0;  // Will be updated in the next step
                        
                        // Copy to output value
                        *value = s_sensors[i].lastValue;
                        
                        // Update timestamp and sample count
                        uint32_t currentTime = 0;  // TODO: get current time
                        s_sensors[i].lastSampleTime = currentTime;
                        s_sensors[i].lastValue.timestamp = currentTime;
                        value->timestamp = currentTime;
                        s_sensors[i].sampleCount++;
                        
                        return 0;
                    } else {
                        return -3;  // Driver read failed
                    }
                } else {
                    return -4;  // Driver not available
                }
            } else {
                // No driver, simply return last value
                *value = s_sensors[i].lastValue;
                return 0;
            }
        }
    }
    
    return -5;  // Sensor not found
}

int MCP_SensorGetStatus(const char* id, MCP_SensorStatus* status) {
    if (!s_initialized || id == NULL || status == NULL) {
        return -1;
    }
    
    // Find sensor
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (s_sensors[i].registered && strcmp(s_sensors[i].config.id, id) == 0) {
            // Fill status
            status->id = s_sensors[i].config.id;
            status->connected = true;  // Assume connected for simplicity
            status->enabled = s_sensors[i].enabled;
            status->lastSampleTime = s_sensors[i].lastSampleTime;
            status->sampleCount = s_sensors[i].sampleCount;
            status->lastValue = s_sensors[i].lastValue;
            
            return 0;
        }
    }
    
    return -2;  // Sensor not found
}

int MCP_SensorProcess(uint32_t currentTimeMs) {
    if (!s_initialized) {
        return -1;
    }
    
    int processed = 0;
    
    // Process each enabled sensor
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (s_sensors[i].registered && s_sensors[i].enabled) {
            // Check if it's time to sample
            if (currentTimeMs - s_sensors[i].lastSampleTime >= s_sensors[i].config.sampleInterval) {
                // Read sensor
                MCP_SensorValue value;
                int result = MCP_SensorRead(s_sensors[i].config.id, &value);
                
                if (result == 0) {
                    processed++;
                }
            }
        }
    }
    
    return processed;
}

int MCP_SensorSetConfig(const char* id, const char* configKey, const char* value) {
    if (!s_initialized || id == NULL || configKey == NULL || value == NULL) {
        return -1;
    }
    
    // Find sensor
    for (uint16_t i = 0; i < s_maxSensors; i++) {
        if (s_sensors[i].registered && strcmp(s_sensors[i].config.id, id) == 0) {
            // Get driver
            if (s_sensors[i].config.driverId != NULL) {
                const MCP_DriverInfo* driver = MCP_DriverFind(s_sensors[i].config.driverId);
                if (driver != NULL && driver->initialized && driver->iface.control != NULL) {
                    // Create control command
                    char command[128];
                    snprintf(command, sizeof(command), "{\"key\":\"%s\",\"value\":%s}", configKey, value);
                    
                    // Send control command to driver
                    return driver->iface.control(0, command);
                } else {
                    return -2;  // Driver not available
                }
            } else {
                return -3;  // No driver
            }
        }
    }
    
    return -4;  // Sensor not found
}

MCP_SensorValue MCP_SensorCreateBoolValue(bool value) {
    MCP_SensorValue sensorValue;
    sensorValue.type = MCP_SENSOR_VALUE_TYPE_BOOL;
    sensorValue.value.boolValue = value;
    sensorValue.timestamp = 0;  // Will be set when used
    return sensorValue;
}

MCP_SensorValue MCP_SensorCreateIntValue(int32_t value) {
    MCP_SensorValue sensorValue;
    sensorValue.type = MCP_SENSOR_VALUE_TYPE_INT;
    sensorValue.value.intValue = value;
    sensorValue.timestamp = 0;  // Will be set when used
    return sensorValue;
}

MCP_SensorValue MCP_SensorCreateFloatValue(float value) {
    MCP_SensorValue sensorValue;
    sensorValue.type = MCP_SENSOR_VALUE_TYPE_FLOAT;
    sensorValue.value.floatValue = value;
    sensorValue.timestamp = 0;  // Will be set when used
    return sensorValue;
}

MCP_SensorValue MCP_SensorCreateStringValue(const char* value) {
    MCP_SensorValue sensorValue;
    
    if (value != NULL) {
        sensorValue.type = MCP_SENSOR_VALUE_TYPE_STRING;
        sensorValue.value.stringValue = strdup(value);
        
        if (sensorValue.value.stringValue == NULL) {
            sensorValue.type = MCP_SENSOR_VALUE_TYPE_INT;
            sensorValue.value.intValue = 0;
        }
    } else {
        sensorValue.type = MCP_SENSOR_VALUE_TYPE_STRING;
        sensorValue.value.stringValue = NULL;
    }
    
    sensorValue.timestamp = 0;  // Will be set when used
    return sensorValue;
}

void MCP_SensorFreeValue(MCP_SensorValue* value) {
    if (value == NULL) {
        return;
    }
    
    if (value->type == MCP_SENSOR_VALUE_TYPE_STRING && value->value.stringValue != NULL) {
        free(value->value.stringValue);
        value->value.stringValue = NULL;
    }
}
