#include "actuator_manager.h"
#include "driver_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Internal actuator storage
typedef struct {
    MCP_ActuatorConfig config;
    bool registered;
    bool enabled;
    uint32_t lastUpdateTime;
    uint32_t updateCount;
    MCP_ActuatorState currentState;
} ActuatorEntry;

// Internal state
static ActuatorEntry* s_actuators = NULL;
static uint16_t s_maxActuators = 0;
static uint16_t s_actuatorCount = 0;
static bool s_initialized = false;

int MCP_ActuatorManagerInit(uint16_t maxActuators) {
    if (s_initialized) {
        return -1;  // Already initialized
    }
    
    // Allocate actuator array
    s_actuators = (ActuatorEntry*)calloc(maxActuators, sizeof(ActuatorEntry));
    if (s_actuators == NULL) {
        return -2;  // Memory allocation failed
    }
    
    s_maxActuators = maxActuators;
    s_actuatorCount = 0;
    s_initialized = true;
    
    return 0;
}

// Helper function to copy actuator state
static int copyActuatorState(MCP_ActuatorState* dest, const MCP_ActuatorState* src) {
    if (dest == NULL || src == NULL) {
        return -1;
    }
    
    // Free destination if it contains dynamic memory
    MCP_ActuatorFreeState(dest);
    
    // Copy type and timestamp
    dest->type = src->type;
    dest->timestamp = src->timestamp;
    
    // Copy value based on type
    switch (src->type) {
        case MCP_ACTUATOR_STATE_TYPE_BOOL:
            dest->value.boolValue = src->value.boolValue;
            break;
            
        case MCP_ACTUATOR_STATE_TYPE_INT:
            dest->value.intValue = src->value.intValue;
            break;
            
        case MCP_ACTUATOR_STATE_TYPE_FLOAT:
            dest->value.floatValue = src->value.floatValue;
            break;
            
        case MCP_ACTUATOR_STATE_TYPE_STRING:
            if (src->value.stringValue != NULL) {
                dest->value.stringValue = strdup(src->value.stringValue);
                if (dest->value.stringValue == NULL) {
                    return -2;  // Memory allocation failed
                }
            } else {
                dest->value.stringValue = NULL;
            }
            break;
            
        case MCP_ACTUATOR_STATE_TYPE_OBJECT:
            // Not implemented for simplicity
            dest->value.objectValue = NULL;
            break;
    }
    
    return 0;
}

int MCP_ActuatorRegister(const MCP_ActuatorConfig* config) {
    if (!s_initialized || config == NULL || config->id == NULL) {
        return -1;
    }
    
    // Check if actuator already exists
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (s_actuators[i].registered && strcmp(s_actuators[i].config.id, config->id) == 0) {
            return -2;  // Actuator already registered
        }
    }
    
    // Find free slot
    uint16_t slot = UINT16_MAX;
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (!s_actuators[i].registered) {
            slot = i;
            break;
        }
    }
    
    if (slot == UINT16_MAX) {
        return -3;  // No free slots
    }
    
    // Copy actuator config
    s_actuators[slot].config.id = strdup(config->id);
    if (s_actuators[slot].config.id == NULL) {
        return -4;  // Memory allocation failed
    }
    
    s_actuators[slot].config.name = config->name ? strdup(config->name) : NULL;
    s_actuators[slot].config.type = config->type;
    s_actuators[slot].config.iface = config->iface;
    s_actuators[slot].config.pin = config->pin ? strdup(config->pin) : NULL;
    s_actuators[slot].config.driverId = config->driverId ? strdup(config->driverId) : NULL;
    s_actuators[slot].config.configJson = config->configJson ? strdup(config->configJson) : NULL;
    
    // Copy initial state
    copyActuatorState(&s_actuators[slot].config.initialState, &config->initialState);
    
    // Initialize status
    s_actuators[slot].registered = true;
    s_actuators[slot].enabled = false;
    s_actuators[slot].lastUpdateTime = 0;
    s_actuators[slot].updateCount = 0;
    
    // Copy initial state to current state
    copyActuatorState(&s_actuators[slot].currentState, &config->initialState);
    
    s_actuatorCount++;
    
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

int MCP_ActuatorUnregister(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find actuator
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (s_actuators[i].registered && strcmp(s_actuators[i].config.id, id) == 0) {
            // Disable actuator first
            if (s_actuators[i].enabled) {
                MCP_ActuatorDisable(id);
            }
            
            // Free strings
            free(s_actuators[i].config.id);
            if (s_actuators[i].config.name) free(s_actuators[i].config.name);
            if (s_actuators[i].config.pin) free(s_actuators[i].config.pin);
            if (s_actuators[i].config.driverId) free(s_actuators[i].config.driverId);
            if (s_actuators[i].config.configJson) free(s_actuators[i].config.configJson);
            
            // Free states
            MCP_ActuatorFreeState(&s_actuators[i].config.initialState);
            MCP_ActuatorFreeState(&s_actuators[i].currentState);
            
            // Mark as unregistered
            s_actuators[i].registered = false;
            s_actuatorCount--;
            
            return 0;
        }
    }
    
    return -2;  // Actuator not found
}

const MCP_ActuatorConfig* MCP_ActuatorFind(const char* id) {
    if (!s_initialized || id == NULL) {
        return NULL;
    }
    
    // Find actuator
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (s_actuators[i].registered && strcmp(s_actuators[i].config.id, id) == 0) {
            return &s_actuators[i].config;
        }
    }
    
    return NULL;  // Actuator not found
}

int MCP_ActuatorGetByType(int type, const MCP_ActuatorConfig** actuators, uint16_t maxActuators) {
    if (!s_initialized || actuators == NULL || maxActuators == 0) {
        return -1;
    }
    
    uint16_t count = 0;
    
    // Find actuators of the specified type
    for (uint16_t i = 0; i < s_maxActuators && count < maxActuators; i++) {
        if (s_actuators[i].registered && (type < 0 || (int)s_actuators[i].config.type == type)) {
            actuators[count++] = &s_actuators[i].config;
        }
    }
    
    return count;
}

int MCP_ActuatorEnable(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find actuator
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (s_actuators[i].registered && strcmp(s_actuators[i].config.id, id) == 0) {
            // Already enabled?
            if (s_actuators[i].enabled) {
                return 0;
            }
            
            // Enable the driver if one is specified
            if (s_actuators[i].config.driverId != NULL) {
                const MCP_DriverInfo* driver = MCP_DriverFind(s_actuators[i].config.driverId);
                if (driver != NULL && !driver->initialized) {
                    int result = MCP_DriverInitialize(s_actuators[i].config.driverId, 
                                                   s_actuators[i].config.configJson,
                                                   s_actuators[i].config.configJson ? 
                                                   strlen(s_actuators[i].config.configJson) : 0);
                    if (result != 0) {
                        return result;
                    }
                }
                
                // Apply initial state
                if (s_actuators[i].updateCount == 0) {
                    MCP_ActuatorSetState(id, &s_actuators[i].config.initialState);
                }
            }
            
            // Mark as enabled
            s_actuators[i].enabled = true;
            
            return 0;
        }
    }
    
    return -2;  // Actuator not found
}

int MCP_ActuatorDisable(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find actuator
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (s_actuators[i].registered && strcmp(s_actuators[i].config.id, id) == 0) {
            // Already disabled?
            if (!s_actuators[i].enabled) {
                return 0;
            }
            
            // Mark as disabled
            s_actuators[i].enabled = false;
            
            return 0;
        }
    }
    
    return -2;  // Actuator not found
}

int MCP_ActuatorSetState(const char* id, const MCP_ActuatorState* state) {
    if (!s_initialized || id == NULL || state == NULL) {
        return -1;
    }
    
    // Find actuator
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (s_actuators[i].registered && strcmp(s_actuators[i].config.id, id) == 0) {
            // Check if enabled
            if (!s_actuators[i].enabled) {
                return -2;  // Actuator disabled
            }
            
            // If the actuator has a driver, write to it
            if (s_actuators[i].config.driverId != NULL) {
                const MCP_DriverInfo* driver = MCP_DriverFind(s_actuators[i].config.driverId);
                if (driver != NULL && driver->initialized && driver->iface.write != NULL) {
                    // Write state to driver
                    int result = driver->iface.write(state, sizeof(*state));
                    
                    if (result == 0) {
                        // Update current state
                        copyActuatorState(&s_actuators[i].currentState, state);
                        
                        // Update timestamp and count
                        uint32_t currentTime = 0;  // TODO: get current time
                        s_actuators[i].lastUpdateTime = currentTime;
                        s_actuators[i].currentState.timestamp = currentTime;
                        s_actuators[i].updateCount++;
                        
                        return 0;
                    } else {
                        return result;  // Driver write failed
                    }
                } else {
                    return -3;  // Driver not available
                }
            } else {
                // No driver, simply update current state
                copyActuatorState(&s_actuators[i].currentState, state);
                
                // Update timestamp and count
                uint32_t currentTime = 0;  // TODO: get current time
                s_actuators[i].lastUpdateTime = currentTime;
                s_actuators[i].currentState.timestamp = currentTime;
                s_actuators[i].updateCount++;
                
                return 0;
            }
        }
    }
    
    return -4;  // Actuator not found
}

int MCP_ActuatorGetState(const char* id, MCP_ActuatorState* state) {
    if (!s_initialized || id == NULL || state == NULL) {
        return -1;
    }
    
    // Find actuator
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (s_actuators[i].registered && strcmp(s_actuators[i].config.id, id) == 0) {
            // Copy current state
            copyActuatorState(state, &s_actuators[i].currentState);
            
            return 0;
        }
    }
    
    return -2;  // Actuator not found
}

int MCP_ActuatorGetStatus(const char* id, MCP_ActuatorStatus* status) {
    if (!s_initialized || id == NULL || status == NULL) {
        return -1;
    }
    
    // Find actuator
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (s_actuators[i].registered && strcmp(s_actuators[i].config.id, id) == 0) {
            // Fill status
            status->id = s_actuators[i].config.id;
            status->connected = true;  // Assume connected for simplicity
            status->enabled = s_actuators[i].enabled;
            status->lastUpdateTime = s_actuators[i].lastUpdateTime;
            status->updateCount = s_actuators[i].updateCount;
            
            // Copy current state
            copyActuatorState(&status->currentState, &s_actuators[i].currentState);
            
            return 0;
        }
    }
    
    return -2;  // Actuator not found
}

int MCP_ActuatorSendCommand(const char* id, const char* command, const char* params) {
    if (!s_initialized || id == NULL || command == NULL) {
        return -1;
    }
    
    // Find actuator
    for (uint16_t i = 0; i < s_maxActuators; i++) {
        if (s_actuators[i].registered && strcmp(s_actuators[i].config.id, id) == 0) {
            // Check if enabled
            if (!s_actuators[i].enabled) {
                return -2;  // Actuator disabled
            }
            
            // If the actuator has a driver, send command to it
            if (s_actuators[i].config.driverId != NULL) {
                const MCP_DriverInfo* driver = MCP_DriverFind(s_actuators[i].config.driverId);
                if (driver != NULL && driver->initialized && driver->iface.control != NULL) {
                    // Create command structure
                    char commandJson[512];
                    snprintf(commandJson, sizeof(commandJson), 
                             "{\"command\":\"%s\",\"params\":%s}", 
                             command, params ? params : "{}");
                    
                    // Send command to driver
                    return driver->iface.control(0, commandJson);
                } else {
                    return -3;  // Driver not available
                }
            } else {
                return -4;  // No driver
            }
        }
    }
    
    return -5;  // Actuator not found
}

MCP_ActuatorState MCP_ActuatorCreateBoolState(bool value) {
    MCP_ActuatorState state;
    state.type = MCP_ACTUATOR_STATE_TYPE_BOOL;
    state.value.boolValue = value;
    state.timestamp = 0;  // Will be set when used
    return state;
}

MCP_ActuatorState MCP_ActuatorCreateIntState(int32_t value) {
    MCP_ActuatorState state;
    state.type = MCP_ACTUATOR_STATE_TYPE_INT;
    state.value.intValue = value;
    state.timestamp = 0;  // Will be set when used
    return state;
}

MCP_ActuatorState MCP_ActuatorCreateFloatState(float value) {
    MCP_ActuatorState state;
    state.type = MCP_ACTUATOR_STATE_TYPE_FLOAT;
    state.value.floatValue = value;
    state.timestamp = 0;  // Will be set when used
    return state;
}

MCP_ActuatorState MCP_ActuatorCreateStringState(const char* value) {
    MCP_ActuatorState state;
    
    if (value != NULL) {
        state.type = MCP_ACTUATOR_STATE_TYPE_STRING;
        state.value.stringValue = strdup(value);
        
        if (state.value.stringValue == NULL) {
            state.type = MCP_ACTUATOR_STATE_TYPE_INT;
            state.value.intValue = 0;
        }
    } else {
        state.type = MCP_ACTUATOR_STATE_TYPE_STRING;
        state.value.stringValue = NULL;
    }
    
    state.timestamp = 0;  // Will be set when used
    return state;
}

void MCP_ActuatorFreeState(MCP_ActuatorState* state) {
    if (state == NULL) {
        return;
    }
    
    if (state->type == MCP_ACTUATOR_STATE_TYPE_STRING && state->value.stringValue != NULL) {
        free(state->value.stringValue);
        state->value.stringValue = NULL;
    }
}