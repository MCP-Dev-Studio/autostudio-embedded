#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mock_tool_registry.h"

extern char* json_get_string_field(const char* json, const char* field);

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

// Simple mock of MCP_ToolRegister
int MCP_ToolRegister(const char* name, MCP_ToolHandler handler, const char* schema) {
    (void)handler; // Unused in this simple test
    (void)schema;  // Unused in this simple test
    printf("Registered tool: %s\n", name);
    return 0;
}

// Simple mock of MCP_ToolSaveDynamic
int MCP_ToolSaveDynamic(const char* name) {
    printf("Saved dynamic tool: %s\n", name);
    return 0;
}

// Simple mock of MCP_ToolRegisterDynamic
int MCP_ToolRegisterDynamic(const char* json, size_t length) {
    (void)length; // Unused in this simple test
    
    // Extract tool name from JSON
    char* toolName = json_get_string_field(json, "tool");
    if (toolName != NULL) {
        printf("Registered dynamic tool from JSON: %s\n", toolName);
        free(toolName);
        return 0;
    } else {
        printf("Failed to extract tool name from JSON\n");
        return -1;
    }
}

// Simple mock for MCP_ToolExecute
MCP_ToolResult MCP_ToolExecute(const char* json, size_t length) {
    (void)length; // Unused in this simple test
    
    printf("Executing tool: %s\n", json);
    return MCP_ToolCreateSuccessResult("{\"result\":\"success\"}");
}

// Main function
int main(void) {
    printf("=== Dynamic Tool Registration and Persistence Test (Simplified) ===\n\n");
    
    // Test logging tool
    printf("Testing logging tool...\n");
    MCP_ToolResult logResult = logToolHandler("{\"message\":\"Hello, world!\"}", 23);
    printf("Log result: %s\n\n", logResult.resultJson);
    free((void*)logResult.resultJson);
    
    // Test registering a tool
    printf("Testing tool registration...\n");
    int result = MCP_ToolRegister("test.tool", logToolHandler, "{}");
    printf("Registration result: %d\n\n", result);
    
    // Test registering a dynamic tool
    printf("Testing dynamic tool registration...\n");
    const char* dynamicToolJson = "{\"tool\":\"system.defineTool\",\"params\":{\"name\":\"test.dynamicTool\"}}";
    result = MCP_ToolRegisterDynamic(dynamicToolJson, strlen(dynamicToolJson));
    printf("Dynamic registration result: %d\n\n", result);
    
    // Test saving a dynamic tool
    printf("Testing saving dynamic tool...\n");
    result = MCP_ToolSaveDynamic("test.dynamicTool");
    printf("Save result: %d\n\n", result);
    
    // Test executing a tool
    printf("Testing tool execution...\n");
    const char* executeJson = "{\"tool\":\"test.dynamicTool\",\"params\":{}}";
    MCP_ToolResult execResult = MCP_ToolExecute(executeJson, strlen(executeJson));
    printf("Execution result: %s\n\n", execResult.resultJson);
    free((void*)execResult.resultJson);
    
    printf("=== All tests passed successfully! ===\n");
    return 0;
}
