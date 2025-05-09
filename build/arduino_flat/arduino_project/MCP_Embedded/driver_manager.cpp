#include "arduino_compat.h"
#include "driver_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declaration of JSON utility functions
extern bool json_validate_schema(const char* json, const char* schema);

// Internal driver storage
typedef struct {
    MCP_DriverInfo info;
    bool active;
} DriverEntry;

// Internal state
static DriverEntry* s_drivers = NULL;
static uint16_t s_maxDrivers = 0;
static uint16_t s_driverCount = 0;
static bool s_initialized = false;

int MCP_DriverManagerInit(uint16_t maxDrivers) {
    if (s_initialized) {
        return -1;  // Already initialized
    }
    
    // Allocate driver array
    s_drivers = (DriverEntry*)calloc(maxDrivers, sizeof(DriverEntry));
    if (s_drivers == NULL) {
        return -2;  // Memory allocation failed
    }
    
    s_maxDrivers = maxDrivers;
    s_driverCount = 0;
    s_initialized = true;
    
    return 0;
}

int MCP_DriverRegister(const MCP_DriverInfo* info) {
    if (!s_initialized || info == NULL || info->id == NULL) {
        return -1;
    }
    
    // Check if driver already exists
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active && strcmp(s_drivers[i].info.id, info->id) == 0) {
            return -2;  // Driver already registered
        }
    }
    
    // Find free slot
    uint16_t slot = UINT16_MAX;
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (!s_drivers[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == UINT16_MAX) {
        return -3;  // No free slots
    }
    
    // Copy driver info
    s_drivers[slot].info.id = strdup(info->id);
    if (s_drivers[slot].info.id == NULL) {
        return -4;  // Memory allocation failed
    }
    
    s_drivers[slot].info.name = info->name ? strdup(info->name) : NULL;
    s_drivers[slot].info.version = info->version ? strdup(info->version) : NULL;
    s_drivers[slot].info.type = info->type;
    
    // Copy interface functions
    memcpy(&s_drivers[slot].info.iface, &info->iface, sizeof(MCP_DriverInterface));
    
    s_drivers[slot].info.initialized = false;
    s_drivers[slot].info.configSchema = info->configSchema ? strdup(info->configSchema) : NULL;
    
    s_drivers[slot].active = true;
    s_driverCount++;
    
    return 0;
}

int MCP_DriverUnregister(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find driver
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active && strcmp(s_drivers[i].info.id, id) == 0) {
            // Deinitialize if initialized
            if (s_drivers[i].info.initialized && s_drivers[i].info.iface.deinit != NULL) {
                s_drivers[i].info.iface.deinit();
            }
            
            // Free strings
            free(s_drivers[i].info.id);
            if (s_drivers[i].info.name) free(s_drivers[i].info.name);
            if (s_drivers[i].info.version) free(s_drivers[i].info.version);
            if (s_drivers[i].info.configSchema) free(s_drivers[i].info.configSchema);
            
            // Mark as inactive
            s_drivers[i].active = false;
            s_driverCount--;
            
            return 0;
        }
    }
    
    return -2;  // Driver not found
}

const MCP_DriverInfo* MCP_DriverFind(const char* id) {
    if (!s_initialized || id == NULL) {
        return NULL;
    }
    
    // Find driver
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active && strcmp(s_drivers[i].info.id, id) == 0) {
            return &s_drivers[i].info;
        }
    }
    
    return NULL;  // Driver not found
}

int MCP_DriverGetByType(int type, const MCP_DriverInfo** drivers, uint16_t maxDrivers) {
    if (!s_initialized || drivers == NULL || maxDrivers == 0) {
        return -1;
    }
    
    uint16_t count = 0;
    
    // Find drivers of the specified type
    for (uint16_t i = 0; i < s_maxDrivers && count < maxDrivers; i++) {
        if (s_drivers[i].active && (type < 0 || (int)s_drivers[i].info.type == type)) {
            drivers[count++] = &s_drivers[i].info;
        }
    }
    
    return count;
}

int MCP_DriverInitialize(const char* id, const char* config, size_t configLength) {
    (void)configLength; // Unused parameter, might be used in future implementations
    
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find driver
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active && strcmp(s_drivers[i].info.id, id) == 0) {
            // Check if already initialized
            if (s_drivers[i].info.initialized) {
                return -2;  // Already initialized
            }
            
            // Validate configuration if schema exists
            if (s_drivers[i].info.configSchema != NULL && config != NULL) {
                if (!json_validate_schema(config, s_drivers[i].info.configSchema)) {
                    return -3;  // Invalid configuration
                }
            }
            
            // Call init function
            if (s_drivers[i].info.iface.init != NULL) {
                int result = s_drivers[i].info.iface.init(config);
                if (result == 0) {
                    s_drivers[i].info.initialized = true;
                }
                return result;
            } else {
                return -4;  // No init function
            }
        }
    }
    
    return -5;  // Driver not found
}

int MCP_DriverDeinitialize(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find driver
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active && strcmp(s_drivers[i].info.id, id) == 0) {
            // Check if initialized
            if (!s_drivers[i].info.initialized) {
                return -2;  // Not initialized
            }
            
            // Call deinit function
            if (s_drivers[i].info.iface.deinit != NULL) {
                int result = s_drivers[i].info.iface.deinit();
                if (result == 0) {
                    s_drivers[i].info.initialized = false;
                }
                return result;
            } else {
                s_drivers[i].info.initialized = false;
                return 0;
            }
        }
    }
    
    return -3;  // Driver not found
}

int MCP_DriverRead(const char* id, void* data, size_t maxSize, size_t* actualSize) {
    if (!s_initialized || id == NULL || data == NULL || maxSize == 0) {
        return -1;
    }
    
    // Find driver
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active && strcmp(s_drivers[i].info.id, id) == 0) {
            // Check if initialized
            if (!s_drivers[i].info.initialized) {
                return -2;  // Not initialized
            }
            
            // Call read function
            if (s_drivers[i].info.iface.read != NULL) {
                return s_drivers[i].info.iface.read(data, maxSize, actualSize);
            } else {
                return -3;  // No read function
            }
        }
    }
    
    return -4;  // Driver not found
}

int MCP_DriverWrite(const char* id, const void* data, size_t size) {
    if (!s_initialized || id == NULL || data == NULL) {
        return -1;
    }
    
    // Find driver
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active && strcmp(s_drivers[i].info.id, id) == 0) {
            // Check if initialized
            if (!s_drivers[i].info.initialized) {
                return -2;  // Not initialized
            }
            
            // Call write function
            if (s_drivers[i].info.iface.write != NULL) {
                return s_drivers[i].info.iface.write(data, size);
            } else {
                return -3;  // No write function
            }
        }
    }
    
    return -4;  // Driver not found
}

int MCP_DriverControl(const char* id, uint32_t command, void* arg) {
    if (!s_initialized || id == NULL) {
        return -1;
    }
    
    // Find driver
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active && strcmp(s_drivers[i].info.id, id) == 0) {
            // Check if initialized
            if (!s_drivers[i].info.initialized) {
                return -2;  // Not initialized
            }
            
            // Call control function
            if (s_drivers[i].info.iface.control != NULL) {
                return s_drivers[i].info.iface.control(command, arg);
            } else {
                return -3;  // No control function
            }
        }
    }
    
    return -4;  // Driver not found
}

int MCP_DriverGetStatus(const char* id, void* status, size_t maxSize) {
    if (!s_initialized || id == NULL || status == NULL) {
        return -1;
    }
    
    // Find driver
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active && strcmp(s_drivers[i].info.id, id) == 0) {
            // Check if initialized
            if (!s_drivers[i].info.initialized) {
                return -2;  // Not initialized
            }
            
            // Call getStatus function
            if (s_drivers[i].info.iface.getStatus != NULL) {
                return s_drivers[i].info.iface.getStatus(status, maxSize);
            } else {
                return -3;  // No getStatus function
            }
        }
    }
    
    return -4;  // Driver not found
}

int MCP_DriverExportConfig(char* buffer, size_t bufferSize) {
    if (!s_initialized || buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // Start JSON array
    int offset = 0;
    offset += snprintf(buffer + offset, bufferSize - offset, "[");
    
    // Add driver information
    bool first = true;
    for (uint16_t i = 0; i < s_maxDrivers; i++) {
        if (s_drivers[i].active) {
            // Add comma if not first driver
            if (!first) {
                offset += snprintf(buffer + offset, bufferSize - offset, ",");
            }
            first = false;
            
            // Add driver info
            offset += snprintf(buffer + offset, bufferSize - offset, 
                             "{"
                             "\"id\":\"%s\","
                             "\"name\":\"%s\","
                             "\"type\":%d,"
                             "\"initialized\":%s"
                             "}",
                             s_drivers[i].info.id,
                             s_drivers[i].info.name ? s_drivers[i].info.name : "",
                             s_drivers[i].info.type,
                             s_drivers[i].info.initialized ? "true" : "false");
            
            // Check if we're about to overflow
            if ((size_t)offset >= bufferSize - 2) {
                return -2;  // Buffer too small
            }
        }
    }
    
    // End JSON array
    offset += snprintf(buffer + offset, bufferSize - offset, "]");
    
    return offset;
}
