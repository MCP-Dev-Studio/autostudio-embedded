#include "core/mcp/logging/mcp_logging.h"
#include "core/mcp/logging/mcp_logging_tool.h"
#include "core/mcp/server.h"
#include "core/mcp/server_utils.h"
#include "core/tool_system/tool_registry.h"
#include "system/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Example of MCP log configuration functionality
 * 
 * This example demonstrates:
 * 1. How to initialize the MCP logging tool
 * 2. How to register the logging tool with the MCP server
 * 3. Sample invocations of the logging tool
 */

// Mock function to simulate tool invocation from client
static void invoke_logging_tool(const char* sessionId, const char* operationId, const char* action, const char* json) {
    printf("===== Invoking logging tool with action: %s =====\n", action);
    
    // Create content with action
    MCP_Content* params = MCP_ContentCreateObject();
    MCP_ContentAddString(params, "action", action);
    
    // Parse additional JSON if provided
    if (json != NULL && json[0] != '\0') {
        MCP_Content* jsonContent = MCP_ContentCreateFromJson(json, strlen(json));
        if (jsonContent != NULL) {
            // Merge JSON content with params
            // (This is a simplification - in a real implementation we'd merge properly)
            MCP_Content* configObj = NULL;
            if (MCP_ContentGetObject(jsonContent, "config", &configObj)) {
                MCP_ContentAddObject(params, "config", configObj);
            }
            
            const char* level = NULL;
            if (MCP_ContentGetString(jsonContent, "level", &level)) {
                MCP_ContentAddString(params, "level", level);
            }
            
            const char* moduleName = NULL;
            if (MCP_ContentGetString(jsonContent, "moduleName", &moduleName)) {
                MCP_ContentAddString(params, "moduleName", moduleName);
            }
            
            MCP_ContentFree(jsonContent);
        }
    }
    
    // Invoke tool
    MCP_LoggingToolInvoke(sessionId, operationId, params);
    
    MCP_ContentFree(params);
    printf("\n");
}

int main(void) {
    // Initialize logging system
    LogConfig logConfig = {
        .level = LOG_LEVEL_DEBUG,
        .outputs = LOG_OUTPUT_SERIAL | LOG_OUTPUT_MEMORY,
        .logFileName = NULL,
        .maxFileSize = 0,
        .maxMemoryEntries = 100,
        .includeTimestamp = true,
        .includeLevelName = true,
        .includeModuleName = true,
        .colorOutput = true,
        .customLogCallback = NULL
    };
    
    log_init(&logConfig);
    
    // Initialize MCP server (simplified for example)
    MCP_ServerConfig serverConfig = {
        .deviceName = "LogConfigExampleDevice",
        .version = "1.0.0",
        .maxSessions = 5,
        .maxOperationsPerSession = 10,
        .maxContentSize = 4096,
        .sessionTimeout = 30000,
        .enableTools = true,
        .enableResources = true,
        .enableEvents = true,
        .enableAutomation = false,
        .deviceId = "LOG_CONFIG_EXAMPLE_001",
        .maxTools = 10,
        .maxDrivers = 5,
        .initialOpenAccess = true,
        .enableUSB = true,
        .enableEthernet = false,
        .enableWifi = false,
        .enableBluetooth = false,
        .enableSerial = true
    };
    
    int result = MCP_ServerInit(&serverConfig);
    if (result < 0) {
        printf("Failed to initialize MCP server: %d\n", result);
        return 1;
    }
    
    MCP_Server* server = MCP_GetServer();
    if (server == NULL) {
        printf("Failed to get MCP server instance\n");
        return 1;
    }
    
    // Initialize MCP logging bridge
    result = MCP_LoggingInit(server, LOG_LEVEL_DEBUG);
    if (result < 0) {
        printf("Failed to initialize MCP logging bridge: %d\n", result);
        return 1;
    }
    
    // Enable MCP logging
    MCP_LoggingEnable(true);
    
    // Initialize tool registry (simplified)
    MCP_ToolRegistryInit(10);
    
    // Initialize and register logging tool
    MCP_LoggingToolInit();
    MCP_LoggingToolRegister();
    
    // Generate some logs to test with
    LOG_INFO("MAIN", "MCP logging tool initialized");
    LOG_DEBUG("MAIN", "This is a debug message");
    LOG_ERROR("NETWORK", "This is an error message from NETWORK module");
    
    // Simulate tool invocations from client
    const char* mockSessionId = "mock-session-1";
    const char* mockOpId = "mock-op-1";
    
    // Example 1: Get current configuration
    invoke_logging_tool(mockSessionId, mockOpId, "getConfig", "");
    
    // Example 2: Set log level
    invoke_logging_tool(mockSessionId, mockOpId, "setLevel", "{\"level\":\"debug\"}");
    
    // Generate more logs to see the effect
    LOG_DEBUG("SENSOR", "This debug message should now be visible");
    LOG_TRACE("MAIN", "This trace message should still be filtered out");
    
    // Example 3: Configure module filtering
    invoke_logging_tool(mockSessionId, mockOpId, "addModule", "{\"moduleName\":\"MAIN\"}");
    invoke_logging_tool(mockSessionId, mockOpId, "addModule", "{\"moduleName\":\"SENSOR\"}");
    invoke_logging_tool(mockSessionId, mockOpId, "enableModuleFilter", "");
    
    // Generate logs to test module filtering
    LOG_INFO("MAIN", "This message from MAIN should pass the filter");
    LOG_INFO("SENSOR", "This message from SENSOR should pass the filter");
    LOG_INFO("NETWORK", "This message from NETWORK should be filtered out");
    
    // Example 4: Set complete configuration
    const char* configJson = "{"
        "\"config\": {"
            "\"enabled\": true,"
            "\"maxLevel\": 3,"
            "\"outputs\": 3,"
            "\"includeTimestamp\": true,"
            "\"includeLevelName\": true,"
            "\"includeModuleName\": true,"
            "\"filterByModule\": false,"
            "\"allowedModules\": []"
        "}"
    "}";
    
    invoke_logging_tool(mockSessionId, mockOpId, "setConfig", configJson);
    
    // Generate final test logs
    LOG_INFO("MAIN", "Module filtering should now be disabled");
    LOG_INFO("NETWORK", "This message from NETWORK should now be visible");
    LOG_DEBUG("SENSOR", "This debug message should be filtered out due to maxLevel=INFO");
    
    // Clean up
    MCP_LoggingDeinit();
    log_deinit();
    
    return 0;
}