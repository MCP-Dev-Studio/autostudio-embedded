#include "driver_bytecode.h"
#include "driver_manager.h"
#include "../tool_system/bytecode_interpreter.h"
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

// Maximum bytecode program size (in bytes)
#define MAX_DRIVER_BYTECODE_SIZE (8 * 1024)

// Bytecode driver implementation structure
typedef struct {
    char* id;                       // Driver ID
    char* name;                     // Driver name
    char* version;                  // Driver version
    MCP_DriverType type;            // Driver type
    MCP_BytecodeProgram* initProgram;    // Init function bytecode
    MCP_BytecodeProgram* deinitProgram;  // Deinit function bytecode
    MCP_BytecodeProgram* readProgram;    // Read function bytecode
    MCP_BytecodeProgram* writeProgram;   // Write function bytecode
    MCP_BytecodeProgram* controlProgram; // Control function bytecode
    MCP_BytecodeProgram* getStatusProgram; // GetStatus function bytecode
    bool initialized;               // Init state
    char* configSchema;             // JSON schema for config
    bool persistent;                // Persistence flag
} BytecodeDriverDefinition;

// Bytecode driver registry
static BytecodeDriverDefinition** s_bytecodeDrivers = NULL;
static int s_maxBytecodeDrivers = 0;
static int s_bytecodeDriverCount = 0;
static bool s_initialized = false;

// Forward declarations for driver interface functions
static int bytecodeDriverInit(const void* config);
static int bytecodeDriverDeinit(void);
static int bytecodeDriverRead(void* data, size_t maxSize, size_t* actualSize);
static int bytecodeDriverWrite(const void* data, size_t size);
static int bytecodeDriverControl(uint32_t command, void* arg);
static int bytecodeDriverGetStatus(void* status, size_t maxSize);

// Forward declarations for tool handlers
MCP_ToolResult defineDriverBytecodeHandler(const char* json, size_t length);
MCP_ToolResult listDriversBytecodeHandler(const char* json, size_t length);
MCP_ToolResult removeDriverBytecodeHandler(const char* json, size_t length);
MCP_ToolResult executeDriverFunctionBytecodeHandler(const char* json, size_t length);

// Get driver context ID for thread-local storage
static const char* getDriverContextId(const char* driverId) {
    static char contextId[64];
    snprintf(contextId, sizeof(contextId), "driver_%s", driverId);
    return contextId;
}

int MCP_BytecodeDriverInit(void) {
    if (s_initialized) {
        return 0;  // Already initialized
    }

    // Initialize bytecode interpreter
    if (MCP_BytecodeInterpreterInit() != 0) {
        return -1;  // Failed to initialize bytecode interpreter
    }

    // Initialize bytecode driver registry
    s_maxBytecodeDrivers = 16;  // Start with space for 16 bytecode drivers
    s_bytecodeDrivers = (BytecodeDriverDefinition**)calloc(s_maxBytecodeDrivers, 
                                                      sizeof(BytecodeDriverDefinition*));
    if (s_bytecodeDrivers == NULL) {
        return -2;  // Memory allocation failed
    }

    s_bytecodeDriverCount = 0;
    s_initialized = true;

    // Register system.defineDriverBytecode tool
    const char* defineDriverSchema = "{"
                                   "\"name\":\"system.defineDriver\","
                                   "\"description\":\"Define a new driver with bytecode implementation\","
                                   "\"params\":{"
                                   "\"properties\":{"
                                   "\"id\":{\"type\":\"string\"},"
                                   "\"name\":{\"type\":\"string\"},"
                                   "\"version\":{\"type\":\"string\"},"
                                   "\"type\":{\"type\":\"integer\",\"description\":\"Driver type (0=sensor, 1=actuator, 2=interface, 3=storage, 4=network, 5=custom)\"},"
                                   "\"implementation\":{\"type\":\"object\",\"properties\":{"
                                   "\"init\":{\"type\":\"object\"},"
                                   "\"deinit\":{\"type\":\"object\"},"
                                   "\"read\":{\"type\":\"object\"},"
                                   "\"write\":{\"type\":\"object\"},"
                                   "\"control\":{\"type\":\"object\"},"
                                   "\"getStatus\":{\"type\":\"object\"}"
                                   "}},"
                                   "\"configSchema\":{\"type\":\"object\"},"
                                   "\"persistent\":{\"type\":\"boolean\"}"
                                   "},"
                                   "\"required\":[\"id\",\"implementation\"]"
                                   "}"
                                   "}";

    MCP_ToolRegister_Legacy("system.defineDriver", defineDriverBytecodeHandler, defineDriverSchema);

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

    MCP_ToolRegister_Legacy("system.listDrivers", listDriversBytecodeHandler, listDriversSchema);

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

    MCP_ToolRegister_Legacy("system.removeDriver", removeDriverBytecodeHandler, removeDriverSchema);

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

    MCP_ToolRegister_Legacy("system.executeDriverFunction", executeDriverFunctionBytecodeHandler, executeDriverFunctionSchema);

    // Load all persistent drivers
    MCP_BytecodeDriverLoadAll();

    return 0;
}

// Parse bytecode from JSON
static MCP_BytecodeProgram* parseBytecodeProgram(const void* bytecodeObj) {
    if (bytecodeObj == NULL) {
        return NULL;
    }

    // This would need a real implementation to convert JSON to bytecode
    // For this example, we'll just create a minimal bytecode program
    
    MCP_BytecodeProgram* program = (MCP_BytecodeProgram*)calloc(1, sizeof(MCP_BytecodeProgram));
    if (program == NULL) {
        return NULL;
    }
    
    // Allocate instruction array (just enough for a basic program)
    program->instructionCount = 3;
    program->instructions = (MCP_BytecodeInstruction*)calloc(
        program->instructionCount, sizeof(MCP_BytecodeInstruction));
    
    if (program->instructions == NULL) {
        free(program);
        return NULL;
    }
    
    // Build a simple program that returns a constant value
    program->instructions[0].opcode = MCP_BYTECODE_OP_PUSH_NUM;
    program->instructions[0].operand.numberValue = 0;  // Success return code
    
    program->instructions[1].opcode = MCP_BYTECODE_OP_PUSH_STR;
    program->instructions[1].operand.stringIndex = 0;  // Index in string pool
    
    program->instructions[2].opcode = MCP_BYTECODE_OP_HALT;
    
    // Allocate string pool
    program->stringPoolSize = 1;
    program->stringPool = (char**)calloc(program->stringPoolSize, sizeof(char*));
    if (program->stringPool == NULL) {
        free(program->instructions);
        free(program);
        return NULL;
    }
    
    // Add default result string
    program->stringPool[0] = strdup("{\"status\":\"success\"}");
    if (program->stringPool[0] == NULL) {
        free(program->stringPool);
        free(program->instructions);
        free(program);
        return NULL;
    }
    
    return program;
}

// Parse a bytecode driver definition from JSON
static BytecodeDriverDefinition* parseBytecodeDriver(const char* json, size_t length) {
    if (json == NULL || length == 0) {
        return NULL;
    }

    // Extract params object from JSON
    void* paramsJson = json_get_object_field((const char*)json, "params");
    if (paramsJson == NULL) {
        return NULL;
    }

    // Allocate driver definition
    BytecodeDriverDefinition* driver = (BytecodeDriverDefinition*)calloc(1, sizeof(BytecodeDriverDefinition));
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

    // Extract bytecode programs
    void* implObj = json_get_object_field((const char*)paramsJson, "implementation");
    if (implObj == NULL) {
        free(driver->id);
        free(driver->name);
        free(driver->version);
        free(driver);
        return NULL;
    }

    // Parse bytecode programs for each function
    void* initObj = json_get_object_field((const char*)implObj, "init");
    driver->initProgram = parseBytecodeProgram(initObj);

    void* deinitObj = json_get_object_field((const char*)implObj, "deinit");
    driver->deinitProgram = parseBytecodeProgram(deinitObj);

    void* readObj = json_get_object_field((const char*)implObj, "read");
    driver->readProgram = parseBytecodeProgram(readObj);

    void* writeObj = json_get_object_field((const char*)implObj, "write");
    driver->writeProgram = parseBytecodeProgram(writeObj);

    void* controlObj = json_get_object_field((const char*)implObj, "control");
    driver->controlProgram = parseBytecodeProgram(controlObj);

    void* getStatusObj = json_get_object_field((const char*)implObj, "getStatus");
    driver->getStatusProgram = parseBytecodeProgram(getStatusObj);

    // Ensure required programs exist
    if (driver->readProgram == NULL || driver->writeProgram == NULL) {
        // Free allocated resources
        if (driver->initProgram) MCP_BytecodeFreeProgram(driver->initProgram);
        if (driver->deinitProgram) MCP_BytecodeFreeProgram(driver->deinitProgram);
        if (driver->readProgram) MCP_BytecodeFreeProgram(driver->readProgram);
        if (driver->writeProgram) MCP_BytecodeFreeProgram(driver->writeProgram);
        if (driver->controlProgram) MCP_BytecodeFreeProgram(driver->controlProgram);
        if (driver->getStatusProgram) MCP_BytecodeFreeProgram(driver->getStatusProgram);
        free(driver->id);
        free(driver->name);
        free(driver->version);
        free(driver);
        return NULL;
    }

    // Get config schema
    driver->configSchema = json_get_string_field((const char*)paramsJson, "configSchema");

    // Get persistence flag
    driver->persistent = json_get_bool_field((const char*)paramsJson, "persistent", false);

    return driver;
}

// Free a bytecode driver definition
static void freeBytecodeDriver(BytecodeDriverDefinition* driver) {
    if (driver == NULL) {
        return;
    }

    free(driver->id);
    free(driver->name);
    free(driver->version);
    free(driver->configSchema);
    
    if (driver->initProgram) MCP_BytecodeFreeProgram(driver->initProgram);
    if (driver->deinitProgram) MCP_BytecodeFreeProgram(driver->deinitProgram);
    if (driver->readProgram) MCP_BytecodeFreeProgram(driver->readProgram);
    if (driver->writeProgram) MCP_BytecodeFreeProgram(driver->writeProgram);
    if (driver->controlProgram) MCP_BytecodeFreeProgram(driver->controlProgram);
    if (driver->getStatusProgram) MCP_BytecodeFreeProgram(driver->getStatusProgram);
    
    free(driver);
}

int MCP_BytecodeDriverRegister(const char* json, size_t length) {
    if (!s_initialized || json == NULL || length == 0) {
        return -1;
    }

    // Parse bytecode driver
    BytecodeDriverDefinition* driverDef = parseBytecodeDriver(json, length);
    if (driverDef == NULL) {
        return -2;  // Parsing failed
    }

    // Check if driver already exists
    const MCP_DriverInfo* existingDriver = MCP_DriverFind(driverDef->id);
    if (existingDriver != NULL) {
        freeBytecodeDriver(driverDef);
        return -3;  // Driver already exists
    }

    // Check if we have space
    if (s_bytecodeDriverCount >= s_maxBytecodeDrivers) {
        // Try to expand the registry
        int newMax = s_maxBytecodeDrivers * 2;
        BytecodeDriverDefinition** newDrivers = (BytecodeDriverDefinition**)realloc(
            s_bytecodeDrivers, newMax * sizeof(BytecodeDriverDefinition*));
        
        if (newDrivers == NULL) {
            freeBytecodeDriver(driverDef);
            return -4;  // No space
        }

        s_bytecodeDrivers = newDrivers;
        s_maxBytecodeDrivers = newMax;
    }

    // Add to registry
    s_bytecodeDrivers[s_bytecodeDriverCount++] = driverDef;

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
    driverInfo.iface.init = bytecodeDriverInit;
    driverInfo.iface.deinit = bytecodeDriverDeinit;
    driverInfo.iface.read = bytecodeDriverRead;
    driverInfo.iface.write = bytecodeDriverWrite;
    driverInfo.iface.control = bytecodeDriverControl;
    driverInfo.iface.getStatus = bytecodeDriverGetStatus;

    int result = MCP_DriverRegister(&driverInfo);
    if (result != 0) {
        // Clean up if registration failed
        s_bytecodeDriverCount--;
        freeBytecodeDriver(driverDef);
        return result;
    }

    // Save to persistent storage if needed
    if (driverDef->persistent) {
        MCP_BytecodeDriverSave(driverDef->id);
    }

    return 0;
}

int MCP_BytecodeDriverUnregister(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }

    // Find bytecode driver
    int index = -1;
    for (int i = 0; i < s_bytecodeDriverCount; i++) {
        if (strcmp(s_bytecodeDrivers[i]->id, id) == 0) {
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

    // Free driver definition
    freeBytecodeDriver(s_bytecodeDrivers[index]);

    // Remove from registry by shifting remaining drivers
    for (int i = index; i < s_bytecodeDriverCount - 1; i++) {
        s_bytecodeDrivers[i] = s_bytecodeDrivers[i + 1];
    }
    s_bytecodeDrivers[s_bytecodeDriverCount - 1] = NULL;
    s_bytecodeDriverCount--;

    return 0;
}

// Find a bytecode driver by ID
static BytecodeDriverDefinition* findBytecodeDriver(const char* id) {
    if (!s_initialized || id == NULL) {
        return NULL;
    }

    for (int i = 0; i < s_bytecodeDriverCount; i++) {
        if (strcmp(s_bytecodeDrivers[i]->id, id) == 0) {
            return s_bytecodeDrivers[i];
        }
    }

    return NULL;
}

// Execute a specific program for a driver with parameters
static int executeBytecodeDriverProgram(BytecodeDriverDefinition* driver, MCP_BytecodeProgram* program, 
                                      const char* params, void* output, size_t maxOutputSize) {
    if (driver == NULL || program == NULL) {
        return -1;
    }

    // Execute the bytecode program
    MCP_BytecodeResult result = MCP_BytecodeExecute(program);
    
    // Check execution result
    if (!result.success) {
        // Free resources
        MCP_BytecodeFreeResult(&result);
        return -2;
    }
    
    // Copy result to output buffer
    if (output != NULL && maxOutputSize > 0) {
        if (result.returnValue.type == MCP_BYTECODE_VALUE_STRING && 
            result.returnValue.value.stringValue != NULL) {
            size_t resultLen = strlen(result.returnValue.value.stringValue);
            size_t copySize = (resultLen < maxOutputSize) ? resultLen : maxOutputSize - 1;
            memcpy(output, result.returnValue.value.stringValue, copySize);
            ((char*)output)[copySize] = '\0'; // Ensure null termination
        } else if (result.returnValue.type == MCP_BYTECODE_VALUE_NUMBER) {
            // Convert number to string
            snprintf((char*)output, maxOutputSize, "%g", result.returnValue.value.numberValue);
        } else {
            // Default output for other types
            strncpy((char*)output, "{}", maxOutputSize - 1);
            ((char*)output)[maxOutputSize - 1] = '\0';
        }
    }
    
    // Get return code if number
    int returnCode = 0;
    if (result.returnValue.type == MCP_BYTECODE_VALUE_NUMBER) {
        returnCode = (int)result.returnValue.value.numberValue;
    }
    
    // Free result resources
    MCP_BytecodeFreeResult(&result);
    
    return returnCode;
}

// Bytecode driver interface functions

static int bytecodeDriverInit(const void* config) {
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
    if (driverId == NULL || strncmp(driverId, "driver_", 7) != 0) {
        // No driver ID in context, try to find the driver by checking all drivers
        // This is a fallback approach and wouldn't be needed in a real implementation
        // where the driver manager properly sets the context

        // In a production implementation, you would set driver_id in the context before calling
        // For this example, we'll just get the first driver as a demo
        if (s_bytecodeDriverCount > 0) {
            driverId = s_bytecodeDrivers[0]->id;
            
            // Store the driver ID in the context for future calls
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            MCP_ContextFree(context);
            return -2;  // No driver ID available
        }
    }

    // Find bytecode driver
    BytecodeDriverDefinition* driver = findBytecodeDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Check if init program exists
    if (driver->initProgram == NULL) {
        // Consider it a success if no init program
        driver->initialized = true;
        return 0;
    }

    // Convert config to JSON string
    char configJson[1024] = "{}";
    if (config != NULL) {
        // In a real implementation, this would convert the config pointer to JSON
        snprintf(configJson, sizeof(configJson), "%s", (const char*)config);
    }

    // Execute the init program
    char result[128];
    int bytecodeResult = executeBytecodeDriverProgram(driver, driver->initProgram, 
                                                   configJson, result, sizeof(result));
    if (bytecodeResult == 0) {
        driver->initialized = true;
        return 0;
    }

    return bytecodeResult;
}

static int bytecodeDriverDeinit(void) {
    // Get driver context
    MCP_ExecutionContext* context = MCP_ContextGetCurrent();
    if (context == NULL) {
        return -1;
    }

    // Get driver ID from context
    const char* driverId = MCP_ContextGetValue(context, "driver_id");
    if (driverId == NULL) {
        // Fallback - same as init function
        if (s_bytecodeDriverCount > 0) {
            driverId = s_bytecodeDrivers[0]->id;
        } else {
            return -2;
        }
    }

    // Find bytecode driver
    BytecodeDriverDefinition* driver = findBytecodeDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Check if deinit program exists
    if (driver->deinitProgram == NULL) {
        // Consider it a success if no deinit program
        driver->initialized = false;
        return 0;
    }

    // Execute the deinit program
    char result[128];
    int bytecodeResult = executeBytecodeDriverProgram(driver, driver->deinitProgram, 
                                                   "{}", result, sizeof(result));
    
    driver->initialized = false;
    return bytecodeResult;
}

static int bytecodeDriverRead(void* data, size_t maxSize, size_t* actualSize) {
    // Implementation would call the driver's read function with bytecode
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
        if (s_bytecodeDriverCount > 0) {
            driverId = s_bytecodeDrivers[0]->id;
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            return -3;
        }
    }

    // Find bytecode driver
    BytecodeDriverDefinition* driver = findBytecodeDriver(driverId);
    if (driver == NULL) {
        return -4;
    }

    // Create parameters for read function
    char params[128];
    snprintf(params, sizeof(params), "{\"maxSize\":%zu}", maxSize);

    // Execute read program
    char result[4096]; // Buffer for read data
    int bytecodeResult = executeBytecodeDriverProgram(driver, driver->readProgram, 
                                                   params, result, sizeof(result));
    if (bytecodeResult != 0) {
        return -5;
    }

    // Copy result to data buffer
    size_t resultLen = strlen(result);
    size_t copySize = (resultLen < maxSize) ? resultLen : maxSize;
    memcpy(data, result, copySize);
    
    if (actualSize != NULL) {
        *actualSize = copySize;
    }

    return 0;
}

static int bytecodeDriverWrite(const void* data, size_t size) {
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
        if (s_bytecodeDriverCount > 0) {
            driverId = s_bytecodeDrivers[0]->id;
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            return -3;
        }
    }

    // Find bytecode driver
    BytecodeDriverDefinition* driver = findBytecodeDriver(driverId);
    if (driver == NULL) {
        return -4;
    }

    // For simplicity assume data is a string
    // In real implementation, would need to properly encode binary data
    char params[4096]; 
    snprintf(params, sizeof(params), "{\"data\":\"%.*s\",\"size\":%zu}", 
             (int)size, (const char*)data, size);

    // Execute write program
    char result[128];
    int bytecodeResult = executeBytecodeDriverProgram(driver, driver->writeProgram, 
                                                   params, result, sizeof(result));
    if (bytecodeResult < 0) {
        return -5;
    }

    // Parse result to get number of bytes written
    int bytesWritten = atoi(result);
    return bytesWritten;
}

static int bytecodeDriverControl(uint32_t command, void* arg) {
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
        if (s_bytecodeDriverCount > 0) {
            driverId = s_bytecodeDrivers[0]->id;
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            return -2;
        }
    }

    // Find bytecode driver
    BytecodeDriverDefinition* driver = findBytecodeDriver(driverId);
    if (driver == NULL) {
        return -3;
    }

    // Check if control program exists
    if (driver->controlProgram == NULL) {
        return -4; // No control program
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

    // Execute control program
    char result[128];
    int bytecodeResult = executeBytecodeDriverProgram(driver, driver->controlProgram, 
                                                   params, result, sizeof(result));
    if (bytecodeResult < 0) {
        return -5;
    }

    // Parse result
    int controlResult = atoi(result);
    return controlResult;
}

static int bytecodeDriverGetStatus(void* status, size_t maxSize) {
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
        if (s_bytecodeDriverCount > 0) {
            driverId = s_bytecodeDrivers[0]->id;
            MCP_ContextSetValue(context, "driver_id", driverId);
        } else {
            return -3;
        }
    }

    // Find bytecode driver
    BytecodeDriverDefinition* driver = findBytecodeDriver(driverId);
    if (driver == NULL) {
        return -4;
    }

    // Check if getStatus program exists
    if (driver->getStatusProgram == NULL) {
        strncpy((char*)status, "{\"status\":\"unknown\"}", maxSize);
        return 0;
    }

    // Call driver's getStatus function
    char params[32];
    snprintf(params, sizeof(params), "{\"maxSize\":%zu}", maxSize);
    
    // Execute getStatus program
    char result[4096]; // Buffer for status data
    int bytecodeResult = executeBytecodeDriverProgram(driver, driver->getStatusProgram, 
                                                   params, result, sizeof(result));
    if (bytecodeResult < 0) {
        return -5;
    }

    // Copy result to status buffer
    size_t resultLen = strlen(result);
    size_t copySize = (resultLen < maxSize) ? resultLen : maxSize;
    memcpy(status, result, copySize);

    return copySize;
}

// Serialize bytecode program to JSON
static char* serializeBytecodeProgram(const MCP_BytecodeProgram* program) {
    if (program == NULL) {
        return strdup("null");
    }
    
    // In a real implementation, this would properly serialize the bytecode program
    // For this example, we'll just create a placeholder object
    return strdup("{\"instructionCount\":3,\"stringPoolSize\":1}");
}

int MCP_BytecodeDriverSave(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }

    // Find bytecode driver
    BytecodeDriverDefinition* driver = findBytecodeDriver(id);
    if (driver == NULL) {
        return -2;
    }

    // Create storage key
    char key[64];
    snprintf(key, sizeof(key), "driver_%s", id);

    // Serialize driver to JSON
    // In a real implementation, this would serialize the bytecode programs properly
    char* json = (char*)malloc(MAX_DRIVER_BYTECODE_SIZE);
    if (json == NULL) {
        return -3;
    }
    
    int offset = 0;
    offset += snprintf(json + offset, MAX_DRIVER_BYTECODE_SIZE - offset, 
                    "{"
                    "\"id\":\"%s\",", driver->id);
                    
    if (driver->name) {
        offset += snprintf(json + offset, MAX_DRIVER_BYTECODE_SIZE - offset, 
                        "\"name\":\"%s\",", driver->name);
    }

    if (driver->version) {
        offset += snprintf(json + offset, MAX_DRIVER_BYTECODE_SIZE - offset, 
                        "\"version\":\"%s\",", driver->version);
    }

    offset += snprintf(json + offset, MAX_DRIVER_BYTECODE_SIZE - offset, 
                    "\"type\":%d,"
                    "\"implementation\":{", driver->type);
    
    // Serialize bytecode programs
    char* initProgram = serializeBytecodeProgram(driver->initProgram);
    char* deinitProgram = serializeBytecodeProgram(driver->deinitProgram);
    char* readProgram = serializeBytecodeProgram(driver->readProgram);
    char* writeProgram = serializeBytecodeProgram(driver->writeProgram);
    char* controlProgram = serializeBytecodeProgram(driver->controlProgram);
    char* getStatusProgram = serializeBytecodeProgram(driver->getStatusProgram);
    
    offset += snprintf(json + offset, MAX_DRIVER_BYTECODE_SIZE - offset,
                    "\"init\":%s,"
                    "\"deinit\":%s,"
                    "\"read\":%s,"
                    "\"write\":%s,"
                    "\"control\":%s,"
                    "\"getStatus\":%s},",
                    initProgram, deinitProgram, readProgram, 
                    writeProgram, controlProgram, getStatusProgram);
    
    free(initProgram);
    free(deinitProgram);
    free(readProgram);
    free(writeProgram);
    free(controlProgram);
    free(getStatusProgram);

    if (driver->configSchema) {
        offset += snprintf(json + offset, MAX_DRIVER_BYTECODE_SIZE - offset, 
                        "\"configSchema\":%s,", driver->configSchema);
    }

    offset += snprintf(json + offset, MAX_DRIVER_BYTECODE_SIZE - offset, 
                    "\"persistent\":true"
                    "}");

    // Write to persistent storage
    int result = persistent_storage_write(key, json, strlen(json));
    free(json);
    
    return result;
}

int MCP_BytecodeDriverLoad(const char* id) {
    if (!s_initialized || id == NULL) {
        return -1;
    }

    // Create storage key
    char key[64];
    snprintf(key, sizeof(key), "driver_%s", id);

    // Read from persistent storage
    char json[MAX_DRIVER_BYTECODE_SIZE];
    size_t actualSize;
    int result = persistent_storage_read(key, json, sizeof(json), &actualSize);
    if (result != 0) {
        return result;
    }

    // Create define driver tool JSON
    char toolJson[MAX_DRIVER_BYTECODE_SIZE + 128]; // Add extra space for wrapper
    snprintf(toolJson, sizeof(toolJson),
             "{\"tool\":\"system.defineDriver\",\"params\":%s}", json);

    // Register the driver
    result = MCP_BytecodeDriverRegister(toolJson, strlen(toolJson));

    return result;
}

int MCP_BytecodeDriverLoadAll(void) {
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
            if (MCP_BytecodeDriverLoad(driverId) == 0) {
                loadedCount++;
            }
        }
        free(keys[i]);  // Free allocated key string
    }

    return loadedCount;
}

int MCP_BytecodeDriverExecuteFunction(const char* driverId, const char* funcName, 
                                   const char* params, size_t paramsLength,
                                   void* result, size_t maxResultSize) {
    if (!s_initialized || driverId == NULL || funcName == NULL || result == NULL) {
        return -1;
    }

    // Find bytecode driver
    BytecodeDriverDefinition* driver = findBytecodeDriver(driverId);
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

    // Determine which program to execute based on function name
    MCP_BytecodeProgram* program = NULL;
    
    if (strcmp(funcName, "init") == 0) {
        program = driver->initProgram;
    } else if (strcmp(funcName, "deinit") == 0) {
        program = driver->deinitProgram;
    } else if (strcmp(funcName, "read") == 0) {
        program = driver->readProgram;
    } else if (strcmp(funcName, "write") == 0) {
        program = driver->writeProgram;
    } else if (strcmp(funcName, "control") == 0) {
        program = driver->controlProgram;
    } else if (strcmp(funcName, "getStatus") == 0) {
        program = driver->getStatusProgram;
    } else {
        return -4; // Unknown function
    }
    
    if (program == NULL) {
        return -5; // Program not available
    }

    // Construct parameters
    const char* actualParams = "{}";
    if (params != NULL && paramsLength > 0) {
        actualParams = params;
    }

    // Execute the program
    return executeBytecodeDriverProgram(driver, program, actualParams, result, maxResultSize);
}

// Tool handler for system.defineDriver
MCP_ToolResult defineDriverBytecodeHandler(const char* json, size_t length) {
    int result = MCP_BytecodeDriverRegister(json, length);
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
            default:
                snprintf(errorMsg, sizeof(errorMsg), "Failed to register driver (code: %d)", result);
                break;
        }
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, errorMsg);
    }
}

// Tool handler for system.listDrivers
MCP_ToolResult listDriversBytecodeHandler(const char* json, size_t length) {
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
    // For demonstration, we'll just list our bytecode drivers
    int driverCount = 0;

    // Loop through all bytecode drivers
    for (int i = 0; i < s_bytecodeDriverCount; i++) {
        // Apply filter if specified
        if (filterType != NULL) {
            const char* typeStr = "custom";
            switch (s_bytecodeDrivers[i]->type) {
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
                        s_bytecodeDrivers[i]->id);

        // Add name if available
        if (s_bytecodeDrivers[i]->name != NULL) {
            offset += snprintf(resultJson + offset, 4096 - offset, ",\"name\":\"%s\"", 
                            s_bytecodeDrivers[i]->name);
        }

        // Add version if available
        if (s_bytecodeDrivers[i]->version != NULL) {
            offset += snprintf(resultJson + offset, 4096 - offset, ",\"version\":\"%s\"", 
                            s_bytecodeDrivers[i]->version);
        }

        // Add type
        const char* typeStr = "custom";
        switch (s_bytecodeDrivers[i]->type) {
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
                        s_bytecodeDrivers[i]->initialized ? "true" : "false");

        // Add dynamic flag
        offset += snprintf(resultJson + offset, 4096 - offset, ",\"dynamic\":true");

        // Add driver implementation type
        offset += snprintf(resultJson + offset, 4096 - offset, ",\"implementationType\":\"bytecode\"");

        // Add persistence flag
        offset += snprintf(resultJson + offset, 4096 - offset, ",\"persistent\":%s", 
                        s_bytecodeDrivers[i]->persistent ? "true" : "false");

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
MCP_ToolResult removeDriverBytecodeHandler(const char* json, size_t length) {
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
    int result = MCP_BytecodeDriverUnregister(driverId);
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
MCP_ToolResult executeDriverFunctionBytecodeHandler(const char* json, size_t length) {
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
    int result = MCP_BytecodeDriverExecuteFunction(driverId, funcName, funcParams, funcParamsLength,
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
            case -4:
                snprintf(errorMsg, sizeof(errorMsg), "Unknown function");
                break;
            case -5:
                snprintf(errorMsg, sizeof(errorMsg), "Function not implemented");
                break;
            default:
                snprintf(errorMsg, sizeof(errorMsg), "Function execution failed (code: %d)", result);
                break;
        }
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, errorMsg);
    }
}