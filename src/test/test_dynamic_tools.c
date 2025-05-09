/**
 * @file test_dynamic_tools.c
 * @brief Test dynamic tool registration and persistence
 */
#include "core/tool_system/tool_registry.h"
#include "persistent_storage.h"
#include "logging.h"
#include "build_fix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mock implementation of json_get_string_field
char* json_get_string_field(const char* json, const char* field) {
    // Simple mock implementation
    if (json && field && strstr(json, field)) {
        return strdup("mock-value");
    }
    return NULL;
}

// Test JSON for a simple composite tool
const char* TEST_COMPOSITE_TOOL = "{"
    "\"tool\":\"system.defineTool\","
    "\"params\":{"
        "\"name\":\"test.compositeAction\","
        "\"description\":\"A composite test action\","
        "\"implementationType\":\"composite\","
        "\"implementation\":{"
            "\"steps\":["
                "{"
                    "\"tool\":\"system.log\","
                    "\"params\":{\"message\":\"Step 1 executed\"},"
                    "\"store\":\"step1Result\""
                "},"
                "{"
                    "\"tool\":\"system.log\","
                    "\"params\":{\"message\":\"Step 2 executed with result: {{step1Result}}\"},"
                    "\"store\":\"step2Result\""
                "}"
            "]"
        "},"
        "\"schema\":{"
            "\"properties\":{"
                "\"action\":{\"type\":\"string\"}"
            "}"
        "},"
        "\"persistent\":true"
    "}"
"}";

// Test system.log tool implementation
static MCP_ToolResult logToolHandler(const char* json, size_t length) {
    // Unused parameter
    (void)length;
    
    // Extract message from json
    char* message = json_get_string_field(json, "message");
    if (message != NULL) {
        printf("LOG: %s\n", message);
        free(message);
    }
    
    return MCP_ToolCreateSuccessResult("{\"status\":\"ok\"}");
}

/**
 * @brief Initialize the test environment
 */
int initializeTestEnvironment(void) {
    // Initialize memory manager and other dependencies
    printf("Initializing test environment...\n");
    
    // Initialize tool registry
    if (MCP_ToolRegistryInit(32) != 0) {
        printf("Failed to initialize tool registry\n");
        return -1;
    }
    
    // Register system.log tool
    if (MCP_ToolRegister_Legacy("system.log", logToolHandler, 
                          "{\"properties\":{\"message\":{\"type\":\"string\"}}}") != 0) {
        printf("Failed to register system.log tool\n");
        return -2;
    }
    
    // Initialize persistent storage with memory-backed implementation
    StorageConfig config = {
        .type = STORAGE_TYPE_EEPROM,
        .size = 32 * 1024,  // 32 KB
        .baseAddress = 0,
        .readOnly = false
    };
    
    if (persistent_storage_init(&config) != 0) {
        printf("Failed to initialize persistent storage\n");
        return -3;
    }
    
    return 0;
}

/**
 * @brief Test dynamic tool registration
 */
int testDynamicToolRegistration(void) {
    printf("\n==== Testing Dynamic Tool Registration ====\n");
    
    // Register a dynamic composite tool
    printf("Registering dynamic composite tool...\n");
    int result = MCP_ToolRegisterDynamic(TEST_COMPOSITE_TOOL, strlen(TEST_COMPOSITE_TOOL));
    
    if (result != 0) {
        printf("Failed to register dynamic tool, error: %d\n", result);
        return -1;
    }
    
    // Verify tool was registered
    const MCP_ToolDefinition* toolDef = MCP_ToolGetDefinition("test.compositeAction");
    if (toolDef == NULL) {
        printf("Failed to get tool definition\n");
        return -2;
    }
    
    printf("Tool registered successfully!\n");
    printf("Name: %s\n", toolDef->name);
    printf("Type: %d\n", toolDef->type);
    printf("Is Dynamic: %s\n", toolDef->isDynamic ? "true" : "false");
    printf("Is Persistent: %s\n", toolDef->persistent ? "true" : "false");
    
    // Execute the tool
    printf("\nExecuting the composite tool...\n");
    char testJson[256];
    snprintf(testJson, sizeof(testJson), 
             "{\"tool\":\"test.compositeAction\",\"params\":{\"action\":\"test\"}}");
    
    MCP_ToolResult result2 = MCP_ToolExecute(testJson, strlen(testJson));
    
    if (result2.status != MCP_TOOL_RESULT_SUCCESS) {
        printf("Tool execution failed, status: %d, error: %s\n", 
               result2.status, result2.resultJson);
        return -3;
    }
    
    printf("Tool executed successfully, result: %s\n", result2.resultJson);
    
    // Clean up
    if (result2.resultJson) {
        free((void*)result2.resultJson);
    }
    
    return 0;
}

/**
 * @brief Test tool persistence
 */
int testToolPersistence(void) {
    printf("\n==== Testing Tool Persistence ====\n");
    
    // Save all dynamic tools
    printf("Saving dynamic tools...\n");
    
    // We can either call MCP_ToolSaveDynamic or restart and let the registry load from storage
    int result = MCP_ToolSaveDynamic("test.compositeAction");
    
    if (result != 0) {
        printf("Failed to save dynamic tool, error: %d\n", result);
        return -1;
    }
    
    // Get list of registered tools
    char toolList[2048] = {0};
    result = MCP_ToolGetList(toolList, sizeof(toolList));
    
    if (result <= 0) {
        printf("Failed to get tool list, error: %d\n", result);
        return -2;
    }
    
    printf("Registered tools before reset: %s\n", toolList);
    
    // Simulate restart by reinitializing
    printf("\nSimulating system restart...\n");
    
    // Deinitialize
    persistent_storage_deinit();
    
    // Re-initialize persistent storage
    StorageConfig config = {
        .type = STORAGE_TYPE_EEPROM,
        .size = 32 * 1024,  // 32 KB
        .baseAddress = 0,
        .readOnly = false
    };
    
    if (persistent_storage_init(&config) != 0) {
        printf("Failed to reinitialize persistent storage\n");
        return -3;
    }
    
    // Re-initialize tool registry (this loads all dynamic tools from persistent storage)
    if (MCP_ToolRegistryInit(32) != 0) {
        printf("Failed to reinitialize tool registry\n");
        return -4;
    }
    
    // Register system.log tool again
    if (MCP_ToolRegister_Legacy("system.log", logToolHandler, 
                          "{\"properties\":{\"message\":{\"type\":\"string\"}}}") != 0) {
        printf("Failed to register system.log tool\n");
        return -5;
    }
    
    // Verify tool was loaded
    const MCP_ToolDefinition* toolDef = MCP_ToolGetDefinition("test.compositeAction");
    if (toolDef == NULL) {
        printf("Failed to load tool definition from persistent storage\n");
        return -6;
    }
    
    // Get list of registered tools
    memset(toolList, 0, sizeof(toolList));
    result = MCP_ToolGetList(toolList, sizeof(toolList));
    
    if (result <= 0) {
        printf("Failed to get tool list after restart, error: %d\n", result);
        return -7;
    }
    
    printf("Registered tools after reset: %s\n", toolList);
    
    // Execute the tool again
    printf("\nExecuting the reloaded composite tool...\n");
    char testJson[256];
    snprintf(testJson, sizeof(testJson), 
             "{\"tool\":\"test.compositeAction\",\"params\":{\"action\":\"test after restart\"}}");
    
    MCP_ToolResult result2 = MCP_ToolExecute(testJson, strlen(testJson));
    
    if (result2.status != MCP_TOOL_RESULT_SUCCESS) {
        printf("Tool execution failed after restart, status: %d, error: %s\n", 
               result2.status, result2.resultJson);
        return -8;
    }
    
    printf("Tool executed successfully after restart, result: %s\n", result2.resultJson);
    
    // Clean up
    if (result2.resultJson) {
        free((void*)result2.resultJson);
    }
    
    return 0;
}

/**
 * @brief Main test function
 */
int main(void) {
    printf("=== Dynamic Tool Registration and Persistence Test ===\n\n");
    
    // Initialize test environment
    if (initializeTestEnvironment() != 0) {
        printf("Test failed: Environment initialization failed\n");
        return 1;
    }
    
    // Test dynamic tool registration
    if (testDynamicToolRegistration() != 0) {
        printf("Test failed: Dynamic tool registration failed\n");
        return 2;
    }
    
    // Test tool persistence
    if (testToolPersistence() != 0) {
        printf("Test failed: Tool persistence failed\n");
        return 3;
    }
    
    printf("\n=== All tests passed successfully! ===\n");
    return 0;
}
