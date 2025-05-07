#include "core/mcp/logging/mcp_logging.h"
#include "core/mcp/logging/mcp_logging_client.h"
#include "core/mcp/server.h"
#include "core/mcp/server_utils.h"
#include "system/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Example of MCP log export functionality
 * 
 * This example demonstrates:
 * 1. How to initialize the MCP logging bridge on the server
 * 2. How to configure client-side log handling
 * 3. How to generate logs that will be sent via MCP
 */

// Custom log handler for demonstration
void custom_log_handler(LogLevel level, const char* module, const char* message, uint32_t timestamp) {
    printf("CUSTOM HANDLER: [%s] %s\n", module, message);
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
        .deviceName = "LogExampleDevice",
        .version = "1.0.0",
        .maxSessions = 5,
        .maxOperationsPerSession = 10,
        .maxContentSize = 4096,
        .sessionTimeout = 30000,
        .enableTools = true,
        .enableResources = true,
        .enableEvents = true, // Important: events must be enabled
        .enableAutomation = false,
        .deviceId = "LOG_EXAMPLE_001",
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
    
    // Initialize client logging (for demonstration)
    // In a real application, this would be in the client code
    
    // Example 1: Default configuration (console output)
    MCP_LogClientInit(NULL);
    MCP_LogClientEnable(true);
    
    // Generate some logs
    LOG_INFO("MAIN", "MCP logging bridge initialized with default client config");
    LOG_DEBUG("MAIN", "This is a debug message");
    LOG_ERROR("MAIN", "This is an error message");
    
    // Example 2: Custom configuration (file output)
    MCP_LogClientConfig clientConfig = MCP_LogClientGetDefaultConfig();
    clientConfig.output = MCP_LOG_OUTPUT_FILE;
    clientConfig.filePath = "mcp_logs.txt";
    clientConfig.maxFileSize = 1024 * 1024; // 1MB
    
    MCP_LogClientSetConfig(&clientConfig);
    
    // Generate more logs
    LOG_INFO("MAIN", "MCP logging bridge now using file output");
    LOG_WARN("MAIN", "This is a warning message");
    
    // Example 3: Custom handler
    MCP_LogClientSetOutput(MCP_LOG_OUTPUT_CUSTOM);
    MCP_LogClientSetCustomHandler(custom_log_handler);
    
    // Generate more logs
    LOG_INFO("MAIN", "MCP logging bridge now using custom handler");
    LOG_ERROR("MAIN", "Custom handler should receive this error");
    
    // Example of filtering by log level
    MCP_LogClientSetLevel(LOG_LEVEL_ERROR);
    
    LOG_INFO("MAIN", "This info message should not appear due to level filter");
    LOG_ERROR("MAIN", "This error message should appear despite level filter");
    
    // Simulate log messages from different modules
    LOG_INFO("MODULE1", "Information from Module 1");
    LOG_ERROR("MODULE2", "Error from Module 2");
    LOG_DEBUG("MODULE3", "Debug from Module 3 (filtered out)");
    
    // Clean up
    MCP_LoggingDeinit();
    log_deinit();
    
    return 0;
}