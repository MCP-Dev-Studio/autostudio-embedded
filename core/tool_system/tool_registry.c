/**
 * @file tool_registry.c
 * @brief Tool registry implementation with dynamic tool support
 */
#include "tool_registry.h"
#include "context_manager.h"
#include <stdlib.h>
#include <string.h>

// External JSON utility functions (minimal implementation)
extern char* json_get_string_field(const char* json, const char* field);
extern bool json_validate_schema(const char* json, const char* schema);
extern void* json_get_array_field(const char* json, const char* field);
extern void* json_get_object_field(const char* json, const char* field);
extern int json_array_length(const void* array);
extern void* json_array_get_object(const void* array, size_t index);
extern bool json_get_bool_field(const char* json, const char* field, bool defaultValue);

// External persistence functions
extern int persistent_storage_write(const char* key, const void* data, size_t size);
extern int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize);
extern int persistent_storage_get_keys(char** keys, size_t maxKeys);

// Tool entry structure
typedef struct {
    MCP_ToolDefinition definition;
    bool active;
} ToolEntry;

// Internal state
static ToolEntry* s_tools = NULL;
static int s_maxTools = 0;
static int s_toolCount = 0;
static bool s_initialized = false;

// Dynamic tool execution handler
static MCP_ToolResult dynamicToolHandler(const char* json, size_t length);

int MCP_ToolRegistryInit(int maxTools) {
    if (s_initialized) {
        return -1;  // Already initialized
    }

    // Allocate tool array
    s_tools = (ToolEntry*)calloc(maxTools, sizeof(ToolEntry));
    if (s_tools == NULL) {
        return -2;  // Memory allocation failed
    }

    s_maxTools = maxTools;
    s_toolCount = 0;
    s_initialized = true;

    // Register system.defineTool as a built-in tool
    const char* defineToolSchema = "{"
                                   "\"name\":\"system.defineTool\","
                                   "\"description\":\"Define a new tool dynamically\","
                                   "\"params\":{"
                                   "\"properties\":{"
                                   "\"name\":{\"type\":\"string\"},"
                                   "\"description\":{\"type\":\"string\"},"
                                   "\"schema\":{\"type\":\"object\"},"
                                   "\"implementationType\":{\"type\":\"string\"},"
                                   "\"implementation\":{\"type\":\"object\"},"
                                   "\"persistent\":{\"type\":\"boolean\"}"
                                   "},"
                                   "\"required\":[\"name\",\"implementation\"]"
                                   "}"
                                   "}";

    MCP_ToolRegister("system.defineTool", MCP_ToolRegisterDynamic, defineToolSchema);

    // Load all dynamic tools from persistent storage
    MCP_ToolLoadAllDynamic();

    return 0;
}

int MCP_ToolRegister(const char* name, MCP_ToolHandler handler, const char* schema) {
    if (!s_initialized || name == NULL || handler == NULL) {
        return -1;
    }

    // Check if tool already exists
    int existingIndex = MCP_ToolFind(name);
    if (existingIndex >= 0) {
        return -2;  // Tool already registered
    }

    // Check if we have space for a new tool
    if (s_toolCount >= s_maxTools) {
        return -3;  // No space for new tool
    }

    // Find free slot
    int i;
    for (i = 0; i < s_maxTools; i++) {
        if (!s_tools[i].active) {
            break;
        }
    }

    if (i >= s_maxTools) {
        return -4;  // No free slot found
    }

    // Initialize the tool definition
    s_tools[i].definition.name = strdup(name);
    if (s_tools[i].definition.name == NULL) {
        return -5;  // Memory allocation failed
    }

    s_tools[i].definition.description = NULL;

    if (schema != NULL) {
        s_tools[i].definition.schema = strdup(schema);
    } else {
        s_tools[i].definition.schema = NULL;
    }

    s_tools[i].definition.type = MCP_TOOL_TYPE_NATIVE;
    s_tools[i].definition.implementation.nativeHandler = handler;
    s_tools[i].definition.isDynamic = false;
    s_tools[i].definition.creationTime = 0;  // Set current time if needed
    s_tools[i].definition.persistent = false;

    s_tools[i].active = true;
    s_toolCount++;

    return 0;
}

// Parse a dynamic tool from JSON
static MCP_ToolDefinition* parseDynamicTool(const char* json, size_t length) {
    if (json == NULL || length == 0) {
        return NULL;
    }

    // Extract params object from JSON
    void* paramsJson = json_get_object_field(json, "params");
    if (paramsJson == NULL) {
        return NULL;
    }

    // Allocate tool definition
    MCP_ToolDefinition* tool = (MCP_ToolDefinition*)calloc(1, sizeof(MCP_ToolDefinition));
    if (tool == NULL) {
        return NULL;
    }

    // Parse basic info
    tool->name = json_get_string_field(paramsJson, "name");
    tool->description = json_get_string_field(paramsJson, "description");
    tool->schema = json_get_string_field(paramsJson, "schema");
    tool->persistent = json_get_bool_field(paramsJson, "persistent", false);

    if (tool->name == NULL) {
        // Required field missing
        free(tool->description);
        free(tool->schema);
        free(tool);
        return NULL;
    }

    // Parse implementation type
    char* implType = json_get_string_field(paramsJson, "implementationType");
    if (implType == NULL) {
        // Default to composite
        tool->type = MCP_TOOL_TYPE_COMPOSITE;
    } else if (strcmp(implType, "native") == 0) {
        tool->type = MCP_TOOL_TYPE_NATIVE;
    } else if (strcmp(implType, "composite") == 0) {
        tool->type = MCP_TOOL_TYPE_COMPOSITE;
    } else if (strcmp(implType, "script") == 0) {
        tool->type = MCP_TOOL_TYPE_SCRIPT;
    } else if (strcmp(implType, "bytecode") == 0) {
        tool->type = MCP_TOOL_TYPE_BYTECODE;
    } else {
        // Unknown implementation type
        free(tool->name);
        free(tool->description);
        free(tool->schema);
        free(implType);
        free(tool);
        return NULL;
    }

    if (implType != NULL) {
        free(implType);
    }

    // Parse implementation details
    void* implementation = json_get_object_field(paramsJson, "implementation");
    if (implementation == NULL) {
        // No implementation
        free(tool->name);
        free(tool->description);
        free(tool->schema);
        free(tool);
        return NULL;
    }

    // Parse according to type
    switch (tool->type) {
        case MCP_TOOL_TYPE_COMPOSITE: {
            // Parse composite tool steps
            void* stepsArray = json_get_array_field(implementation, "steps");
            if (stepsArray != NULL) {
                int stepCount = json_array_length(stepsArray);
                tool->implementation.composite.stepCount = stepCount;

                // Parse each step
                MCP_ToolStep* firstStep = NULL;
                MCP_ToolStep* currentStep = NULL;

                for (int i = 0; i < stepCount; i++) {
                    void* stepObj = json_array_get_object(stepsArray, i);
                    if (stepObj == NULL) {
                        continue;
                    }

                    // Allocate step
                    MCP_ToolStep* step = (MCP_ToolStep*)calloc(1, sizeof(MCP_ToolStep));
                    if (step == NULL) {
                        continue;
                    }

                    // Parse step properties
                    step->toolName = json_get_string_field(stepObj, "tool");
                    step->paramsTemplate = json_get_string_field(stepObj, "params");
                    step->resultStore = json_get_string_field(stepObj, "store");

                    if (step->toolName == NULL) {
                        // Required field missing
                        free(step->paramsTemplate);
                        free(step->resultStore);
                        free(step);
                        continue;
                    }

                    // Add to linked list
                    if (firstStep == NULL) {
                        firstStep = step;
                        currentStep = step;
                    } else {
                        currentStep->next = step;
                        currentStep = step;
                    }
                }

                tool->implementation.composite.steps = firstStep;
            }
            break;
        }

        case MCP_TOOL_TYPE_SCRIPT: {
            // Parse script implementation
            tool->implementation.script.script = json_get_string_field(implementation, "script");
            tool->implementation.script.language = json_get_string_field(implementation, "language");

            if (tool->implementation.script.script == NULL) {
                // Required field missing
                free(tool->name);
                free(tool->description);
                free(tool->schema);
                free(tool->implementation.script.language);
                free(tool);
                return NULL;
            }
            break;
        }

        case MCP_TOOL_TYPE_BYTECODE: {
            // Bytecode implementation - would need additional parsing logic
            // Set bytecode program to NULL for now
            tool->implementation.bytecode.program = NULL;
            tool->implementation.bytecode.size = 0;
            break;
        }

        case MCP_TOOL_TYPE_NATIVE:
            // For dynamic tools, native type doesn't make sense
            // Set handler to dynamicToolHandler
            tool->implementation.nativeHandler = dynamicToolHandler;
            break;
    }

    // Set dynamic flag and timestamp
    tool->isDynamic = true;
    tool->creationTime = 0;  // Would set to current time

    return tool;
}

int MCP_ToolRegisterDynamic(const char* json, size_t length) {
    if (!s_initialized || json == NULL || length == 0) {
        return -1;
    }

    // Parse dynamic tool
    MCP_ToolDefinition* toolDef = parseDynamicTool(json, length);
    if (toolDef == NULL) {
        return -2;  // Parsing failed
    }

    // Check if tool already exists
    int existingIndex = MCP_ToolFind(toolDef->name);
    if (existingIndex >= 0) {
        // Free the parsed tool
        // TODO: Implement proper cleanup for toolDef
        free(toolDef->name);
        free(toolDef->description);
        free(toolDef->schema);
        // Free implementation based on type
        free(toolDef);
        return -3;  // Tool already exists
    }

    // Find free slot
    int i;
    for (i = 0; i < s_maxTools; i++) {
        if (!s_tools[i].active) {
            break;
        }
    }

    if (i >= s_maxTools) {
        // Free the parsed tool
        // TODO: Implement proper cleanup for toolDef
        free(toolDef);
        return -4;  // No free slot
    }

    // Register the tool
    memcpy(&s_tools[i].definition, toolDef, sizeof(MCP_ToolDefinition));
    s_tools[i].active = true;
    s_toolCount++;

    // Free the temporary structure, but not its contents
    free(toolDef);

    // Save to persistent storage if needed
    if (s_tools[i].definition.persistent) {
        MCP_ToolSaveDynamic(s_tools[i].definition.name);
    }

    return 0;
}

int MCP_ToolFind(const char* name) {
    if (!s_initialized || name == NULL) {
        return -1;
    }

    for (int i = 0; i < s_maxTools; i++) {
        if (s_tools[i].active && strcmp(s_tools[i].definition.name, name) == 0) {
            return i;
        }
    }

    return -2;  // Tool not found
}

const MCP_ToolDefinition* MCP_ToolGetDefinition(const char* name) {
    int index = MCP_ToolFind(name);
    if (index < 0) {
        return NULL;
    }

    return &s_tools[index].definition;
}

// Execute a composite tool
static MCP_ToolResult executeCompositeTool(const MCP_ToolDefinition* toolDef, const char* params, size_t paramsLength) {
    // Create execution context
    MCP_ExecutionContext* context = MCP_ContextCreate("tool_execution", NULL, 32);
    if (context == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_EXECUTION_ERROR, "Failed to create execution context");
    }

    // Parse and store input parameters
    if (params != NULL && paramsLength > 0) {
        void* paramsObj = json_get_object_field(params, "params");
        if (paramsObj != NULL) {
            // Iterate through params and store in context
            // This would need implementation based on your JSON parser
        }
    }

    // Execute each step
    MCP_ToolStep* step = toolDef->implementation.composite.steps;
    MCP_ToolResult result = {0};

    while (step != NULL) {
        // Substitute variables in parameters template
        char* substParams = MCP_ContextSubstituteVariables(context, step->paramsTemplate);
        if (substParams == NULL) {
            MCP_ContextFree(context);
            return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_EXECUTION_ERROR, "Parameter substitution failed");
        }

        // Create tool execution JSON
        char* toolJson = NULL;
        size_t toolJsonSize = strlen(step->toolName) + strlen(substParams) + 64;
        toolJson = (char*)malloc(toolJsonSize);
        if (toolJson == NULL) {
            free(substParams);
            MCP_ContextFree(context);
            return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_EXECUTION_ERROR, "Memory allocation failed");
        }

        // Format tool execution JSON
        snprintf(toolJson, toolJsonSize, "{\"tool\":\"%s\",\"params\":%s}",
                 step->toolName, substParams);

        // Execute the tool
        result = MCP_ToolExecute(toolJson, strlen(toolJson));

        // Clean up
        free(toolJson);
        free(substParams);

        // Store result if needed
        if (step->resultStore != NULL && result.status == MCP_TOOL_RESULT_SUCCESS) {
            MCP_ContextStoreToolResult(context, step->resultStore, &result);
        }

        // Handle errors
        if (result.status != MCP_TOOL_RESULT_SUCCESS) {
            break;
        }

        // Move to next step
        step = step->next;
    }

    // Clean up
    MCP_ContextFree(context);

    return result;
}

// Execute a script-based tool
static MCP_ToolResult executeScriptTool(const MCP_ToolDefinition* toolDef, const char* params, size_t paramsLength) {
    // This would require script engine integration
    return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_EXECUTION_ERROR, "Script execution not implemented");
}

// Execute a bytecode-based tool
static MCP_ToolResult executeBytecodeTools(const MCP_ToolDefinition* toolDef, const char* params, size_t paramsLength) {
    // This would require bytecode interpreter integration
    return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_EXECUTION_ERROR, "Bytecode execution not implemented");
}

// Dynamic tool execution handler
static MCP_ToolResult dynamicToolHandler(const char* json, size_t length) {
    // Parse tool name
    char* toolName = json_get_string_field(json, "tool");
    if (toolName == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing tool name");
    }

    // Get tool definition
    const MCP_ToolDefinition* toolDef = MCP_ToolGetDefinition(toolName);
    free(toolName); // Free the parsed name

    if (toolDef == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_NOT_FOUND, "Tool not found");
    }

    // Execute based on tool type
    MCP_ToolResult result;

    switch (toolDef->type) {
        case MCP_TOOL_TYPE_COMPOSITE:
            result = executeCompositeTool(toolDef, json, length);
            break;

        case MCP_TOOL_TYPE_SCRIPT:
            result = executeScriptTool(toolDef, json, length);
            break;

        case MCP_TOOL_TYPE_BYTECODE:
            result = executeBytecodeTools(toolDef, json, length);
            break;

        default:
            result = MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_EXECUTION_ERROR, "Unsupported tool type");
            break;
    }

    return result;
}

MCP_ToolResult MCP_ToolExecute(const char* json, size_t length) {
    if (!s_initialized || json == NULL || length == 0) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Invalid input");
    }

    // Extract tool name from JSON
    char* toolName = json_get_string_field(json, "tool");
    if (toolName == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing tool name");
    }

    // Find tool
    int toolIndex = MCP_ToolFind(toolName);
    if (toolIndex < 0) {
        free(toolName);
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_NOT_FOUND, "Tool not found");
    }

    free(toolName);

    // Validate parameters against schema if schema exists
    if (s_tools[toolIndex].definition.schema != NULL) {
        if (!json_validate_schema(json, s_tools[toolIndex].definition.schema)) {
            return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Invalid parameters");
        }
    }

    // Execute tool handler based on type
    MCP_ToolResult result;

    switch (s_tools[toolIndex].definition.type) {
        case MCP_TOOL_TYPE_NATIVE:
            // Execute native handler directly
            result = s_tools[toolIndex].definition.implementation.nativeHandler(json, length);
            break;

        case MCP_TOOL_TYPE_COMPOSITE:
            // Execute composite tool
            result = executeCompositeTool(&s_tools[toolIndex].definition, json, length);
            break;

        case MCP_TOOL_TYPE_SCRIPT:
            // Execute script-based tool
            result = executeScriptTool(&s_tools[toolIndex].definition, json, length);
            break;

        case MCP_TOOL_TYPE_BYTECODE:
            // Execute bytecode tool
            result = executeBytecodeTools(&s_tools[toolIndex].definition, json, length);
            break;

        default:
            result = MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_EXECUTION_ERROR, "Unsupported tool type");
            break;
    }

    return result;
}

const char* MCP_ToolGetSchema(const char* name) {
    int toolIndex = MCP_ToolFind(name);
    if (toolIndex < 0) {
        return NULL;
    }

    return s_tools[toolIndex].definition.schema;
}

// Serialize a tool definition to JSON for storage
static char* serializeToolDefinition(const MCP_ToolDefinition* toolDef) {
    // This would need a proper implementation based on your JSON serialization utilities
    // Simplified placeholder implementation
    char* json = (char*)malloc(4096); // Arbitrary size
    if (json == NULL) {
        return NULL;
    }

    // Start JSON object
    int offset = 0;
    offset += snprintf(json + offset, 4096 - offset, "{\"name\":\"%s\"", toolDef->name);

    // Add description if present
    if (toolDef->description != NULL) {
        offset += snprintf(json + offset, 4096 - offset, ",\"description\":\"%s\"", toolDef->description);
    }

    // Add schema if present
    if (toolDef->schema != NULL) {
        offset += snprintf(json + offset, 4096 - offset, ",\"schema\":%s", toolDef->schema);
    }

    // Add type
    const char* typeStr = "native";
    switch (toolDef->type) {
        case MCP_TOOL_TYPE_COMPOSITE: typeStr = "composite"; break;
        case MCP_TOOL_TYPE_SCRIPT: typeStr = "script"; break;
        case MCP_TOOL_TYPE_BYTECODE: typeStr = "bytecode"; break;
        default: typeStr = "native"; break;
    }

    offset += snprintf(json + offset, 4096 - offset, ",\"type\":\"%s\"", typeStr);

    // Add implementation
    offset += snprintf(json + offset, 4096 - offset, ",\"implementation\":{");

    switch (toolDef->type) {
        case MCP_TOOL_TYPE_COMPOSITE: {
            // Add composite steps
            offset += snprintf(json + offset, 4096 - offset, "\"steps\":[");

            // Add each step
            MCP_ToolStep* step = toolDef->implementation.composite.steps;
            bool firstStep = true;

            while (step != NULL) {
                if (!firstStep) {
                    offset += snprintf(json + offset, 4096 - offset, ",");
                }

                offset += snprintf(json + offset, 4096 - offset, "{\"tool\":\"%s\"", step->toolName);

                if (step->paramsTemplate != NULL) {
                    offset += snprintf(json + offset, 4096 - offset, ",\"params\":%s", step->paramsTemplate);
                }

                if (step->resultStore != NULL) {
                    offset += snprintf(json + offset, 4096 - offset, ",\"store\":\"%s\"", step->resultStore);
                }

                offset += snprintf(json + offset, 4096 - offset, "}");

                firstStep = false;
                step = step->next;
            }

            offset += snprintf(json + offset, 4096 - offset, "]");
            break;
        }

        case MCP_TOOL_TYPE_SCRIPT: {
            // Add script details
            if (toolDef->implementation.script.script != NULL) {
                offset += snprintf(json + offset, 4096 - offset, "\"script\":\"%s\"",
                                   toolDef->implementation.script.script);
            }

            if (toolDef->implementation.script.language != NULL) {
                offset += snprintf(json + offset, 4096 - offset, ",\"language\":\"%s\"",
                                   toolDef->implementation.script.language);
            }
            break;
        }

        case MCP_TOOL_TYPE_BYTECODE:
            // Bytecode would need special handling
            offset += snprintf(json + offset, 4096 - offset, "\"bytecode\":\"...\"");
            break;

        default:
            // Nothing to add for native
            break;
    }

    offset += snprintf(json + offset, 4096 - offset, "}");

    // Add metadata
    offset += snprintf(json + offset, 4096 - offset, ",\"isDynamic\":%s",
                       toolDef->isDynamic ? "true" : "false");

    offset += snprintf(json + offset, 4096 - offset, ",\"creationTime\":%u",
                       toolDef->creationTime);

    offset += snprintf(json + offset, 4096 - offset, ",\"persistent\":%s",
                       toolDef->persistent ? "true" : "false");

    // Close JSON object
    offset += snprintf(json + offset, 4096 - offset, "}");

    return json;
}

int MCP_ToolSaveDynamic(const char* name) {
    if (!s_initialized || name == NULL) {
        return -1;
    }

    // Find tool
    int toolIndex = MCP_ToolFind(name);
    if (toolIndex < 0) {
        return -2;  // Tool not found
    }

    // Only dynamic tools should be saved
    if (!s_tools[toolIndex].definition.isDynamic) {
        return -3;  // Not a dynamic tool
    }

    // Serialize tool definition
    char* json = serializeToolDefinition(&s_tools[toolIndex].definition);
    if (json == NULL) {
        return -4;  // Serialization failed
    }

    // Create storage key
    char key[128];
    snprintf(key, sizeof(key), "tool.%s", name);

    // Write to persistent storage
    int result = persistent_storage_write(key, json, strlen(json));

    free(json);

    return result;
}

int MCP_ToolLoadDynamic(const char* name) {
    if (!s_initialized || name == NULL) {
        return -1;
    }

    // Create storage key
    char key[128];
    snprintf(key, sizeof(key), "tool.%s", name);

    // Read from persistent storage
    char buffer[4096];  // Adjust size as needed
    size_t actualSize;

    int result = persistent_storage_read(key, buffer, sizeof(buffer), &actualSize);
    if (result != 0) {
        return -2;  // Read failed
    }

    // Ensure null termination
    buffer[actualSize < sizeof(buffer) ? actualSize : sizeof(buffer) - 1] = '\0';

    // Create tool JSON
    char toolJson[4096 + 128];
    snprintf(toolJson, sizeof(toolJson),
             "{\"tool\":\"system.defineTool\",\"params\":%s}", buffer);

    // Register the tool
    result = MCP_ToolRegisterDynamic(toolJson, strlen(toolJson));

    return result;
}

int MCP_ToolLoadAllDynamic(void) {
    if (!s_initialized) {
        return -1;
    }

    // Get keys from persistent storage
    char* keys[100];  // Adjust size as needed
    int keyCount = persistent_storage_get_keys(keys, 100);

    if (keyCount <= 0) {
        return 0;  // No keys or error
    }

    // Count of loaded tools
    int loadedCount = 0;

    // Load each tool
    for (int i = 0; i < keyCount; i++) {
        // Check if key is a tool definition
        if (strncmp(keys[i], "tool.", 5) == 0) {
            // Get tool name
            const char* toolName = keys[i] + 5;  // Skip "tool." prefix

            // Load the tool
            if (MCP_ToolLoadDynamic(toolName) == 0) {
                loadedCount++;
            }
        }
    }

    return loadedCount;
}

int MCP_ToolGetList(char* buffer, size_t bufferSize) {
    if (!s_initialized || buffer == NULL || bufferSize == 0) {
        return -1;
    }

    // Start JSON array
    int offset = 0;
    offset += snprintf(buffer + offset, bufferSize - offset, "[");

    // Add tools
    bool first = true;
    for (int i = 0; i < s_maxTools; i++) {
        if (s_tools[i].active) {
            // Add comma if not first tool
            if (!first) {
                offset += snprintf(buffer + offset, bufferSize - offset, ",");
            }
            first = false;

            // Add tool info
            offset += snprintf(buffer + offset, bufferSize - offset,
                               "{\"name\":\"%s\"", s_tools[i].definition.name);

            if (s_tools[i].definition.description != NULL) {
                offset += snprintf(buffer + offset, bufferSize - offset,
                                   ",\"description\":\"%s\"", s_tools[i].definition.description);
            }

            offset += snprintf(buffer + offset, bufferSize - offset,
                               ",\"hasSchema\":%s",
                               s_tools[i].definition.schema != NULL ? "true" : "false");

            offset += snprintf(buffer + offset, bufferSize - offset,
                               ",\"isDynamic\":%s",
                               s_tools[i].definition.isDynamic ? "true" : "false");

            // Add tool type
            const char* typeStr = "native";
            switch (s_tools[i].definition.type) {
                case MCP_TOOL_TYPE_COMPOSITE: typeStr = "composite"; break;
                case MCP_TOOL_TYPE_SCRIPT: typeStr = "script"; break;
                case MCP_TOOL_TYPE_BYTECODE: typeStr = "bytecode"; break;
                default: typeStr = "native"; break;
            }

            offset += snprintf(buffer + offset, bufferSize - offset,
                               ",\"type\":\"%s\"", typeStr);

            // Close tool object
            offset += snprintf(buffer + offset, bufferSize - offset, "}");

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

MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult) {
    MCP_ToolResult result;
    result.status = MCP_TOOL_RESULT_SUCCESS;

    if (jsonResult != NULL) {
        result.resultJson = strdup(jsonResult);
    } else {
        result.resultJson = strdup("{}");
    }

    result.resultData = NULL;
    result.resultDataSize = 0;

    return result;
}

MCP_ToolResult MCP_ToolCreateErrorResult(MCP_ToolResultStatus status, const char* errorMessage) {
    MCP_ToolResult result;
    result.status = status;

    // Format error JSON
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "{\"error\":true,\"code\":%d,\"message\":\"%s\"}",
             status, errorMessage ? errorMessage : "Unknown error");

    result.resultJson = strdup(buffer);
    result.resultData = NULL;
    result.resultDataSize = 0;

    return result;
}