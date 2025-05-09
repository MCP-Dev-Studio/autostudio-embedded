#include "arduino_compat.h"
#include "mcp_logging_tool.h"
#include "mcp_logging.h"
#include "json_helpers.h"
#include "logging.h"
#include "tool_registry.h"
#include "tool_info.h"
#include "server.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(MCP_PLATFORM_HOST) && !defined(MCP_OS_HOST)
// Tool schema in JSON format
static const char* s_toolSchema = "{"
    "\"name\": \"mcp.logging\","
    "\"description\": \"Configure logging options for the MCP server\","
    "\"parameters\": {"
        "\"type\": \"object\","
        "\"properties\": {"
            "\"action\": {"
                "\"type\": \"string\","
                "\"enum\": [\"getConfig\", \"setConfig\", \"enableLogging\", \"disableLogging\", "
                          "\"setLevel\", \"addModule\", \"removeModule\", \"clearModules\", "
                          "\"enableModuleFilter\", \"disableModuleFilter\"],"
                "\"description\": \"Action to perform\""
            "},"
            "\"config\": {"
                "\"type\": \"object\","
                "\"properties\": {"
                    "\"enabled\": {\"type\": \"boolean\"},"
                    "\"maxLevel\": {\"type\": \"number\", \"minimum\": 0, \"maximum\": 5},"
                    "\"outputs\": {\"type\": \"number\"},"
                    "\"includeTimestamp\": {\"type\": \"boolean\"},"
                    "\"includeLevelName\": {\"type\": \"boolean\"},"
                    "\"includeModuleName\": {\"type\": \"boolean\"},"
                    "\"filterByModule\": {\"type\": \"boolean\"},"
                    "\"allowedModules\": {\"type\": \"array\", \"items\": {\"type\": \"string\"}}"
                "}"
            "},"
            "\"level\": {"
                "\"type\": \"string\","
                "\"enum\": [\"none\", \"error\", \"warn\", \"info\", \"debug\", \"trace\"],"
                "\"description\": \"Log level\""
            "},"
            "\"moduleName\": {"
                "\"type\": \"string\","
                "\"description\": \"Module name for filtering\""
            "}"
        "},"
        "\"required\": [\"action\"]"
    "}"
"}";
#else
// Simplified schema for host platform
static const char* s_toolSchema = "{"
    "\"name\": \"mcp.logging\","
    "\"description\": \"Configure logging options for the MCP server\""
"}";
#endif

// Forward declarations
static int handle_get_config(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result);
static int handle_set_config(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result);
static int handle_enable_logging(const char* sessionId, const char* operationId, 
                             const MCP_Content* params, MCP_Content** result);
static int handle_disable_logging(const char* sessionId, const char* operationId, 
                              const MCP_Content* params, MCP_Content** result);
static int handle_set_level(const char* sessionId, const char* operationId, 
                         const MCP_Content* params, MCP_Content** result);
static int handle_add_module(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result);
static int handle_remove_module(const char* sessionId, const char* operationId, 
                             const MCP_Content* params, MCP_Content** result);
static int handle_clear_modules(const char* sessionId, const char* operationId, 
                             const MCP_Content* params, MCP_Content** result);
static int handle_enable_module_filter(const char* sessionId, const char* operationId, 
                                   const MCP_Content* params, MCP_Content** result);
static int handle_disable_module_filter(const char* sessionId, const char* operationId, 
                                    const MCP_Content* params, MCP_Content** result);

#if defined(MCP_PLATFORM_HOST) || defined(MCP_OS_HOST)
// Host platform implementation

/**
 * @brief Initialize logging tool (host implementation)
 */
int MCP_LoggingToolInit(void) {
    printf("[HOST] MCP_LoggingToolInit called\n");
    return 0;
}

/**
 * @brief Deinitialize logging tool (host implementation)
 */
int MCP_LoggingToolDeinit(void) {
    printf("[HOST] MCP_LoggingToolDeinit called\n");
    return 0;
}

/**
 * @brief Host implementation stub for tool invocation
 * 
 * This function is used by the HOST-specific implementation to handle tool invocations
 * without dealing with potentially conflicting types.
 */
int MCP_LoggingToolInvokeHost(const char* sessionId, const char* operationId, void* params) {
    printf("[HOST] MCP_LoggingToolInvokeHost called for session %s\n", sessionId);
    printf("[HOST] Operation %s completed successfully (HOST platform stub)\n", operationId);
    return 0;
}

/**
 * @brief Register logging tool with the tool registry (host implementation)
 */
int MCP_LoggingToolRegister(void) {
    printf("[HOST] MCP_LoggingToolRegister called\n");
    printf("[HOST] Logging tool registered successfully\n");
    return 0;
}

// MCP_LoggingToolInvoke function implementation will use the standard parameters signature
// This ensures compatibility with the function declarations in the header file
int MCP_LoggingToolInvoke(const char* sessionId, const char* operationId, const MCP_Content* params) {
    printf("[HOST] MCP_LoggingToolInvoke called for session: %s, operation: %s\n", 
            sessionId ? sessionId : "NULL", operationId ? operationId : "NULL");
    // Just pass through to the host implementation with param as void*
    return MCP_LoggingToolInvokeHost(sessionId, operationId, (void*)params);
}

// Host platform stubs for the handler functions
static int handle_get_config(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_get_config called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

static int handle_set_config(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_set_config called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

static int handle_enable_logging(const char* sessionId, const char* operationId, 
                             const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_enable_logging called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

static int handle_disable_logging(const char* sessionId, const char* operationId, 
                              const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_disable_logging called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

static int handle_set_level(const char* sessionId, const char* operationId, 
                         const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_set_level called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

static int handle_add_module(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_add_module called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

static int handle_remove_module(const char* sessionId, const char* operationId, 
                             const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_remove_module called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

static int handle_clear_modules(const char* sessionId, const char* operationId, 
                             const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_clear_modules called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

static int handle_enable_module_filter(const char* sessionId, const char* operationId, 
                                   const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_enable_module_filter called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

static int handle_disable_module_filter(const char* sessionId, const char* operationId, 
                                    const MCP_Content* params, MCP_Content** result) {
    printf("[HOST] handle_disable_module_filter called\n");
    *result = MCP_ContentCreateObject();
    return 0;
}

#else /* Non-HOST platform implementation */

/**
 * @brief Initialize logging tool
 */
int MCP_LoggingToolInit(void) {
    // Nothing to initialize
    return 0;
}

/**
 * @brief Deinitialize logging tool
 */
int MCP_LoggingToolDeinit(void) {
    // Nothing to deinitialize
    return 0;
}

/**
 * @brief Handle tool invocation
 */
int MCP_LoggingToolInvoke(const char* sessionId, const char* operationId, 
                        const MCP_Content* params) {
    if (sessionId == NULL || operationId == NULL || params == NULL) {
        return -1;
    }
    
    const char* action = NULL;
    if (!MCP_ContentGetStringField(params, "action", &action) || action == NULL) {
        // Send error: missing action
        MCP_Content* result = MCP_ContentCreateObject();
        MCP_ContentAddBoolean(result, "success", false);
        MCP_ContentAddString(result, "error", "Missing required parameter: action");
        
        MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId, false, result);
        MCP_ContentFree(result);
        return -2;
    }
    
    MCP_Content* result = NULL;
    int status = 0;
    
    // Route based on action
    if (strcmp(action, "getConfig") == 0) {
        status = handle_get_config(sessionId, operationId, params, &result);
    } 
    else if (strcmp(action, "setConfig") == 0) {
        status = handle_set_config(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "enableLogging") == 0) {
        status = handle_enable_logging(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "disableLogging") == 0) {
        status = handle_disable_logging(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "setLevel") == 0) {
        status = handle_set_level(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "addModule") == 0) {
        status = handle_add_module(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "removeModule") == 0) {
        status = handle_remove_module(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "clearModules") == 0) {
        status = handle_clear_modules(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "enableModuleFilter") == 0) {
        status = handle_enable_module_filter(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "disableModuleFilter") == 0) {
        status = handle_disable_module_filter(sessionId, operationId, params, &result);
    }
    else {
        // Unknown action
        if (result == NULL) {
            result = MCP_ContentCreateObject();
        }
        MCP_ContentAddBoolean(result, "success", false);
        MCP_ContentAddString(result, "error", "Unknown action");
        
        MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId, false, result);
        MCP_ContentFree(result);
        return -3;
    }
    
    // Send result
    if (result != NULL) {
        MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId, 
                         status >= 0, result);
        MCP_ContentFree(result);
    }
    
    return status;
}

/**
 * @brief Register logging tool with the tool registry
 */
int MCP_LoggingToolRegister(void) {
    MCP_ToolInfo toolInfo = {
        .name = MCP_LOGGING_TOOL_NAME,
        .description = "Configure logging options for the MCP server",
        .schemaJson = s_toolSchema,
        .init = MCP_LoggingToolInit,
        .deinit = MCP_LoggingToolDeinit,
        .invoke = MCP_LoggingToolInvoke
    };
    
    return MCP_ToolRegister(&toolInfo);
}

/**
 * @brief Handle getConfig action
 */
static int handle_get_config(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    MCP_ContentAddBoolean(*result, "success", true);
    
    // Get current configuration
    MCP_LogConfig config;
    memset(&config, 0, sizeof(config));
    int status = MCP_LoggingGetConfig(&config);
    
    if (status < 0) {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Failed to get logging configuration");
        return status;
    }
    
    // Create configuration object
    MCP_Content* configObj = MCP_ContentCreateObject();
    if (configObj == NULL) {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Failed to create configuration object");
        return -2;
    }
    
    // Add configuration properties
    MCP_ContentAddBoolean(configObj, "enabled", config.enabled);
    MCP_ContentAddNumber(configObj, "maxLevel", (double)config.maxLevel);
    MCP_ContentAddNumber(configObj, "outputs", (double)config.outputs);
    MCP_ContentAddBoolean(configObj, "includeTimestamp", config.includeTimestamp);
    MCP_ContentAddBoolean(configObj, "includeLevelName", config.includeLevelName);
    MCP_ContentAddBoolean(configObj, "includeModuleName", config.includeModuleName);
    MCP_ContentAddBoolean(configObj, "filterByModule", config.filterByModule);
    
    // Add allowed modules
    MCP_Content* modulesArray = MCP_ContentCreateArray();
    if (modulesArray != NULL) {
        for (uint32_t i = 0; i < config.allowedModuleCount; i++) {
            if (config.allowedModules != NULL && config.allowedModules[i] != NULL) {
                MCP_ContentAddArrayString(modulesArray, config.allowedModules[i]);
            }
        }
        
        MCP_ContentAddArray(configObj, "allowedModules", modulesArray);
    }
    
    // Add configuration object to result
    MCP_ContentAddObject(*result, "config", configObj);
    
    // Free allocated memory
    if (config.allowedModules != NULL) {
        for (uint32_t i = 0; i < config.allowedModuleCount; i++) {
            free(config.allowedModules[i]);
        }
        free(config.allowedModules);
    }
    
    return 0;
}

/**
 * @brief Handle setConfig action
 */
static int handle_set_config(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    // Get configuration object from params
    MCP_Content* configObj = NULL;
    if (!MCP_ContentGetObject(params, "config", &configObj) || configObj == NULL) {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Missing config object");
        return -2;
    }
    
    // Parse configuration
    MCP_LogConfig config;
    memset(&config, 0, sizeof(config));
    
    // Get basic configuration fields
    MCP_ContentGetBoolean(configObj, "enabled", &config.enabled);
    
    double maxLevel = (double)LOG_LEVEL_INFO;
    MCP_ContentGetNumber(configObj, "maxLevel", &maxLevel);
    config.maxLevel = (LogLevel)(int)maxLevel;
    
    double outputs = (double)(LOG_OUTPUT_SERIAL | LOG_OUTPUT_MEMORY);
    MCP_ContentGetNumber(configObj, "outputs", &outputs);
    config.outputs = (uint32_t)outputs;
    
    MCP_ContentGetBoolean(configObj, "includeTimestamp", &config.includeTimestamp);
    MCP_ContentGetBoolean(configObj, "includeLevelName", &config.includeLevelName);
    MCP_ContentGetBoolean(configObj, "includeModuleName", &config.includeModuleName);
    MCP_ContentGetBoolean(configObj, "filterByModule", &config.filterByModule);
    
    // Get allowed modules
    MCP_Content* modulesArray = NULL;
    config.allowedModuleCount = 0;
    config.allowedModules = NULL;
    
    if (MCP_ContentGetArray(configObj, "allowedModules", &modulesArray) && modulesArray != NULL) {
        uint32_t count = MCP_ContentGetArrayLength(modulesArray);
        if (count > 0) {
            config.allowedModules = (char**)malloc(sizeof(char*) * count);
            if (config.allowedModules == NULL) {
                MCP_ContentAddBoolean(*result, "success", false);
                MCP_ContentAddString(*result, "error", "Memory allocation failed");
                return -3;
            }
            
            for (uint32_t i = 0; i < count; i++) {
                const char* module = NULL;
                if (MCP_ContentGetArrayStringAt(modulesArray, i, &module) && module != NULL) {
                    config.allowedModules[config.allowedModuleCount] = strdup(module);
                    if (config.allowedModules[config.allowedModuleCount] != NULL) {
                        config.allowedModuleCount++;
                    }
                }
            }
        }
    }
    
    // Apply configuration
    int status = MCP_LoggingSetConfig(&config);
    
    // Free allocated memory
    if (config.allowedModules != NULL) {
        for (uint32_t i = 0; i < config.allowedModuleCount; i++) {
            free(config.allowedModules[i]);
        }
        free(config.allowedModules);
    }
    
    if (status < 0) {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Failed to set logging configuration");
        return status;
    }
    
    MCP_ContentAddBoolean(*result, "success", true);
    
    // Broadcast configuration change to clients
    MCP_LoggingHandleConfig(sessionId, operationId, configObj);
    
    return 0;
}

/**
 * @brief Handle enableLogging action
 */
static int handle_enable_logging(const char* sessionId, const char* operationId, 
                             const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    bool previousState = MCP_LoggingEnable(true);
    
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddBoolean(*result, "enabled", true);
    MCP_ContentAddBoolean(*result, "previousState", previousState);
    
    // Inform clients about configuration change
    MCP_Content* configContent = MCP_ContentCreateObject();
    MCP_ContentAddBoolean(configContent, "enabled", true);
    MCP_LoggingHandleConfig(sessionId, operationId, configContent);
    MCP_ContentFree(configContent);
    
    return 0;
}

/**
 * @brief Handle disableLogging action
 */
static int handle_disable_logging(const char* sessionId, const char* operationId, 
                              const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    bool previousState = MCP_LoggingEnable(false);
    
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddBoolean(*result, "enabled", false);
    MCP_ContentAddBoolean(*result, "previousState", previousState);
    
    // Inform clients about configuration change
    MCP_Content* configContent = MCP_ContentCreateObject();
    MCP_ContentAddBoolean(configContent, "enabled", false);
    MCP_LoggingHandleConfig(sessionId, operationId, configContent);
    MCP_ContentFree(configContent);
    
    return 0;
}

/**
 * @brief Handle setLevel action
 */
static int handle_set_level(const char* sessionId, const char* operationId, 
                         const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    // Get level from params
    const char* levelStr = NULL;
    #if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
    if (!MCP_ContentGetStringValue(params, "level", &levelStr) || levelStr == NULL) {
    #else
    // Get level string from params using an alternative method for non-Arduino platforms
    levelStr = MCP_ContentGetString(params);
    if (levelStr == NULL) {
    #endif
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Missing level parameter");
        return -2;
    }
    
    // Convert string level to LogLevel enum
    LogLevel level = LOG_LEVEL_INFO;
    if (strcmp(levelStr, "none") == 0) {
        level = LOG_LEVEL_NONE;
    } else if (strcmp(levelStr, "error") == 0) {
        level = LOG_LEVEL_ERROR;
    } else if (strcmp(levelStr, "warn") == 0) {
        level = LOG_LEVEL_WARN;
    } else if (strcmp(levelStr, "info") == 0) {
        level = LOG_LEVEL_INFO;
    } else if (strcmp(levelStr, "debug") == 0) {
        level = LOG_LEVEL_DEBUG;
    } else if (strcmp(levelStr, "trace") == 0) {
        level = LOG_LEVEL_TRACE;
    } else {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Invalid log level");
        return -3;
    }
    
    // Set log level
    LogLevel previousLevel = MCP_LoggingSetMaxLevel(level);
    
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddString(*result, "level", levelStr);
    MCP_ContentAddString(*result, "previousLevel", log_level_name(previousLevel));
    
    // Inform clients about configuration change
    MCP_Content* configContent = MCP_ContentCreateObject();
    MCP_ContentAddNumber(configContent, "maxLevel", (double)level);
    MCP_LoggingHandleConfig(sessionId, operationId, configContent);
    MCP_ContentFree(configContent);
    
    return 0;
}

/**
 * @brief Handle addModule action
 */
static int handle_add_module(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    // Get module name from params
    const char* moduleName = NULL;
    if (!MCP_ContentGetStringValue(params, "moduleName", &moduleName) || moduleName == NULL) {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Missing moduleName parameter");
        return -2;
    }
    
    // Add module to allowed list
    int status = MCP_LoggingAddAllowedModule(moduleName);
    if (status < 0) {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Failed to add module to allowed list");
        return status;
    }
    
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddString(*result, "moduleName", moduleName);
    
    // Get updated configuration
    MCP_LogConfig config;
    memset(&config, 0, sizeof(config));
    MCP_LoggingGetConfig(&config);
    
    // Inform clients about configuration change
    MCP_Content* configContent = MCP_ContentCreateObject();
    MCP_Content* modulesArray = MCP_ContentCreateArray();
    
    for (uint32_t i = 0; i < config.allowedModuleCount; i++) {
        if (config.allowedModules != NULL && config.allowedModules[i] != NULL) {
            MCP_ContentAddArrayString(modulesArray, config.allowedModules[i]);
        }
    }
    
    MCP_ContentAddArray(configContent, "allowedModules", modulesArray);
    MCP_LoggingHandleConfig(sessionId, operationId, configContent);
    MCP_ContentFree(configContent);
    
    // Free allocated memory
    if (config.allowedModules != NULL) {
        for (uint32_t i = 0; i < config.allowedModuleCount; i++) {
            free(config.allowedModules[i]);
        }
        free(config.allowedModules);
    }
    
    return 0;
}

/**
 * @brief Handle removeModule action
 */
static int handle_remove_module(const char* sessionId, const char* operationId, 
                             const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    // Get module name from params
    const char* moduleName = NULL;
    if (!MCP_ContentGetStringValue(params, "moduleName", &moduleName) || moduleName == NULL) {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Missing moduleName parameter");
        return -2;
    }
    
    // Remove module from allowed list
    int status = MCP_LoggingRemoveAllowedModule(moduleName);
    if (status < 0) {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Failed to remove module from allowed list");
        return status;
    }
    
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddString(*result, "moduleName", moduleName);
    
    // Get updated configuration
    MCP_LogConfig config;
    memset(&config, 0, sizeof(config));
    MCP_LoggingGetConfig(&config);
    
    // Inform clients about configuration change
    MCP_Content* configContent = MCP_ContentCreateObject();
    MCP_Content* modulesArray = MCP_ContentCreateArray();
    
    for (uint32_t i = 0; i < config.allowedModuleCount; i++) {
        if (config.allowedModules != NULL && config.allowedModules[i] != NULL) {
            MCP_ContentAddArrayString(modulesArray, config.allowedModules[i]);
        }
    }
    
    MCP_ContentAddArray(configContent, "allowedModules", modulesArray);
    MCP_LoggingHandleConfig(sessionId, operationId, configContent);
    MCP_ContentFree(configContent);
    
    // Free allocated memory
    if (config.allowedModules != NULL) {
        for (uint32_t i = 0; i < config.allowedModuleCount; i++) {
            free(config.allowedModules[i]);
        }
        free(config.allowedModules);
    }
    
    return 0;
}

/**
 * @brief Handle clearModules action
 */
static int handle_clear_modules(const char* sessionId, const char* operationId, 
                             const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    // Clear allowed modules
    int status = MCP_LoggingClearAllowedModules();
    if (status < 0) {
        MCP_ContentAddBoolean(*result, "success", false);
        MCP_ContentAddString(*result, "error", "Failed to clear allowed modules");
        return status;
    }
    
    MCP_ContentAddBoolean(*result, "success", true);
    
    // Inform clients about configuration change
    MCP_Content* configContent = MCP_ContentCreateObject();
    MCP_Content* modulesArray = MCP_ContentCreateArray();
    MCP_ContentAddArray(configContent, "allowedModules", modulesArray);
    MCP_LoggingHandleConfig(sessionId, operationId, configContent);
    MCP_ContentFree(configContent);
    
    return 0;
}

/**
 * @brief Handle enableModuleFilter action
 */
static int handle_enable_module_filter(const char* sessionId, const char* operationId, 
                                   const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    bool previousState = MCP_LoggingSetFilterByModule(true);
    
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddBoolean(*result, "filterByModule", true);
    MCP_ContentAddBoolean(*result, "previousState", previousState);
    
    // Inform clients about configuration change
    MCP_Content* configContent = MCP_ContentCreateObject();
    MCP_ContentAddBoolean(configContent, "filterByModule", true);
    MCP_LoggingHandleConfig(sessionId, operationId, configContent);
    MCP_ContentFree(configContent);
    
    return 0;
}

/**
 * @brief Handle disableModuleFilter action
 */
static int handle_disable_module_filter(const char* sessionId, const char* operationId, 
                                    const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    bool previousState = MCP_LoggingSetFilterByModule(false);
    
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddBoolean(*result, "filterByModule", false);
    MCP_ContentAddBoolean(*result, "previousState", previousState);
    
    // Inform clients about configuration change
    MCP_Content* configContent = MCP_ContentCreateObject();
    MCP_ContentAddBoolean(configContent, "filterByModule", false);
    MCP_LoggingHandleConfig(sessionId, operationId, configContent);
    MCP_ContentFree(configContent);
    
    return 0;
}
#endif /* !MCP_PLATFORM_HOST && !MCP_OS_HOST */
