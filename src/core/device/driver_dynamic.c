#include "driver_dynamic.h"
#include "driver_manager.h"
#include "../tool_system/context_manager.h"
#include "../tool_system/tool_registry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// External JSON utility functions
extern char* json_get_string_field(const char* json, const char* field);
extern bool json_validate_schema(const char* json, const char* schema);
extern void* json_get_array_field(const char* json, const char* field);
extern void* json_get_object_field(const char* json, const char* field);
extern int json_array_length(const void* array);
extern void* json_array_get_object(const void* array, size_t index);
extern bool json_get_bool_field(const char* json, const char* field, bool defaultValue);
extern int json_get_int_field(const char* json, const char* field, int defaultValue);

// External persistence functions
extern int persistent_storage_write(const char* key, const void* data, size_t size);
extern int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize);
extern int persistent_storage_get_keys(char** keys, size_t maxKeys);

// JavaScript interpreter functions (minimal bindings)
extern int js_init(void);
extern int js_eval(const char* script, size_t scriptLength, char* result, size_t maxResultLen);
extern int js_call_function(const char* moduleName, const char* funcName, const char* params, 
                          char* result, size_t maxResultLen);
extern int js_register_native_function(const char* name, void* funcPtr);
extern int js_create_module(const char* moduleName, const char* script, size_t scriptLength);
extern int js_delete_module(const char* moduleName);

// Maximum script size for dynamic drivers
#define MAX_DRIVER_SCRIPT_SIZE (16 * 1024)

// Script environment for dynamic drivers
static bool s_jsInitialized = false;

// Dynamic driver implementation structure
typedef struct {
    char* id;                       // Driver ID
    char* name;                     // Driver name
    char* version;                  // Driver version
    MCP_DriverType type;            // Driver type
    char* script;                   // JS implementation script
    size_t scriptLength;            // Script length
    bool initialized;               // Init state
    char* configSchema;             // JSON schema for config
    bool persistent;                // Persistence flag
} DynamicDriverDefinition;

// Dynamic driver registry
static DynamicDriverDefinition** s_dynamicDrivers = NULL;
static int s_maxDynamicDrivers = 0;
static int s_dynamicDriverCount = 0;
static bool s_initialized = false;

// Forward declarations for driver interface functions
static int dynamicDriverInit(const void* config);
static int dynamicDriverDeinit(void);
static int dynamicDriverRead(void* data, size_t maxSize, size_t* actualSize);
static int dynamicDriverWrite(const void* data, size_t size);
static int dynamicDriverControl(uint32_t command, void* arg);
static int dynamicDriverGetStatus(void* status, size_t maxSize);

// Tool handlers
MCP_ToolResult defineDriverHandler(const char* json, size_t length);
MCP_ToolResult listDriversHandler(const char* json, size_t length);
MCP_ToolResult removeDriverHandler(const char* json, size_t length);
MCP_ToolResult executeDriverFunctionHandler(const char* json, size_t length);

// Get driver context ID for thread-local storage
static const char* getDriverContextId(const char* driverId) {
    static char contextId[64];
    snprintf(contextId, sizeof(contextId), "driver_%s", driverId);
    return contextId;
}

int MCP_DynamicDriverInit(void) {
    if (s_initialized) {
        return 0;  // Already initialized
    }

    // Initialize JavaScript environment
    if (!s_jsInitialized) {
        if (js_init() != 0) {
            return -1;  // Failed to initialize JavaScript
        }
        s_jsInitialized = true;
    }

    // Initialize dynamic driver registry
    s_maxDynamicDrivers = 16;  // Start with space for 16 dynamic drivers
    s_dynamicDrivers = (DynamicDriverDefinition**)calloc(s_maxDynamicDrivers, 
                                                      sizeof(DynamicDriverDefinition*));
    if (s_dynamicDrivers == NULL) {
        return -2;  // Memory allocation failed
    }

    s_dynamicDriverCount = 0;
    s_initialized = true;

    // Register system.defineDriver tool
    const char* defineDriverSchema = "{"
                                   "\"name\":\"system.defineDriver\","
                                   "\"description\":\"Define a new driver dynamically\","
                                   "\"params\":{"
                                   "\"properties\":{"
                                   "\"id\":{\"type\":\"string\"},"
                                   "\"name\":{\"type\":\"string\"},"
                                   "\"version\":{\"type\":\"string\"},"
                                   "\"type\":{\"type\":\"integer\",\"description\":\"Driver type (0=sensor, 1=actuator, 2=interface, 3=storage, 4=network, 5=custom)\"},"
                                   "\"implementation\":{\"type\":\"object\",\"properties\":{"
                                   "\"script\":{\"type\":\"string\"}"
                                   "}},"
                                   "\"configSchema\":{\"type\":\"object\"},"
                                   "\"persistent\":{\"type\":\"boolean\"}"
                                   "},"
                                   "\"required\":[\"id\",\"implementation\"]"
                                   "}"
                                   "}";

    MCP_ToolRegister_Legacy("system.defineDriver", defineDriverHandler, defineDriverSchema);

    // Register system.listDrivers tool
    const char* listDriversSchema = "{"
                                  "\"name\":\"system.listDrivers\","
                                  "\"description\":\"List available drivers\","
                                  "\"params\":{"
                                  "\"properties\":{"
                                  "\"type\":{\"type\":\"string\",\"description\":\"Filter by driver type (sensor, actuator, interface, storage, network, custom)\"}"
                                  "}"
                                  "}"
                                  "}";

    MCP_ToolRegister_Legacy("system.listDrivers", listDriversHandler, listDriversSchema);

    // Register system.removeDriver tool
    const char* removeDriverSchema = "{"
                                   "\"name\":\"system.removeDriver\","
                                   "\"description\":\"Unregister a dynamic driver\","
                                   "\"params\":{"
                                   "\"properties\":{"
                                   "\"id\":{\"type\":\"string\"}"
                                   "},"
                                   "\"required\":[\"id\"]"
                                   "}"
                                   "}";

    MCP_ToolRegister_Legacy("system.removeDriver", removeDriverHandler, removeDriverSchema);

    // Register system.executeDriverFunction tool
    const char* executeDriverFunctionSchema = "{"
                                            "\"name\":\"system.executeDriverFunction\","
                                            "\"description\":\"Execute a custom function on a driver\","
                                            "\"params\":{"
                                            "\"properties\":{"
                                            "\"id\":{\"type\":\"string\"},"
                                            "\"function\":{\"type\":\"string\"},"
                                            "\"functionParams\":{\"type\":\"object\"}"
                                            "},"
                                            "\"required\":[\"id\",\"function\"]"
                                            "}"
                                            "}";

    MCP_ToolRegister_Legacy("system.executeDriverFunction", executeDriverFunctionHandler, executeDriverFunctionSchema);

    // Load all persistent drivers
    MCP_DynamicDriverLoadAll();

    return 0;
}

// Parse a dynamic driver definition from JSON
static DynamicDriverDefinition* parseDynamicDriver(const char* json, size_t length) {
    if (json == NULL || length == 0) {
        return NULL;
    }

    // Extract params object from JSON
    void* paramsJson = json_get_object_field((const char*)json, "params");
    if (paramsJson == NULL) {
        return NULL;
    }

    // Allocate driver definition
    DynamicDriverDefinition* driver = (DynamicDriverDefinition*)calloc(1, sizeof(DynamicDriverDefinition));
    if (driver == NULL) {
        return NULL;
    }

    // Extract driver fields from JSON
    char* id = json_get_string_field((const char*)paramsJson, "id");
    if (id == NULL) {
        free(driver);
        return NULL;
    }
    driver->id = id;

    driver->name = json_get_string_field((const char*)paramsJson, "name");
    driver->version = json_get_string_field((const char*)paramsJson, "version");
    
    // Get driver type
    int type = json_get_int_field((const char*)paramsJson, "type", MCP_DRIVER_TYPE_CUSTOM);
    if (type < 0 || type > MCP_DRIVER_TYPE_CUSTOM) {
        type = MCP_DRIVER_TYPE_CUSTOM; // Default to custom type if invalid
    }
    driver->type = (MCP_DriverType)type;

    // Extract script implementation
    void* implObj = json_get_object_field((const char*)paramsJson, "implementation");
    if (implObj == NULL) {
        free(driver->id);
        free(driver->name);
        free(driver->version);
        free(driver);
        return NULL;
    }

    // Get script
    driver->script = json_get_string_field((const char*)implObj, "script");
    if (driver->script == NULL) {
        free(driver->id);
        free(driver->name);
        free(driver->version);
        free(driver);
        return NULL;
    }
    driver->scriptLength = strlen(driver->script);

    // Get config schema
    driver->configSchema = json_get_string_field((const char*)paramsJson, "configSchema");

    // Get persistence flag
    driver->persistent = json_get_bool_field((const char*)paramsJson, "persistent", false);

    return driver;
}

// Free a dynamic driver definition
static void freeDynamicDriver(DynamicDriverDefinition* driver) {
    if (driver == NULL) {
        return;
    }

    free(driver->id);
    free(driver->name);
    free(driver->version);
    free(driver->script);
    free(driver->configSchema);
    free(driver);
}

int MCP_DynamicDriverRegister(const char* json, size_t length) {
    if (!s_initialized || json == NULL || length == 0) {
        return -1;
    }

    // Parse dynamic driver
    DynamicDriverDefinition* driverDef = parseDynamicDriver(json, length);
    if (driverDef == NULL) {
        return -2;  // Parsing failed
    }

    // Check if driver already exists
    const MCP_DriverInfo* existingDriver = MCP_DriverFind(driverDef->id);
    if (existingDriver != NULL) {
        freeDynamicDriver(driverDef);
        return -3;  // Driver already exists
    }

    // Check if we have space
    if (s_dynamicDriverCount >= s_maxDynamicDrivers) {
        // Try to expand the registry
        int newMax = s_maxDynamicDrivers * 2;
        DynamicDriverDefinition** newDrivers = (DynamicDriverDefinition**)realloc(
            s_dynamicDrivers, newMax * sizeof(DynamicDriverDefinition*));
        
        if (newDrivers == NULL) {
            freeDynamicDriver(driverDef);
            return -4;  // No space
        }

        s_dynamicDrivers = newDrivers;
        s_maxDynamicDrivers = newMax;
    }

    // Add to registry
    s_dynamicDrivers[s_dynamicDriverCount++] = driverDef;

    // Create JavaScript module for driver
    if (js_create_module(driverDef->id, driverDef->script, driverDef->scriptLength) != 0) {
        // Failed to create module, rollback
        s_dynamicDriverCount--;
        freeDynamicDriver(driverDef);
        return -5;  // JS module creation failed
    }

    // Register with driver manager
    MCP_DriverInfo driverInfo;
    memset(&driverInfo, 0, sizeof(MCP_DriverInfo));

    driverInfo.id = driverDef->id;
    driverInfo.name = driverDef->name ? driverDef->name : driverDef->id;  // Use ID as name if name not provided
    driverInfo.version = driverDef->version;
    driverInfo.type = driverDef->type;
    driverInfo.configSchema = driverDef->configSchema;
    driverInfo.initialized = false;

    // Set up interface functions
    driverInfo.iface.init = dynamicDriverInit;
    driverInfo.iface.deinit = dynamicDriverDeinit;
    driverInfo.iface.read = dynamicDriverRead;
    driverInfo.iface.write = dynamicDriverWrite;
    driverInfo.iface.control = dynamicDriverControl;
    driverInfo.iface.getStatus = dynamicDriverGetStatus;

    int result = MCP_DriverRegister(&driverInfo);
    if (result != 0) {
        // Clean up if registration failed
        s_dynamicDriverCount--;
        js_delete_module(driverDef->id);
        freeDynamicDriver(driverDef);
        return result;
    }

    // Save to persistent storage if needed
    if (driverDef->persistent) {
        MCP_DynamicDriverSave(driverDef->id);
    }

    return 0;
}

int MCP_DynamicDriverUnregister(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }

    // Find dynamic driver
    int index = -1;
    for (int i = 0; i < s_dynamicDriverCount; i++) {
        if (strcmp(s_dynamicDrivers[i]->id, id) == 0) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        return -2;  // Driver not found
    }

    // Unregister from driver manager
    int result = MCP_DriverUnregister(id);
    if (result != 0) {
        return result;
    }

    // Delete JavaScript module
    js_delete_module(id);

    // Free driver definition
    freeDynamicDriver(s_dynamicDrivers[index]);

    // Remove from registry by shifting remaining drivers
    for (int i = index; i < s_dynamicDriverCount - 1; i++) {
        s_dynamicDrivers[i] = s_dynamicDrivers[i + 1];
    }
    s_dynamicDrivers[s_dynamicDriverCount - 1] = NULL;
    s_dynamicDriverCount--;

    return 0;
}

// Find a dynamic driver by ID
static DynamicDriverDefinition* findDynamicDriver(const char* id) {
    if (!s_initialized || id == NULL) {
        return NULL;
    }

    for (int i = 0; i < s_dynamicDriverCount; i++) {
        if (strcmp(s_dynamicDrivers[i]->id, id) == 0) {
            return s_dynamicDrivers[i];
        }
    }

    return NULL;
}

// Dynamic driver interface functions

static int dynamicDriverInit(const void* config) {
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        // Create a new context if none exists
        context = MCP_ContextCreate("driver_execution", NULL, 32);
        if (context == NULL) {
            return -1;
        }
        MCP_ContextSetCurrent(context);
    }

    // Find driver ID from driver info - the driver manager should set this
    const char* driverId = context->name; // In a real implementation, this would be set by the driver manager

    // Since our context doesn't have the ID set, we need to extract it another way
    // For this example, we'll assume the ID is the last part of the function name in a stack trace
    // In a real implementation, the driver manager would set this information

    if (driverId == NULL || strncmp(driverId, "driver_", 7) != 0) {
        // No driver ID in context, try to find the driver by checking all drivers
        // This is a fallback approach and wouldn't be needed in a real implementation
        // where the driver manager properly sets the context

        // In a production implementation, you would set driver_id in the context before calling
        // For this example, we'll just get the first driver as a demo
        if (s_dynamicDriverCount > 0) {
            driverId = s_dynamicDrivers[0]->id;
            
            // Store the driver ID in the context for future calls
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            MCP_ContextFree(context);
            return -2;  // No driver ID available
        }
    }

    // Find dynamic driver
    DynamicDriverDefinition* driver = findDynamicDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Convert config to JSON string
    char configJson[1024] = "{}";
    if (config != NULL) {
        // In a real implementation, this would convert the config pointer to JSON
        snprintf(configJson, sizeof(configJson), "%s", (const char*)config);
    }

    // Call driver's init function in JavaScript
    char result[128];
    int jsResult = js_call_function(driver->id, "init", configJson, result, sizeof(result));
    if (jsResult != 0) {
        return -4;
    }

    // Parse result to get success/failure
    int success = atoi(result);
    if (success == 0) {
        driver->initialized = true;
        return 0;
    }

    return -5;
}

static int dynamicDriverDeinit(void) {
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        return -1;
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        // Fallback - same as init function
        if (s_dynamicDriverCount > 0) {
            driverId = s_dynamicDrivers[0]->id;
        } else {
            return -2;
        }
    }

    // Find dynamic driver
    DynamicDriverDefinition* driver = findDynamicDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Call driver's deinit function
    char result[128];
    int jsResult = js_call_function(driver->id, "deinit", "{}", result, sizeof(result));
    if (jsResult != 0) {
        return -4;
    }

    driver->initialized = false;
    return 0;
}

static int dynamicDriverRead(void* data, size_t maxSize, size_t* actualSize) {
    // Implementation would call the driver's read function in JavaScript
    if (data == NULL || maxSize == 0) {
        return -1;
    }

    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        // Create a new context if none exists
        context = MCP_ContextCreate("driver_execution", NULL, 32);
        if (context == NULL) {
            return -2;
        }
        MCP_ContextSetCurrent(context);
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        // Fallback - same as init function
        if (s_dynamicDriverCount > 0) {
            driverId = s_dynamicDrivers[0]->id;
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            return -3;
        }
    }

    // Find dynamic driver
    DynamicDriverDefinition* driver = findDynamicDriver(driverId);
    if (driver == NULL) {
        return -4;
    }

    // Create parameters for read function
    char params[128];
    snprintf(params, sizeof(params), "{\"maxSize\":%zu}", maxSize);

    // Call driver's read function
    char result[4096]; // Buffer for read data
    int jsResult = js_call_function(driver->id, "read", params, result, sizeof(result));
    if (jsResult != 0) {
        return -5;
    }

    // In a real implementation, parse result to get read data and copy to data buffer
    // For this example, just copy the result string
    size_t resultLen = strlen(result);
    size_t copySize = (resultLen < maxSize) ? resultLen : maxSize;
    memcpy(data, result, copySize);
    
    if (actualSize != NULL) {
        *actualSize = copySize;
    }

    return 0;
}

static int dynamicDriverWrite(const void* data, size_t size) {
    // Similar to read but calling the write function
    if (data == NULL) {
        return -1;
    }

    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        // Create a new context if none exists
        context = MCP_ContextCreate("driver_execution", NULL, 32);
        if (context == NULL) {
            return -2;
        }
        MCP_ContextSetCurrent(context);
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        // Fallback - same as init function
        if (s_dynamicDriverCount > 0) {
            driverId = s_dynamicDrivers[0]->id;
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            return -3;
        }
    }

    // Find dynamic driver
    DynamicDriverDefinition* driver = findDynamicDriver(driverId);
    if (driver == NULL) {
        return -4;
    }

    // For simplicity assume data is a string
    // In real implementation, would need to properly encode binary data
    char params[4096]; 
    snprintf(params, sizeof(params), "{\"data\":\"%.*s\",\"size\":%zu}", 
             (int)size, (const char*)data, size);

    // Call driver's write function
    char result[128];
    int jsResult = js_call_function(driver->id, "write", params, result, sizeof(result));
    if (jsResult != 0) {
        return -5;
    }

    // Parse result to get number of bytes written
    int bytesWritten = atoi(result);
    return bytesWritten;
}

static int dynamicDriverControl(uint32_t command, void* arg) {
    // Implement similar to read/write but call the control function
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        // Create a new context if none exists
        context = MCP_ContextCreate("driver_execution", NULL, 32);
        if (context == NULL) {
            return -1;
        }
        MCP_ContextSetCurrent(context);
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        // Fallback - same as init function
        if (s_dynamicDriverCount > 0) {
            driverId = s_dynamicDrivers[0]->id;
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            return -2;
        }
    }

    // Find dynamic driver
    DynamicDriverDefinition* driver = findDynamicDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Create parameters for control function
    char params[4096];
    if (arg != NULL) {
        // Assuming arg is a string for simplicity
        snprintf(params, sizeof(params), "{\"command\":%u,\"arg\":\"%s\"}", 
                 command, (const char*)arg);
    } else {
        snprintf(params, sizeof(params), "{\"command\":%u}", command);
    }

    // Call driver's control function
    char result[128];
    int jsResult = js_call_function(driver->id, "control", params, result, sizeof(result));
    if (jsResult != 0) {
        return -4;
    }

    // Parse result
    int controlResult = atoi(result);
    return controlResult;
}

static int dynamicDriverGetStatus(void* status, size_t maxSize) {
    // Similar to read but call the getStatus function
    if (status == NULL || maxSize == 0) {
        return -1;
    }

    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        // Create a new context if none exists
        context = MCP_ContextCreate("driver_execution", NULL, 32);
        if (context == NULL) {
            return -2;
        }
        MCP_ContextSetCurrent(context);
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        // Fallback - same as init function
        if (s_dynamicDriverCount > 0) {
            driverId = s_dynamicDrivers[0]->id;
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            return -3;
        }
    }

    // Find dynamic driver
    DynamicDriverDefinition* driver = findDynamicDriver(driverId);
    if (driver == NULL) {
        return -4;
    }

    // Call driver's getStatus function
    char params[32];
    snprintf(params, sizeof(params), "{\"maxSize\":%zu}", maxSize);
    
    char result[4096]; // Buffer for status data
    int jsResult = js_call_function(driver->id, "getStatus", params, result, sizeof(result));
    if (jsResult != 0) {
        return -5;
    }

    // Copy result to status buffer
    size_t resultLen = strlen(result);
    size_t copySize = (resultLen < maxSize) ? resultLen : maxSize;
    memcpy(status, result, copySize);

    return copySize;
}

int MCP_DynamicDriverSave(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }

    // Find dynamic driver
    DynamicDriverDefinition* driver = findDynamicDriver(id);
    if (driver == NULL) {
        return -2;
    }

    // Create storage key
    char key[64];
    snprintf(key, sizeof(key), "driver_%s", id);

    // Serialize driver to JSON
    char json[MAX_DRIVER_SCRIPT_SIZE + 1024];
    int offset = 0;

    offset += snprintf(json + offset, sizeof(json) - offset, 
                    "{"
                    "\"id\":\"%s\",", driver->id);
                    
    if (driver->name) {
        offset += snprintf(json + offset, sizeof(json) - offset, 
                        "\"name\":\"%s\",", driver->name);
    }

    if (driver->version) {
        offset += snprintf(json + offset, sizeof(json) - offset, 
                        "\"version\":\"%s\",", driver->version);
    }

    offset += snprintf(json + offset, sizeof(json) - offset, 
                    "\"type\":%d,"
                    "\"implementation\":{"
                    "\"script\":\"%s\""
                    "},",
                    driver->type, driver->script);

    if (driver->configSchema) {
        offset += snprintf(json + offset, sizeof(json) - offset, 
                        "\"configSchema\":%s,", driver->configSchema);
    }

    offset += snprintf(json + offset, sizeof(json) - offset, 
                    "\"persistent\":true"
                    "}");

    // Write to persistent storage
    return persistent_storage_write(key, json, strlen(json));
}

int MCP_DynamicDriverLoad(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }

    // Create storage key
    char key[64];
    snprintf(key, sizeof(key), "driver_%s", id);

    // Read from persistent storage
    char json[MAX_DRIVER_SCRIPT_SIZE + 1024];
    size_t actualSize;
    int result = persistent_storage_read(key, json, sizeof(json), &actualSize);
    if (result != 0) {
        return result;
    }

    // Create define driver tool JSON
    char toolJson[MAX_DRIVER_SCRIPT_SIZE + 1024 + 128]; // Add extra space for wrapper
    snprintf(toolJson, sizeof(toolJson),
             "{\"tool\":\"system.defineDriver\",\"params\":%s}", json);

    // Register the driver
    result = MCP_DynamicDriverRegister(toolJson, strlen(toolJson));

    return result;
}

int MCP_DynamicDriverLoadAll(void) {
    if (!s_initialized) {
        return -1;
    }

    // Get all keys from persistent storage
    char* keys[128];
    int keyCount = persistent_storage_get_keys(keys, 128);
    if (keyCount <= 0) {
        return 0;  // No keys found
    }

    // Load each driver
    int loadedCount = 0;
    for (int i = 0; i < keyCount; i++) {
        if (strncmp(keys[i], "driver_", 7) == 0) {
            char* driverId = keys[i] + 7;  // Skip "driver_" prefix
            if (MCP_DynamicDriverLoad(driverId) == 0) {
                loadedCount++;
            }
        }
        free(keys[i]);  // Free allocated key string
    }

    return loadedCount;
}

int MCP_DynamicDriverExecuteFunction(const char* driverId, const char* funcName, 
                                   const char* params, size_t paramsLength,
                                   void* result, size_t maxResultSize) {
    if (!s_initialized || driverId == NULL || funcName == NULL || result == NULL) {
        return -1;
    }

    // Find dynamic driver
    DynamicDriverDefinition* driver = findDynamicDriver(driverId);
    if (driver == NULL) {
        return -2;
    }

    // Set up execution context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        // Create a new context if none exists
        context = MCP_ContextCreate("driver_execution", NULL, 32);
        if (context == NULL) {
            return -3;
        }
        MCP_ContextSetCurrent(context);
    }

    // Set the driver ID in the context
    MCP_ContextSetValue(context, "driver_id", driverId);

    // Construct parameters
    const char* actualParams = "{}";
    if (params != NULL && paramsLength > 0) {
        actualParams = params;
    }

    // Call the function
    return js_call_function(driver->id, funcName, actualParams, (char*)result, maxResultSize);
}

// Forward declarations for tool handlers
MCP_ToolResult listDriversHandler(const char* json, size_t length);
MCP_ToolResult removeDriverHandler(const char* json, size_t length);
MCP_ToolResult executeDriverFunctionHandler(const char* json, size_t length);

// Tool handler for system.defineDriver
MCP_ToolResult defineDriverHandler(const char* json, size_t length) {
    int result = MCP_DynamicDriverRegister(json, length);
    if (result == 0) {
        return MCP_ToolCreateSuccessResult("{\"status\":\"success\"}");
    } else {
        char errorMsg[128];
        switch(result) {
            case -1:
                snprintf(errorMsg, sizeof(errorMsg), "System not initialized");
                break;
            case -2:
                snprintf(errorMsg, sizeof(errorMsg), "Failed to parse driver definition");
                break;
            case -3:
                snprintf(errorMsg, sizeof(errorMsg), "Driver already exists");
                break;
            case -4:
                snprintf(errorMsg, sizeof(errorMsg), "No space for new driver");
                break;
            case -5:
                snprintf(errorMsg, sizeof(errorMsg), "Failed to create JavaScript module");
                break;
            default:
                snprintf(errorMsg, sizeof(errorMsg), "Failed to register driver (code: %d)", result);
                break;
        }
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, errorMsg);
    }
}

// Tool handler for system.listDrivers
MCP_ToolResult listDriversHandler(const char* json, size_t length) {
    if (!s_initialized) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, "Driver system not initialized");
    }

    // Extract optional filter from params
    const char* filterType = NULL;
    if (json != NULL && length > 0) {
        void* paramsObj = json_get_object_field((const char*)json, "params");
        if (paramsObj != NULL) {
            filterType = json_get_string_field((const char*)paramsObj, "type");
        }
    }

    // Build result JSON
    char* resultJson = (char*)malloc(4096);
    if (resultJson == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, "Memory allocation failed");
    }

    // Start JSON array
    int offset = 0;
    offset += snprintf(resultJson + offset, 4096 - offset, "{\"drivers\":[");

    // Get all drivers from driver manager
    // In a complete implementation, this would call a function like MCP_DriverGetAll
    // For demonstration, we'll just list our dynamic drivers
    int driverCount = 0;

    // Loop through all dynamic drivers
    for (int i = 0; i < s_dynamicDriverCount; i++) {
        // Apply filter if specified
        if (filterType != NULL) {
            const char* typeStr = "custom";
            switch (s_dynamicDrivers[i]->type) {
                case MCP_DRIVER_TYPE_SENSOR: typeStr = "sensor"; break;
                case MCP_DRIVER_TYPE_ACTUATOR: typeStr = "actuator"; break;
                case MCP_DRIVER_TYPE_INTERFACE: typeStr = "interface"; break;
                case MCP_DRIVER_TYPE_STORAGE: typeStr = "storage"; break;
                case MCP_DRIVER_TYPE_NETWORK: typeStr = "network"; break;
                default: typeStr = "custom"; break;
            }

            if (strcmp(filterType, typeStr) != 0) {
                continue;  // Skip this driver
            }
        }

        // Add comma if not first item
        if (driverCount > 0) {
            offset += snprintf(resultJson + offset, 4096 - offset, ",");
        }

        // Start driver object
        offset += snprintf(resultJson + offset, 4096 - offset, "{\"id\":\"%s\"", 
                        s_dynamicDrivers[i]->id);

        // Add name if available
        if (s_dynamicDrivers[i]->name != NULL) {
            offset += snprintf(resultJson + offset, 4096 - offset, ",\"name\":\"%s\"", 
                            s_dynamicDrivers[i]->name);
        }

        // Add version if available
        if (s_dynamicDrivers[i]->version != NULL) {
            offset += snprintf(resultJson + offset, 4096 - offset, ",\"version\":\"%s\"", 
                            s_dynamicDrivers[i]->version);
        }

        // Add type
        const char* typeStr = "custom";
        switch (s_dynamicDrivers[i]->type) {
            case MCP_DRIVER_TYPE_SENSOR: typeStr = "sensor"; break;
            case MCP_DRIVER_TYPE_ACTUATOR: typeStr = "actuator"; break;
            case MCP_DRIVER_TYPE_INTERFACE: typeStr = "interface"; break;
            case MCP_DRIVER_TYPE_STORAGE: typeStr = "storage"; break;
            case MCP_DRIVER_TYPE_NETWORK: typeStr = "network"; break;
            default: typeStr = "custom"; break;
        }
        offset += snprintf(resultJson + offset, 4096 - offset, ",\"type\":\"%s\"", typeStr);

        // Add initialization state
        offset += snprintf(resultJson + offset, 4096 - offset, ",\"initialized\":%s", 
                        s_dynamicDrivers[i]->initialized ? "true" : "false");

        // Add dynamic flag
        offset += snprintf(resultJson + offset, 4096 - offset, ",\"dynamic\":true");

        // Add persistence flag
        offset += snprintf(resultJson + offset, 4096 - offset, ",\"persistent\":%s", 
                        s_dynamicDrivers[i]->persistent ? "true" : "false");

        // End driver object
        offset += snprintf(resultJson + offset, 4096 - offset, "}");
        driverCount++;
    }

    // End JSON array
    offset += snprintf(resultJson + offset, 4096 - offset, "]}");

    // Create and return result
    MCP_ToolResult result = MCP_ToolCreateSuccessResult(resultJson);
    free(resultJson);
    return result;
}

// Tool handler for system.removeDriver
MCP_ToolResult removeDriverHandler(const char* json, size_t length) {
    if (!s_initialized) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, "Driver system not initialized");
    }

    // Extract driver ID from params
    void* paramsObj = json_get_object_field((const char*)json, "params");
    if (paramsObj == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing params object");
    }

    char* driverId = json_get_string_field((const char*)paramsObj, "id");
    if (driverId == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing driver ID");
    }

    // Unregister the driver
    int result = MCP_DynamicDriverUnregister(driverId);
    free(driverId);

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

// Tool handler for system.executeDriverFunction
MCP_ToolResult executeDriverFunctionHandler(const char* json, size_t length) {
    if (!s_initialized) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, "Driver system not initialized");
    }

    // Extract parameters
    void* paramsObj = json_get_object_field((const char*)json, "params");
    if (paramsObj == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing params object");
    }

    char* driverId = json_get_string_field((const char*)paramsObj, "id");
    if (driverId == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing driver ID");
    }

    char* funcName = json_get_string_field((const char*)paramsObj, "function");
    if (funcName == NULL) {
        free(driverId);
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing function name");
    }

    // Get function parameters (optional)
    char* funcParams = json_get_string_field((const char*)paramsObj, "functionParams");
    size_t funcParamsLength = funcParams ? strlen(funcParams) : 0;

    // Execute function
    char resultBuffer[4096];
    int result = MCP_DynamicDriverExecuteFunction(driverId, funcName, funcParams, funcParamsLength,
                                               resultBuffer, sizeof(resultBuffer));

    // Clean up
    free(driverId);
    free(funcName);
    if (funcParams != NULL) {
        free(funcParams);
    }

    // Return result
    if (result >= 0) {
        // Create success result with function output
        char jsonResult[4096 + 32];
        snprintf(jsonResult, sizeof(jsonResult), "{\"result\":%s}", resultBuffer);
        return MCP_ToolCreateSuccessResult(jsonResult);
    } else {
        char errorMsg[128];
        switch(result) {
            case -1:
                snprintf(errorMsg, sizeof(errorMsg), "Invalid parameters or system not initialized");
                break;
            case -2:
                snprintf(errorMsg, sizeof(errorMsg), "Driver not found");
                break;
            case -3:
                snprintf(errorMsg, sizeof(errorMsg), "Context creation failed");
                break;
            default:
                snprintf(errorMsg, sizeof(errorMsg), "Function execution failed (code: %d)", result);
                break;
        }
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, errorMsg);
    }
}