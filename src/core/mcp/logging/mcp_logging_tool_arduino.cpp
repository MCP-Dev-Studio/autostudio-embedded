/**
 * @file mcp_logging_tool_arduino.cpp
 * @brief Arduino-specific implementation of logging tool
 */
#include "mcp_logging_tool.h"
#include "mcp_logging.h"
#include "../server.h"
#include "../content.h"
#include "../content_api_helpers.h"
#include "../../../system/logging.h"
#include <string.h>
#include <stdio.h>

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

// Forward declarations for handler functions
static MCP_Content* handle_get_status(void);
static int handle_set_level(const char* sessionId, const char* operationId, 
                         const MCP_Content* params, MCP_Content** result);
static int handle_add_module(const char* sessionId, const char* operationId, 
                          const MCP_Content* params, MCP_Content** result);
static int handle_clear(const char* sessionId, const char* operationId, 
                     const MCP_Content* params, MCP_Content** result);
static int handle_flush(const char* sessionId, const char* operationId, 
                     const MCP_Content* params, MCP_Content** result);

// Store current log settings
static struct {
    LogConfig config;
    bool initialized;
} s_log_state = {
    .initialized = false
};

// Tool schema in JSON format
static const char* s_logging_tool_schema = 
    "{"
    "  \"name\": \"logging\","
    "  \"description\": \"Logging system control tool\","
    "  \"parameters\": {"
    "    \"type\": \"object\","
    "    \"properties\": {"
    "      \"action\": {"
    "        \"type\": \"string\","
    "        \"enum\": [\"getStatus\", \"setLevel\", \"addModule\", \"clear\", \"flush\"],"
    "        \"description\": \"Action to perform\""
    "      },"
    "      \"level\": {"
    "        \"type\": \"string\","
    "        \"enum\": [\"none\", \"error\", \"warn\", \"info\", \"debug\", \"trace\"],"
    "        \"description\": \"Log level for setLevel action\""
    "      },"
    "      \"moduleName\": {"
    "        \"type\": \"string\","
    "        \"description\": \"Module name for addModule action\""
    "      }"
    "    },"
    "    \"required\": [\"action\"]"
    "  }"
    "}";

/**
 * @brief Initialize the logging tool
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingToolInit(void) {
    if (s_log_state.initialized) {
        return 0; // Already initialized
    }
    
    // Get current log configuration (if any)
    s_log_state.config.level = log_get_level();
    s_log_state.config.outputs = log_get_outputs();
    s_log_state.config.logFileName = NULL;
    s_log_state.config.maxFileSize = 0;
    s_log_state.config.maxMemoryEntries = 100;
    s_log_state.config.includeTimestamp = true;
    s_log_state.config.includeLevelName = true;
    s_log_state.config.includeModuleName = true;
    s_log_state.config.colorOutput = true;
    s_log_state.config.customLogCallback = NULL;
    
    s_log_state.initialized = true;
    
    // Print initialization message 
    log_info("Logging tool initialized successfully");
    
    return 0;
}

/**
 * @brief Handle tool invocation
 * 
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @param params Tool parameters
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoggingToolInvoke(const char* sessionId, const char* operationId, const MCP_Content* params) {
    // Initialize if not already done
    if (!s_log_state.initialized) {
        MCP_LoggingToolInit();
    }
    
    // Check parameters
    if (params == NULL) {
        log_error("Logging tool invoked with NULL parameters");
        return -1;
    }
    
    // Get action parameter
    const char* action = NULL;
    if (!MCP_ContentGetStringValue(params, "action", &action) || action == NULL) {
        log_error("Logging tool invoked without action parameter");
        
        // Create error result
        MCP_Content* result = MCP_ContentCreateObject();
        if (result != NULL) {
            MCP_ContentAddBoolean(result, "success", false);
            MCP_ContentAddString(result, "error", "Missing action parameter");
            
            // Send result
            MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId, false, result);
            MCP_ContentFree(result);
        }
        return -2;
    }
    
    // Handle different actions
    if (strcmp(action, "getStatus") == 0) {
        // Get current status
        MCP_Content* result = handle_get_status();
        if (result == NULL) {
            log_error("Failed to get logging status");
            return -3;
        }
        
        // Send result
        MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId, true, result);
        MCP_ContentFree(result);
    }
    else if (strcmp(action, "setLevel") == 0) {
        // Set log level
        MCP_Content* result = NULL;
        int rc = handle_set_level(sessionId, operationId, params, &result);
        if (rc != 0 || result == NULL) {
            log_error("Failed to set log level");
            return -4;
        }
        
        // Send result
        MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId, true, result);
        MCP_ContentFree(result);
    }
    else if (strcmp(action, "addModule") == 0) {
        // Add module
        MCP_Content* result = NULL;
        int rc = handle_add_module(sessionId, operationId, params, &result);
        if (rc != 0 || result == NULL) {
            log_error("Failed to add module");
            return -5;
        }
        
        // Send result
        MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId,
                         true, result);
        MCP_ContentFree(result);
    }
    else if (strcmp(action, "clear") == 0) {
        // Clear log
        MCP_Content* result = NULL;
        int rc = handle_clear(sessionId, operationId, params, &result);
        if (rc != 0 || result == NULL) {
            log_error("Failed to clear log");
            return -6;
        }
        
        // Send result
        MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId,
                         true, result);
        MCP_ContentFree(result);
    }
    else if (strcmp(action, "flush") == 0) {
        // Flush log
        MCP_Content* result = NULL;
        int rc = handle_flush(sessionId, operationId, params, &result);
        if (rc != 0 || result == NULL) {
            log_error("Failed to flush log");
            return -7;
        }
        
        // Send result
        MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId,
                         true, result);
        MCP_ContentFree(result);
    }
    else {
        // Unknown action
        log_error("Unknown logging tool action: %s", action);
        
        // Create error result
        MCP_Content* result = MCP_ContentCreateObject();
        if (result != NULL) {
            MCP_ContentAddBoolean(result, "success", false);
            MCP_ContentAddString(result, "error", "Unknown action");
            
            // Send result
            MCP_SendToolResult(MCP_GetServer()->transport, sessionId, operationId,
                             false, result);
            MCP_ContentFree(result);
        }
        return -8;
    }
    
    return 0;
}

/**
 * @brief Get tool schema
 * 
 * @return const char* Tool schema JSON
 */
const char* MCP_LoggingToolGetSchema(void) {
    return s_logging_tool_schema;
}

/**
 * @brief Handle getStatus action
 */
static MCP_Content* handle_get_status(void) {
    MCP_Content* result = MCP_ContentCreateObject();
    if (result == NULL) {
        return NULL;
    }
    
    // Add success flag
    MCP_ContentAddBoolean(result, "success", true);
    
    // Add current log level
    const char* levelName = NULL;
    switch (log_get_level()) {
        case LOG_LEVEL_NONE:  levelName = "none"; break;
        case LOG_LEVEL_ERROR: levelName = "error"; break;
        case LOG_LEVEL_WARN:  levelName = "warn"; break;
        case LOG_LEVEL_INFO:  levelName = "info"; break;
        case LOG_LEVEL_DEBUG: levelName = "debug"; break;
        case LOG_LEVEL_TRACE: levelName = "trace"; break;
        default:              levelName = "unknown"; break;
    }
    MCP_ContentAddString(result, "level", levelName);
    
    // Add output flags
    uint32_t outputs = log_get_outputs();
    bool consoleOutput = (outputs & LOG_OUTPUT_CONSOLE) != 0;
    bool fileOutput = (outputs & LOG_OUTPUT_FILE) != 0;
    bool serialOutput = (outputs & LOG_OUTPUT_SERIAL) != 0;
    bool memoryOutput = (outputs & LOG_OUTPUT_MEMORY) != 0;
    
    MCP_ContentAddBoolean(result, "consoleOutput", consoleOutput);
    MCP_ContentAddBoolean(result, "fileOutput", fileOutput);
    MCP_ContentAddBoolean(result, "serialOutput", serialOutput);
    MCP_ContentAddBoolean(result, "memoryOutput", memoryOutput);
    
    // Add memory log stats
    int entryCount = log_get_memory_entry_count();
    MCP_ContentAddNumber(result, "memoryEntryCount", entryCount);
    
    return result;
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
    if (!MCP_ContentGetStringValue(params, "level", &levelStr) || levelStr == NULL) {
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
        MCP_ContentAddString(*result, "error", "Invalid level parameter");
        return -3;
    }
    
    // Set log level
    LogLevel previousLevel = log_set_level(level);
    
    // Update internal config
    s_log_state.config.level = level;
    
    // Set success result
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddString(*result, "level", levelStr);
    
    // Log the change
    log_info("Log level changed from %s to %s", 
           log_level_name(previousLevel), 
           log_level_name(level));
    
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
    
    // In the stub implementation, we just acknowledge the module
    // In a real implementation, we might register it internally
    
    // Set success result
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddString(*result, "moduleName", moduleName);
    
    // Log the action
    log_info("Module '%s' added to logging system", moduleName);
    
    return 0;
}

/**
 * @brief Handle clear action
 */
static int handle_clear(const char* sessionId, const char* operationId, 
                     const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    // Clear memory log entries
    int count = log_clear_memory_entries();
    
    // Set success result
    MCP_ContentAddBoolean(*result, "success", true);
    MCP_ContentAddNumber(*result, "clearedEntries", count);
    
    // Log the action
    log_info("Cleared %d log entries", count);
    
    return 0;
}

/**
 * @brief Handle flush action
 */
static int handle_flush(const char* sessionId, const char* operationId, 
                     const MCP_Content* params, MCP_Content** result) {
    *result = MCP_ContentCreateObject();
    if (*result == NULL) {
        return -1;
    }
    
    // Flush log
    int rc = log_flush();
    
    // Set success result
    MCP_ContentAddBoolean(*result, "success", rc == 0);
    if (rc != 0) {
        MCP_ContentAddString(*result, "error", "Failed to flush log");
    }
    
    // Log the action
    log_info("Log flushed with result: %d", rc);
    
    return 0;
}

/**
 * @brief Get the tool info structure
 */
const MCP_ToolInfo* MCP_LoggingToolGetInfo(void) {
    static const MCP_ToolInfo info = {
        .name = "logging",
        .description = "Logging system control tool",
        .schemaJson = s_logging_tool_schema,
        .init = MCP_LoggingToolInit,
        .deinit = NULL,
        .invoke = MCP_LoggingToolInvoke
    };
    
    return &info;
}

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)