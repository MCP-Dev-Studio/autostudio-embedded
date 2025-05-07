#include "driver_bridge.h"
#include "driver_bytecode.h"
#include "driver_manager.h"
#include "../tool_system/bytecode_interpreter.h"
#include "../tool_system/context_manager.h"
#include "../tool_system/tool_registry.h"
#include "../../driver/actuators/led/led.h"
#include "../../driver/sensors/temperature/ds18b20.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Driver function type enumeration
 */
typedef enum {
    DRIVER_FUNC_INIT,
    DRIVER_FUNC_DEINIT,
    DRIVER_FUNC_READ,
    DRIVER_FUNC_WRITE,
    DRIVER_FUNC_CONTROL,
    DRIVER_FUNC_GET_STATUS,
    DRIVER_FUNC_CUSTOM,
    DRIVER_FUNC_COUNT
} DriverFunctionType;

/**
 * @brief Function mapping entry
 */
typedef struct {
    char functionName[32];  // Function name (e.g. "init", "read", "setBrightness")
    void* nativeFunction;   // Pointer to native function
    DriverFunctionType type; // Function type
} FunctionMapping;

/**
 * @brief Bridge driver entry
 */
typedef struct {
    char id[32];                      // Driver ID
    int deviceType;                   // Device specific type (LED, temperature, etc.)
    MCP_DriverType driverType;        // Driver type (sensor, actuator, etc.)
    int mappingCount;                 // Number of function mappings
    FunctionMapping* mappings;        // Array of function mappings
    void* driverData;                 // Driver-specific data
} BridgeDriverEntry;

// Bridge registry
static BridgeDriverEntry* s_bridgeDrivers = NULL;
static int s_maxBridgeDrivers = 0;
static int s_bridgeDriverCount = 0;
static bool s_initialized = false;

// Forward declarations for bytecode interface functions
static int bridgeDriverInit(const void* config);
static int bridgeDriverDeinit(void);
static int bridgeDriverRead(void* data, size_t maxSize, size_t* actualSize);
static int bridgeDriverWrite(const void* data, size_t size);
static int bridgeDriverControl(uint32_t command, void* arg);
static int bridgeDriverGetStatus(void* status, size_t maxSize);

// Forward declarations for mapping functions
static void* findMappedFunction(const char* driverId, const char* functionName);
static BridgeDriverEntry* findBridgeDriver(const char* id);
static int registerStandardBridgeFunctions(const char* id);

// Forward declarations for specific driver handlers
static int bridgeLEDDriver(const char* id, const char* name, int deviceType);
static int bridgeDS18B20Driver(const char* id, const char* name);

/**
 * @brief Initialize the driver bridge system
 */
int MCP_DriverBridgeInit(void) {
    if (s_initialized) {
        return 0;  // Already initialized
    }

    // Initialize MCP bytecode driver system
    if (MCP_BytecodeDriverInit() != 0) {
        return -1;  // Failed to initialize bytecode driver system
    }

    // Initialize bridge driver registry
    s_maxBridgeDrivers = 16;  // Start with space for 16 bridge drivers
    s_bridgeDrivers = (BridgeDriverEntry*)calloc(s_maxBridgeDrivers, sizeof(BridgeDriverEntry));
    if (s_bridgeDrivers == NULL) {
        return -2;  // Memory allocation failed
    }

    s_bridgeDriverCount = 0;
    s_initialized = true;

    // Register tools for bridge driver management
    // This would register tools like system.registerNativeDriver, etc.
    // For simplicity, we'll leave this for a full implementation

    return 0;
}

/**
 * @brief Register a standard driver with the bytecode driver system
 */
int MCP_DriverBridgeRegister(const char* id, const char* name, 
                            MCP_DriverType type, int deviceType,
                            const char* configSchema) {
    if (!s_initialized || id == NULL || name == NULL) {
        return -1;
    }

    // Check if driver already exists
    if (findBridgeDriver(id) != NULL) {
        return -2;  // Driver already exists
    }

    // Check if we have space
    if (s_bridgeDriverCount >= s_maxBridgeDrivers) {
        // Try to expand the registry
        int newMax = s_maxBridgeDrivers * 2;
        BridgeDriverEntry* newDrivers = (BridgeDriverEntry*)realloc(
            s_bridgeDrivers, newMax * sizeof(BridgeDriverEntry));
        
        if (newDrivers == NULL) {
            return -3;  // No space
        }

        s_bridgeDrivers = newDrivers;
        s_maxBridgeDrivers = newMax;
    }

    // Initialize bridge driver entry
    BridgeDriverEntry* entry = &s_bridgeDrivers[s_bridgeDriverCount++];
    strncpy(entry->id, id, sizeof(entry->id) - 1);
    entry->id[sizeof(entry->id) - 1] = '\0';
    entry->deviceType = deviceType;
    entry->driverType = type;
    entry->mappingCount = 0;
    entry->mappings = NULL;
    entry->driverData = NULL;

    // Allocate space for function mappings (start with 10)
    entry->mappings = (FunctionMapping*)calloc(10, sizeof(FunctionMapping));
    if (entry->mappings == NULL) {
        s_bridgeDriverCount--;
        return -4;  // Memory allocation failed
    }

    // Create JSON for bytecode driver
    char driverJson[2048];
    int offset = 0;

    // Build driver definition JSON
    offset += snprintf(driverJson + offset, sizeof(driverJson) - offset,
                     "{\"tool\":\"system.defineDriver\",\"params\":{"
                     "\"id\":\"%s\","
                     "\"name\":\"%s\","
                     "\"version\":\"1.0.0\","
                     "\"type\":%d,",
                     id, name, (int)type);

    // Add standard bytecode implementation (placeholder)
    offset += snprintf(driverJson + offset, sizeof(driverJson) - offset,
                     "\"implementation\":{"
                     "\"init\":{\"instructions\":[{\"op\":\"PUSH_NUM\",\"value\":0},{\"op\":\"HALT\"}]},"
                     "\"deinit\":{\"instructions\":[{\"op\":\"PUSH_NUM\",\"value\":0},{\"op\":\"HALT\"}]},"
                     "\"read\":{\"instructions\":[{\"op\":\"PUSH_STR\",\"index\":0},{\"op\":\"HALT\"}],\"stringPool\":[\"{}\"]},"
                     "\"write\":{\"instructions\":[{\"op\":\"PUSH_NUM\",\"value\":0},{\"op\":\"HALT\"}]},"
                     "\"control\":{\"instructions\":[{\"op\":\"PUSH_NUM\",\"value\":0},{\"op\":\"HALT\"}]},"
                     "\"getStatus\":{\"instructions\":[{\"op\":\"PUSH_STR\",\"index\":0},{\"op\":\"HALT\"}],\"stringPool\":[\"{\\\"status\\\":\\\"ready\\\"}\"]}");

    // Add config schema if provided
    if (configSchema != NULL) {
        offset += snprintf(driverJson + offset, sizeof(driverJson) - offset,
                         "},\"configSchema\":%s}", configSchema);
    } else {
        offset += snprintf(driverJson + offset, sizeof(driverJson) - offset,
                         "},\"configSchema\":{\"type\":\"object\",\"properties\":{}}}");
    }

    // Register with bytecode driver system
    int result = MCP_BytecodeDriverRegister(driverJson, strlen(driverJson));
    if (result != 0) {
        free(entry->mappings);
        s_bridgeDriverCount--;
        return result;
    }

    // Register standard bridge functions
    registerStandardBridgeFunctions(id);

    // Handle specific driver types
    switch (deviceType) {
        case DEVICE_TYPE_LED_SIMPLE:
        case DEVICE_TYPE_LED_PWM:
        case DEVICE_TYPE_LED_RGB:
        case DEVICE_TYPE_LED_RGBW:
        case DEVICE_TYPE_LED_ADDRESSABLE:
            bridgeLEDDriver(id, name, deviceType);
            break;

        case DEVICE_TYPE_TEMPERATURE_DS18B20:
            bridgeDS18B20Driver(id, name);
            break;

        default:
            // For other device types, we would add specific mappings here
            break;
    }

    return 0;
}

/**
 * @brief Unregister a standard driver
 */
int MCP_DriverBridgeUnregister(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }

    // Find bridge driver
    int index = -1;
    for (int i = 0; i < s_bridgeDriverCount; i++) {
        if (strcmp(s_bridgeDrivers[i].id, id) == 0) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        return -2;  // Driver not found
    }

    // Unregister from bytecode driver system
    int result = MCP_BytecodeDriverUnregister(id);
    if (result != 0) {
        return result;
    }

    // Free driver resources
    free(s_bridgeDrivers[index].mappings);
    if (s_bridgeDrivers[index].driverData != NULL) {
        free(s_bridgeDrivers[index].driverData);
    }

    // Remove from registry by shifting remaining drivers
    for (int i = index; i < s_bridgeDriverCount - 1; i++) {
        memcpy(&s_bridgeDrivers[i], &s_bridgeDrivers[i + 1], sizeof(BridgeDriverEntry));
    }
    memset(&s_bridgeDrivers[s_bridgeDriverCount - 1], 0, sizeof(BridgeDriverEntry));
    s_bridgeDriverCount--;

    return 0;
}

/**
 * @brief Map bytecode function to native driver function
 */
int MCP_DriverBridgeMapFunction(const char* driverId, const char* function, void* nativeFunction) {
    if (!s_initialized || driverId == NULL || function == NULL || nativeFunction == NULL) {
        return -1;
    }

    // Find bridge driver
    BridgeDriverEntry* driver = findBridgeDriver(driverId);
    if (driver == NULL) {
        return -2;  // Driver not found
    }

    // Check if function already mapped
    for (int i = 0; i < driver->mappingCount; i++) {
        if (strcmp(driver->mappings[i].functionName, function) == 0) {
            // Update existing mapping
            driver->mappings[i].nativeFunction = nativeFunction;
            return 0;
        }
    }

    // Add new mapping
    if (driver->mappingCount >= 10) {
        // Resize mappings array
        int newSize = driver->mappingCount * 2;
        FunctionMapping* newMappings = (FunctionMapping*)realloc(
            driver->mappings, newSize * sizeof(FunctionMapping));
        
        if (newMappings == NULL) {
            return -3;  // Memory allocation failed
        }

        driver->mappings = newMappings;
    }

    // Add the new mapping
    strncpy(driver->mappings[driver->mappingCount].functionName, function, 
            sizeof(driver->mappings[driver->mappingCount].functionName) - 1);
    driver->mappings[driver->mappingCount].functionName[
        sizeof(driver->mappings[driver->mappingCount].functionName) - 1] = '\0';
    
    driver->mappings[driver->mappingCount].nativeFunction = nativeFunction;
    
    // Determine function type
    if (strcmp(function, "init") == 0) {
        driver->mappings[driver->mappingCount].type = DRIVER_FUNC_INIT;
    } else if (strcmp(function, "deinit") == 0) {
        driver->mappings[driver->mappingCount].type = DRIVER_FUNC_DEINIT;
    } else if (strcmp(function, "read") == 0) {
        driver->mappings[driver->mappingCount].type = DRIVER_FUNC_READ;
    } else if (strcmp(function, "write") == 0) {
        driver->mappings[driver->mappingCount].type = DRIVER_FUNC_WRITE;
    } else if (strcmp(function, "control") == 0) {
        driver->mappings[driver->mappingCount].type = DRIVER_FUNC_CONTROL;
    } else if (strcmp(function, "getStatus") == 0) {
        driver->mappings[driver->mappingCount].type = DRIVER_FUNC_GET_STATUS;
    } else {
        driver->mappings[driver->mappingCount].type = DRIVER_FUNC_CUSTOM;
    }

    driver->mappingCount++;
    return 0;
}

/**
 * @brief Find a bridge driver by ID
 */
static BridgeDriverEntry* findBridgeDriver(const char* id) {
    if (!s_initialized || id == NULL) {
        return NULL;
    }

    for (int i = 0; i < s_bridgeDriverCount; i++) {
        if (strcmp(s_bridgeDrivers[i].id, id) == 0) {
            return &s_bridgeDrivers[i];
        }
    }

    return NULL;
}

/**
 * @brief Find a mapped function by driver ID and function name
 */
static void* findMappedFunction(const char* driverId, const char* functionName) {
    BridgeDriverEntry* driver = findBridgeDriver(driverId);
    if (driver == NULL) {
        return NULL;
    }

    for (int i = 0; i < driver->mappingCount; i++) {
        if (strcmp(driver->mappings[i].functionName, functionName) == 0) {
            return driver->mappings[i].nativeFunction;
        }
    }

    return NULL;
}

/**
 * @brief Register standard bridge functions for a driver
 */
static int registerStandardBridgeFunctions(const char* id) {
    // Map the standard driver interface functions
    MCP_DriverBridgeMapFunction(id, "init", bridgeDriverInit);
    MCP_DriverBridgeMapFunction(id, "deinit", bridgeDriverDeinit);
    MCP_DriverBridgeMapFunction(id, "read", bridgeDriverRead);
    MCP_DriverBridgeMapFunction(id, "write", bridgeDriverWrite);
    MCP_DriverBridgeMapFunction(id, "control", bridgeDriverControl);
    MCP_DriverBridgeMapFunction(id, "getStatus", bridgeDriverGetStatus);

    return 0;
}

/**
 * @brief Bridge implementation for LED driver
 */
static int bridgeLEDDriver(const char* id, const char* name, int deviceType) {
    // Map LED functions to bridge functions based on device type
    switch(deviceType) {
        case DEVICE_TYPE_LED_SIMPLE:
            // Map simple LED functions
            MCP_DriverBridgeMapFunction(id, "init", LED_Init);
            MCP_DriverBridgeMapFunction(id, "deinit", LED_Deinit);
            MCP_DriverBridgeMapFunction(id, "on", LED_On);
            MCP_DriverBridgeMapFunction(id, "off", LED_Off);
            MCP_DriverBridgeMapFunction(id, "toggle", LED_Toggle);
            MCP_DriverBridgeMapFunction(id, "setState", LED_SetState);
            MCP_DriverBridgeMapFunction(id, "getState", LED_GetState);
            break;

        case DEVICE_TYPE_LED_PWM:
            // Map PWM LED functions (includes all simple LED functions)
            MCP_DriverBridgeMapFunction(id, "init", LED_Init);
            MCP_DriverBridgeMapFunction(id, "deinit", LED_Deinit);
            MCP_DriverBridgeMapFunction(id, "on", LED_On);
            MCP_DriverBridgeMapFunction(id, "off", LED_Off);
            MCP_DriverBridgeMapFunction(id, "toggle", LED_Toggle);
            MCP_DriverBridgeMapFunction(id, "setState", LED_SetState);
            MCP_DriverBridgeMapFunction(id, "getState", LED_GetState);
            MCP_DriverBridgeMapFunction(id, "setBrightness", LED_SetBrightness);
            MCP_DriverBridgeMapFunction(id, "getBrightness", LED_GetBrightness);
            break;

        case DEVICE_TYPE_LED_RGB:
        case DEVICE_TYPE_LED_RGBW:
            // Map RGB/RGBW LED functions (includes simple + PWM + color functions)
            MCP_DriverBridgeMapFunction(id, "init", LED_Init);
            MCP_DriverBridgeMapFunction(id, "deinit", LED_Deinit);
            MCP_DriverBridgeMapFunction(id, "on", LED_On);
            MCP_DriverBridgeMapFunction(id, "off", LED_Off);
            MCP_DriverBridgeMapFunction(id, "toggle", LED_Toggle);
            MCP_DriverBridgeMapFunction(id, "setState", LED_SetState);
            MCP_DriverBridgeMapFunction(id, "getState", LED_GetState);
            MCP_DriverBridgeMapFunction(id, "setBrightness", LED_SetBrightness);
            MCP_DriverBridgeMapFunction(id, "getBrightness", LED_GetBrightness);
            MCP_DriverBridgeMapFunction(id, "setColor", LED_SetColor);
            MCP_DriverBridgeMapFunction(id, "getColor", LED_GetColor);
            MCP_DriverBridgeMapFunction(id, "setPattern", LED_SetPattern);
            MCP_DriverBridgeMapFunction(id, "stopPattern", LED_StopPattern);
            break;

        case DEVICE_TYPE_LED_ADDRESSABLE:
            // Map addressable LED functions (all of the above + pixel functions)
            MCP_DriverBridgeMapFunction(id, "init", LED_Init);
            MCP_DriverBridgeMapFunction(id, "deinit", LED_Deinit);
            MCP_DriverBridgeMapFunction(id, "on", LED_On);
            MCP_DriverBridgeMapFunction(id, "off", LED_Off);
            MCP_DriverBridgeMapFunction(id, "toggle", LED_Toggle);
            MCP_DriverBridgeMapFunction(id, "setState", LED_SetState);
            MCP_DriverBridgeMapFunction(id, "getState", LED_GetState);
            MCP_DriverBridgeMapFunction(id, "setBrightness", LED_SetBrightness);
            MCP_DriverBridgeMapFunction(id, "getBrightness", LED_GetBrightness);
            MCP_DriverBridgeMapFunction(id, "setColor", LED_SetColor);
            MCP_DriverBridgeMapFunction(id, "getColor", LED_GetColor);
            MCP_DriverBridgeMapFunction(id, "setPattern", LED_SetPattern);
            MCP_DriverBridgeMapFunction(id, "stopPattern", LED_StopPattern);
            MCP_DriverBridgeMapFunction(id, "setPixel", LED_SetPixel);
            MCP_DriverBridgeMapFunction(id, "update", LED_Update);
            MCP_DriverBridgeMapFunction(id, "fill", LED_Fill);
            break;

        default:
            // Unknown LED type
            return -1;
    }

    return 0;
}

/**
 * @brief Bridge implementation for DS18B20 temperature sensor driver
 */
static int bridgeDS18B20Driver(const char* id, const char* name) {
    // Map DS18B20 functions to bridge functions
    MCP_DriverBridgeMapFunction(id, "init", DS18B20_Init);
    MCP_DriverBridgeMapFunction(id, "deinit", DS18B20_Deinit);
    MCP_DriverBridgeMapFunction(id, "startConversion", DS18B20_StartConversion);
    MCP_DriverBridgeMapFunction(id, "readTemperature", DS18B20_ReadTemperature);
    MCP_DriverBridgeMapFunction(id, "setResolution", DS18B20_SetResolution);
    MCP_DriverBridgeMapFunction(id, "readPowerSupply", DS18B20_ReadPowerSupply);
    MCP_DriverBridgeMapFunction(id, "getConversionTime", DS18B20_GetConversionTime);
    MCP_DriverBridgeMapFunction(id, "scanDevices", DS18B20_ScanDevices);
    MCP_DriverBridgeMapFunction(id, "selectDevice", DS18B20_SelectDevice);
    MCP_DriverBridgeMapFunction(id, "readAddress", DS18B20_ReadAddress);
    MCP_DriverBridgeMapFunction(id, "readScratchpad", DS18B20_ReadScratchpad);
    MCP_DriverBridgeMapFunction(id, "writeScratchpad", DS18B20_WriteScratchpad);
    MCP_DriverBridgeMapFunction(id, "copyScratchpad", DS18B20_CopyScratchpad);
    MCP_DriverBridgeMapFunction(id, "recallEEPROM", DS18B20_RecallEEPROM);

    return 0;
}

// Driver interface functions that forward to the actual mapped functions

/**
 * @brief Bridge initialization function
 */
static int bridgeDriverInit(const void* config) {
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        return -1;
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        return -2;
    }

    // Find init function
    typedef int (*InitFuncType)(const void*);
    InitFuncType initFunc = (InitFuncType)findMappedFunction(driverId, "init");
    if (initFunc == NULL) {
        return -3;
    }

    // Call the native init function
    return initFunc(config);
}

/**
 * @brief Bridge deinitialization function
 */
static int bridgeDriverDeinit(void) {
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        return -1;
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        return -2;
    }

    // Find deinit function
    typedef int (*DeinitFuncType)(void);
    DeinitFuncType deinitFunc = (DeinitFuncType)findMappedFunction(driverId, "deinit");
    if (deinitFunc == NULL) {
        return -3;
    }

    // Call the native deinit function
    return deinitFunc();
}

/**
 * @brief Bridge read function
 */
static int bridgeDriverRead(void* data, size_t maxSize, size_t* actualSize) {
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        return -1;
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        return -2;
    }

    // Find bridge driver
    BridgeDriverEntry* driver = findBridgeDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Handle reads differently based on driver type
    switch (driver->deviceType) {
        case DEVICE_TYPE_TEMPERATURE_DS18B20: {
            // For DS18B20, read function should return temperature as JSON
            typedef int (*ReadTempFuncType)(float*);
            ReadTempFuncType readTempFunc = (ReadTempFuncType)findMappedFunction(driverId, "readTemperature");
            if (readTempFunc == NULL) {
                return -4;
            }

            float temperature;
            int result = readTempFunc(&temperature);
            if (result != 0) {
                return result;
            }

            // Format temperature as JSON
            int written = snprintf(data, maxSize, "{\"value\":%.2f,\"units\":\"C\"}", temperature);
            if (actualSize != NULL) {
                *actualSize = written;
            }
            return 0;
        }

        // Add other device types here as needed
        default: {
            // For generic drivers, use standard read function
            typedef int (*ReadFuncType)(void*, size_t, size_t*);
            ReadFuncType readFunc = (ReadFuncType)findMappedFunction(driverId, "read");
            if (readFunc == NULL) {
                return -4;
            }

            // Call the native read function
            return readFunc(data, maxSize, actualSize);
        }
    }
}

/**
 * @brief Bridge write function
 */
static int bridgeDriverWrite(const void* data, size_t size) {
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        return -1;
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        return -2;
    }

    // Find bridge driver
    BridgeDriverEntry* driver = findBridgeDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Handle writes differently based on driver type
    switch (driver->deviceType) {
        case DEVICE_TYPE_LED_SIMPLE:
        case DEVICE_TYPE_LED_PWM:
        case DEVICE_TYPE_LED_RGB:
        case DEVICE_TYPE_LED_RGBW:
        case DEVICE_TYPE_LED_ADDRESSABLE: {
            // For LEDs, write could be used to set state, brightness, or color
            // Parse JSON data to determine what's being set
            const char* jsonData = (const char*)data;
            
            // Check for state setting
            if (strstr(jsonData, "\"state\"") != NULL) {
                // Extract state value
                bool state = false;
                if (strstr(jsonData, "\"state\":true") != NULL || 
                    strstr(jsonData, "\"state\":1") != NULL) {
                    state = true;
                }
                
                // Find setState function
                typedef int (*SetStateFuncType)(bool);
                SetStateFuncType setStateFunc = (SetStateFuncType)findMappedFunction(driverId, "setState");
                if (setStateFunc != NULL) {
                    return setStateFunc(state);
                }
            } 
            
            // Check for brightness setting
            else if (strstr(jsonData, "\"brightness\"") != NULL) {
                // Extract brightness value (simplified parsing)
                uint8_t brightness = 0;
                // In real implementation, would parse JSON properly
                sscanf(jsonData, "{\"brightness\":%hhu}", &brightness);
                
                // Find setBrightness function
                typedef int (*SetBrightnessFuncType)(uint8_t);
                SetBrightnessFuncType setBrightnessFunc = 
                    (SetBrightnessFuncType)findMappedFunction(driverId, "setBrightness");
                if (setBrightnessFunc != NULL) {
                    return setBrightnessFunc(brightness);
                }
            }
            
            // Check for color setting
            else if (strstr(jsonData, "\"color\"") != NULL) {
                // Simple color parsing (would need better parsing in real implementation)
                RGBColor color = {0};
                // In real implementation, would parse JSON properly
                sscanf(jsonData, "{\"color\":{\"r\":%hhu,\"g\":%hhu,\"b\":%hhu}}",
                       &color.r, &color.g, &color.b);
                
                // Find setColor function
                typedef int (*SetColorFuncType)(const RGBColor*);
                SetColorFuncType setColorFunc = 
                    (SetColorFuncType)findMappedFunction(driverId, "setColor");
                if (setColorFunc != NULL) {
                    return setColorFunc(&color);
                }
            }
            
            // Default to standard write function
            break;
        }
        
        // Add other device types here as needed
    }

    // For all other cases or fallbacks, use standard write function
    typedef int (*WriteFuncType)(const void*, size_t);
    WriteFuncType writeFunc = (WriteFuncType)findMappedFunction(driverId, "write");
    if (writeFunc == NULL) {
        return -4;
    }

    // Call the native write function
    return writeFunc(data, size);
}

/**
 * @brief Bridge control function
 */
static int bridgeDriverControl(uint32_t command, void* arg) {
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        return -1;
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        return -2;
    }

    // Find bridge driver
    BridgeDriverEntry* driver = findBridgeDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Handle controls differently based on driver type and command
    switch (driver->deviceType) {
        case DEVICE_TYPE_LED_SIMPLE:
        case DEVICE_TYPE_LED_PWM:
        case DEVICE_TYPE_LED_RGB:
        case DEVICE_TYPE_LED_RGBW:
        case DEVICE_TYPE_LED_ADDRESSABLE:
            // LED-specific commands
            switch (command) {
                case 1: { // Turn on
                    typedef int (*OnFuncType)(void);
                    OnFuncType onFunc = (OnFuncType)findMappedFunction(driverId, "on");
                    if (onFunc != NULL) {
                        return onFunc();
                    }
                    break;
                }
                case 2: { // Turn off
                    typedef int (*OffFuncType)(void);
                    OffFuncType offFunc = (OffFuncType)findMappedFunction(driverId, "off");
                    if (offFunc != NULL) {
                        return offFunc();
                    }
                    break;
                }
                case 3: { // Toggle
                    typedef int (*ToggleFuncType)(void);
                    ToggleFuncType toggleFunc = (ToggleFuncType)findMappedFunction(driverId, "toggle");
                    if (toggleFunc != NULL) {
                        return toggleFunc();
                    }
                    break;
                }
                // Add more LED commands as needed
            }
            break;

        case DEVICE_TYPE_TEMPERATURE_DS18B20:
            // DS18B20-specific commands
            switch (command) {
                case 1: { // Start conversion
                    typedef int (*StartConvFuncType)(void);
                    StartConvFuncType startConvFunc = 
                        (StartConvFuncType)findMappedFunction(driverId, "startConversion");
                    if (startConvFunc != NULL) {
                        return startConvFunc();
                    }
                    break;
                }
                case 2: { // Set resolution
                    if (arg != NULL) {
                        uint8_t resolution = *((uint8_t*)arg);
                        typedef int (*SetResFuncType)(DS18B20Resolution);
                        SetResFuncType setResFunc = 
                            (SetResFuncType)findMappedFunction(driverId, "setResolution");
                        if (setResFunc != NULL) {
                            return setResFunc((DS18B20Resolution)resolution);
                        }
                    }
                    break;
                }
                // Add more DS18B20 commands as needed
            }
            break;

        // Add other device types here as needed
    }

    // For all other cases or fallbacks, use standard control function
    typedef int (*ControlFuncType)(uint32_t, void*);
    ControlFuncType controlFunc = (ControlFuncType)findMappedFunction(driverId, "control");
    if (controlFunc == NULL) {
        return -4;
    }

    // Call the native control function
    return controlFunc(command, arg);
}

/**
 * @brief Bridge get status function
 */
static int bridgeDriverGetStatus(void* status, size_t maxSize) {
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        return -1;
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        return -2;
    }

    // Find bridge driver
    BridgeDriverEntry* driver = findBridgeDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Handle status differently based on driver type
    switch (driver->deviceType) {
        case DEVICE_TYPE_LED_SIMPLE:
        case DEVICE_TYPE_LED_PWM:
        case DEVICE_TYPE_LED_RGB:
        case DEVICE_TYPE_LED_RGBW:
        case DEVICE_TYPE_LED_ADDRESSABLE: {
            // For LED, status includes state and possibly brightness/color
            typedef bool (*GetStateFuncType)(void);
            GetStateFuncType getStateFunc = (GetStateFuncType)findMappedFunction(driverId, "getState");
            
            // Start with basic status
            int offset = 0;
            char* statusStr = (char*)status;
            offset += snprintf(statusStr + offset, maxSize - offset, "{\"deviceType\":\"LED\",");
            
            // Add state if available
            if (getStateFunc != NULL) {
                bool state = getStateFunc();
                offset += snprintf(statusStr + offset, maxSize - offset, 
                                 "\"state\":%s,", state ? "true" : "false");
            }
            
            // Add brightness if available
            typedef uint8_t (*GetBrightnessFuncType)(void);
            GetBrightnessFuncType getBrightnessFunc = 
                (GetBrightnessFuncType)findMappedFunction(driverId, "getBrightness");
            if (getBrightnessFunc != NULL) {
                uint8_t brightness = getBrightnessFunc();
                offset += snprintf(statusStr + offset, maxSize - offset,
                                 "\"brightness\":%u,", brightness);
            }
            
            // Add color if available for RGB/RGBW LEDs
            if (driver->deviceType == DEVICE_TYPE_LED_RGB ||
                driver->deviceType == DEVICE_TYPE_LED_RGBW) {
                typedef int (*GetColorFuncType)(RGBColor*);
                GetColorFuncType getColorFunc = 
                    (GetColorFuncType)findMappedFunction(driverId, "getColor");
                if (getColorFunc != NULL) {
                    RGBColor color;
                    if (getColorFunc(&color) == 0) {
                        offset += snprintf(statusStr + offset, maxSize - offset,
                                         "\"color\":{\"r\":%u,\"g\":%u,\"b\":%u",
                                         color.r, color.g, color.b);
                        
                        if (driver->deviceType == DEVICE_TYPE_LED_RGBW) {
                            offset += snprintf(statusStr + offset, maxSize - offset,
                                             ",\"w\":%u},", color.w);
                        } else {
                            offset += snprintf(statusStr + offset, maxSize - offset, "},");
                        }
                    }
                }
            }
            
            // Finish JSON (remove trailing comma if exists)
            if (offset > 0 && statusStr[offset - 1] == ',') {
                offset--;
            }
            snprintf(statusStr + offset, maxSize - offset, "}");
            
            return 0;
        }
        
        case DEVICE_TYPE_TEMPERATURE_DS18B20: {
            // For temperature sensor, include temperature and device details
            typedef int (*ReadTempFuncType)(float*);
            ReadTempFuncType readTempFunc = 
                (ReadTempFuncType)findMappedFunction(driverId, "readTemperature");
            
            // Start with basic status
            int offset = 0;
            char* statusStr = (char*)status;
            offset += snprintf(statusStr + offset, maxSize - offset, 
                             "{\"deviceType\":\"DS18B20\",");
            
            // Add temperature if available
            if (readTempFunc != NULL) {
                float temperature;
                if (readTempFunc(&temperature) == 0) {
                    offset += snprintf(statusStr + offset, maxSize - offset,
                                     "\"temperature\":%.2f,\"units\":\"C\",", temperature);
                }
            }
            
            // Add resolution if available
            typedef uint32_t (*GetConvTimeFuncType)(void);
            GetConvTimeFuncType getConvTimeFunc = 
                (GetConvTimeFuncType)findMappedFunction(driverId, "getConversionTime");
            if (getConvTimeFunc != NULL) {
                uint32_t convTime = getConvTimeFunc();
                // Map conversion time to resolution
                uint8_t resolution;
                if (convTime <= 100) resolution = 9;  // 9-bit
                else if (convTime <= 200) resolution = 10; // 10-bit
                else if (convTime <= 400) resolution = 11; // 11-bit
                else resolution = 12; // 12-bit
                
                offset += snprintf(statusStr + offset, maxSize - offset,
                                 "\"resolution\":%u,", resolution);
            }
            
            // Finish JSON (remove trailing comma if exists)
            if (offset > 0 && statusStr[offset - 1] == ',') {
                offset--;
            }
            snprintf(statusStr + offset, maxSize - offset, "}");
            
            return 0;
        }
        
        // Add other device types here as needed
    }

    // For all other cases or fallbacks, use standard getStatus function
    typedef int (*GetStatusFuncType)(void*, size_t);
    GetStatusFuncType getStatusFunc = (GetStatusFuncType)findMappedFunction(driverId, "getStatus");
    if (getStatusFunc == NULL) {
        // Default status if no function found
        snprintf(status, maxSize, "{\"status\":\"unknown\"}");
        return 0;
    }

    // Call the native getStatus function
    return getStatusFunc(status, maxSize);
}