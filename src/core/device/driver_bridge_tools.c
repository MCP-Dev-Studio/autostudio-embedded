#include "driver_bridge.h"
#include "driver_manager.h"
#include "../tool_system/tool_registry.h"
#include "../../json/json_helpers.h"
#include "../../driver/actuators/led/led.h"
#include "../../driver/sensors/temperature/ds18b20.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Forward declarations for tool handlers
static MCP_ToolResult registerNativeDriverHandler(const char* json, size_t length);
static MCP_ToolResult unregisterNativeDriverHandler(const char* json, size_t length);
static MCP_ToolResult listNativeDriversHandler(const char* json, size_t length);
static MCP_ToolResult executeNativeDriverFunctionHandler(const char* json, size_t length);

/**
 * @brief Initialize the driver bridge tool handlers
 */
int MCP_DriverBridgeToolsInit(void) {
    // Register system.registerNativeDriver tool
    const char* registerSchema = "{"
                               "\"name\":\"system.registerNativeDriver\","
                               "\"description\":\"Register a native hardware driver\","
                               "\"params\":{"
                               "\"properties\":{"
                               "\"id\":{\"type\":\"string\"},"
                               "\"name\":{\"type\":\"string\"},"
                               "\"type\":{\"type\":\"integer\",\"description\":\"Driver type (0=sensor, 1=actuator, 2=interface, 3=storage, 4=network, 5=custom)\"},"
                               "\"deviceType\":{\"type\":\"integer\",\"description\":\"Device specific type (e.g. LED, temperature sensor)\"},"
                               "\"configSchema\":{\"type\":\"object\"}"
                               "},"
                               "\"required\":[\"id\",\"deviceType\"]"
                               "}"
                               "}";

    MCP_ToolRegister("system.registerNativeDriver", registerNativeDriverHandler, registerSchema);

    // Register system.unregisterNativeDriver tool
    const char* unregisterSchema = "{"
                                 "\"name\":\"system.unregisterNativeDriver\","
                                 "\"description\":\"Unregister a native hardware driver\","
                                 "\"params\":{"
                                 "\"properties\":{"
                                 "\"id\":{\"type\":\"string\"}"
                                 "},"
                                 "\"required\":[\"id\"]"
                                 "}"
                                 "}";

    MCP_ToolRegister("system.unregisterNativeDriver", unregisterNativeDriverHandler, unregisterSchema);

    // Register system.listNativeDrivers tool
    const char* listSchema = "{"
                          "\"name\":\"system.listNativeDrivers\","
                          "\"description\":\"List available native drivers\","
                          "\"params\":{"
                          "\"properties\":{"
                          "\"type\":{\"type\":\"string\",\"description\":\"Filter by driver type (sensor, actuator, interface, storage, network, custom)\"}"
                          "}"
                          "}"
                          "}";

    MCP_ToolRegister("system.listNativeDrivers", listNativeDriversHandler, listSchema);

    // Register system.executeNativeDriverFunction tool
    const char* executeSchema = "{"
                              "\"name\":\"system.executeNativeDriverFunction\","
                              "\"description\":\"Execute a function on a native driver\","
                              "\"params\":{"
                              "\"properties\":{"
                              "\"id\":{\"type\":\"string\"},"
                              "\"function\":{\"type\":\"string\"},"
                              "\"args\":{\"type\":\"object\"}"
                              "},"
                              "\"required\":[\"id\",\"function\"]"
                              "}"
                              "}";

    MCP_ToolRegister("system.executeNativeDriverFunction", executeNativeDriverFunctionHandler, executeSchema);

    return 0;
}

/**
 * @brief Handler for system.registerNativeDriver tool
 */
static MCP_ToolResult registerNativeDriverHandler(const char* json, size_t length) {
    if (json == NULL || length == 0) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Invalid JSON");
    }

    // Extract parameters
    void* paramsObj = json_get_object_field(json, "params");
    if (paramsObj == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing params object");
    }

    char* id = json_get_string_field(paramsObj, "id");
    if (id == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing driver ID");
    }

    char* name = json_get_string_field(paramsObj, "name");
    // If name not provided, use ID as name
    if (name == NULL) {
        name = strdup(id);
        if (name == NULL) {
            free(id);
            return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, "Memory allocation failed");
        }
    }

    // Get driver type (default to custom if not specified)
    int type = json_get_int_field(paramsObj, "type", MCP_DRIVER_TYPE_CUSTOM);
    if (type < 0 || type > MCP_DRIVER_TYPE_CUSTOM) {
        type = MCP_DRIVER_TYPE_CUSTOM; // Default to custom type if invalid
    }

    // Get device type (required)
    int deviceType = json_get_int_field(paramsObj, "deviceType", -1);
    if (deviceType < 0) {
        free(id);
        free(name);
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, 
                                       "Missing or invalid device type");
    }

    // Get config schema (optional)
    char* configSchema = json_get_string_field(paramsObj, "configSchema");

    // Register driver with bridge
    int result = MCP_DriverBridgeRegister(id, name, (MCP_DriverType)type, deviceType, configSchema);

    // Free allocated strings
    free(id);
    free(name);
    if (configSchema != NULL) {
        free(configSchema);
    }

    // Return result
    if (result == 0) {
        return MCP_ToolCreateSuccessResult("{\"status\":\"success\"}");
    } else {
        char errorMsg[128];
        switch(result) {
            case -1:
                snprintf(errorMsg, sizeof(errorMsg), "System not initialized");
                break;
            case -2:
                snprintf(errorMsg, sizeof(errorMsg), "Driver already exists");
                break;
            case -3:
                snprintf(errorMsg, sizeof(errorMsg), "No space for new driver");
                break;
            case -4:
                snprintf(errorMsg, sizeof(errorMsg), "Memory allocation failed");
                break;
            default:
                snprintf(errorMsg, sizeof(errorMsg), "Failed to register driver (code: %d)", result);
                break;
        }
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, errorMsg);
    }
}

/**
 * @brief Handler for system.unregisterNativeDriver tool
 */
static MCP_ToolResult unregisterNativeDriverHandler(const char* json, size_t length) {
    if (json == NULL || length == 0) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Invalid JSON");
    }

    // Extract driver ID from params
    void* paramsObj = json_get_object_field(json, "params");
    if (paramsObj == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing params object");
    }

    char* id = json_get_string_field(paramsObj, "id");
    if (id == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing driver ID");
    }

    // Unregister the driver
    int result = MCP_DriverBridgeUnregister(id);
    free(id);

    if (result == 0) {
        return MCP_ToolCreateSuccessResult("{\"status\":\"success\"}");
    } else {
        char errorMsg[128];
        switch(result) {
            case -1:
                snprintf(errorMsg, sizeof(errorMsg), "System not initialized");
                break;
            case -2:
                snprintf(errorMsg, sizeof(errorMsg), "Driver not found");
                break;
            default:
                snprintf(errorMsg, sizeof(errorMsg), "Failed to unregister driver (code: %d)", result);
                break;
        }
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, errorMsg);
    }
}

/**
 * @brief Handler for system.listNativeDrivers tool
 * 
 * This will actually list all drivers registered through the bridge system,
 * which appear as bytecode drivers in the main driver system.
 */
static MCP_ToolResult listNativeDriversHandler(const char* json, size_t length) {
    // This function would access the bridge registry directly to list native drivers
    // For now, we'll just create a success result with example content
    
    // In a real implementation, this would query the bridge registry and
    // return information about all the registered native drivers
    
    const char* exampleResponse = "{"
                                "\"drivers\":["
                                "{"
                                "\"id\":\"led1\","
                                "\"name\":\"RGB LED\","
                                "\"type\":\"actuator\","
                                "\"deviceType\":1002,"
                                "\"initialized\":true,"
                                "\"bridgeType\":\"native\""
                                "},"
                                "{"
                                "\"id\":\"tempSensor1\","
                                "\"name\":\"DS18B20 Temperature Sensor\","
                                "\"type\":\"sensor\","
                                "\"deviceType\":2000,"
                                "\"initialized\":false,"
                                "\"bridgeType\":\"native\""
                                "}"
                                "]"
                                "}";
    
    return MCP_ToolCreateSuccessResult(exampleResponse);
}

/**
 * @brief Handler for system.executeNativeDriverFunction tool
 */
static MCP_ToolResult executeNativeDriverFunctionHandler(const char* json, size_t length) {
    if (json == NULL || length == 0) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Invalid JSON");
    }

    // Extract parameters
    void* paramsObj = json_get_object_field(json, "params");
    if (paramsObj == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing params object");
    }

    char* id = json_get_string_field(paramsObj, "id");
    if (id == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing driver ID");
    }

    char* function = json_get_string_field(paramsObj, "function");
    if (function == NULL) {
        free(id);
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing function name");
    }

    char* args = json_get_string_field(paramsObj, "args");
    // args can be null if no arguments are needed

    // In a real implementation, this would find the mapped function and call it
    // For now, we'll simulate a successful call for demonstration purposes
    
    // Define the default response
    char resultJson[1024] = "{\"status\":\"success\"}";
    
    // Simulate responses for specific functions
    if (strcmp(function, "readTemperature") == 0) {
        snprintf(resultJson, sizeof(resultJson), 
                "{\"value\":24.5,\"units\":\"C\"}");
    } else if (strcmp(function, "getState") == 0) {
        snprintf(resultJson, sizeof(resultJson), 
                "{\"state\":true}");
    } else if (strcmp(function, "getColor") == 0) {
        snprintf(resultJson, sizeof(resultJson), 
                "{\"color\":{\"r\":255,\"g\":100,\"b\":50}}");
    }

    // Clean up
    free(id);
    free(function);
    if (args != NULL) {
        free(args);
    }

    return MCP_ToolCreateSuccessResult(resultJson);
}

/**
 * @brief Example function to create an LED driver with the bridge system
 * 
 * This shows how to use the bridge API directly without going through the tool system
 */
int MCP_CreateLEDDriver(const char* id, const char* name, int type, uint8_t pin) {
    // Map LED type to bridge device type
    int deviceType;
    switch (type) {
        case LED_TYPE_SIMPLE:
            deviceType = DEVICE_TYPE_LED_SIMPLE;
            break;
        case LED_TYPE_PWM:
            deviceType = DEVICE_TYPE_LED_PWM;
            break;
        case LED_TYPE_RGB:
            deviceType = DEVICE_TYPE_LED_RGB;
            break;
        case LED_TYPE_RGBW:
            deviceType = DEVICE_TYPE_LED_RGBW;
            break;
        case LED_TYPE_ADDRESSABLE:
            deviceType = DEVICE_TYPE_LED_ADDRESSABLE;
            break;
        default:
            return -1; // Invalid type
    }

    // Create config schema for LED
    char configSchema[512];
    snprintf(configSchema, sizeof(configSchema),
           "{\"type\":\"object\","
           "\"properties\":{"
           "\"pin\":{\"type\":\"number\",\"description\":\"GPIO pin number\"},"
           "\"activeHigh\":{\"type\":\"boolean\",\"description\":\"Active high (true) or active low (false)\"},"
           "\"initialState\":{\"type\":\"boolean\",\"description\":\"Initial state (on/off)\"}"
           "},"
           "\"required\":[\"pin\"]"
           "}");

    // Register LED driver with bridge system
    int result = MCP_DriverBridgeRegister(id, name, MCP_DRIVER_TYPE_ACTUATOR, deviceType, configSchema);

    return result;
}

/**
 * @brief Example function to create a DS18B20 temperature sensor driver with the bridge system
 * 
 * This shows how to use the bridge API directly without going through the tool system
 */
int MCP_CreateDS18B20Driver(const char* id, const char* name, uint8_t pin) {
    // Create config schema for DS18B20
    char configSchema[512];
    snprintf(configSchema, sizeof(configSchema),
           "{\"type\":\"object\","
           "\"properties\":{"
           "\"pin\":{\"type\":\"number\",\"description\":\"OneWire pin number\"},"
           "\"address\":{\"type\":\"number\",\"description\":\"DS18B20 ROM address (0 for single device mode)\"},"
           "\"resolution\":{\"type\":\"number\",\"description\":\"Temperature resolution (0-3)\"},"
           "\"useCRC\":{\"type\":\"boolean\",\"description\":\"Enable CRC checking\"}"
           "},"
           "\"required\":[\"pin\"]"
           "}");

    // Register DS18B20 driver with bridge system
    int result = MCP_DriverBridgeRegister(id, name, MCP_DRIVER_TYPE_SENSOR, DEVICE_TYPE_TEMPERATURE_DS18B20, configSchema);

    return result;
}