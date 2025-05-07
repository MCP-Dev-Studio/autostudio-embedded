#include "core/tool_system/bytecode_config.h"
#include "core/tool_system/bytecode_mem.h"
#include "core/tool_system/bytecode_interpreter.h"
#include "core/mcp/content.h"
#include "core/mcp/server.h"
#include "core/tool_system/tool_registry.h"
#include "system/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Example of bytecode configuration and memory management
 * 
 * This example demonstrates:
 * 1. How to initialize bytecode configuration
 * 2. How to use the bytecode memory management system
 * 3. How to allocate and free bytecode programs with configurable limits
 */

// Mock function to simulate MCP tool invocation
static void invoke_bytecode_config_tool(const char* action, const char* configJson) {
    printf("===== Invoking bytecode config tool with action: %s =====\n", action);
    
    // Create mock session and operation IDs
    const char* sessionId = "test-session";
    const char* operationId = "test-operation";
    
    // Create content with action
    MCP_Content* params = MCP_ContentCreateObject();
    MCP_ContentAddString(params, "action", action);
    
    // Parse additional JSON if provided
    if (configJson != NULL && configJson[0] != '\0') {
        MCP_Content* jsonContent = MCP_ContentCreateFromJson(configJson, strlen(configJson));
        if (jsonContent != NULL) {
            // Add config object if present
            MCP_Content* configObj = NULL;
            if (MCP_ContentGetObject(jsonContent, "config", &configObj)) {
                MCP_ContentAddObject(params, "config", configObj);
            }
            
            MCP_ContentFree(jsonContent);
        }
    }
    
    // Call the bytecode config tool handler
    MCP_Content* result = NULL;
    if (strcmp(action, "getConfig") == 0) {
        handle_get_config(sessionId, operationId, params, &result);
    } 
    else if (strcmp(action, "setConfig") == 0) {
        handle_set_config(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "resetConfig") == 0) {
        handle_reset_config(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "getRecommended") == 0) {
        handle_get_recommended(sessionId, operationId, params, &result);
    }
    else if (strcmp(action, "getStats") == 0) {
        handle_get_stats(sessionId, operationId, params, &result);
    }
    
    // Print result (would normally be sent to client)
    if (result != NULL) {
        char resultJson[2048];
        MCP_ContentSerializeJson(result, resultJson, sizeof(resultJson));
        printf("Result: %s\n", resultJson);
        MCP_ContentFree(result);
    }
    
    MCP_ContentFree(params);
    printf("\n");
}

// Example of using the bytecode memory management
static void demonstrate_bytecode_memory_management() {
    printf("===== Demonstrating bytecode memory management =====\n");
    
    // Initialize bytecode memory management
    MCP_BytecodeMemInit();
    
    // Allocate some memory
    printf("Allocating memory blocks...\n");
    void* block1 = MCP_BytecodeMemAlloc(1024);
    void* block2 = MCP_BytecodeMemAlloc(2048);
    void* block3 = MCP_BytecodeMemAlloc(4096);
    
    // Print memory statistics
    printf("Total allocated: %zu bytes\n", MCP_BytecodeConfigGetTotalAllocated());
    
    // Allocate a bytecode program
    printf("Allocating bytecode program...\n");
    MCP_BytecodeProgram* program = MCP_BytecodeMemAllocProgram(100, 20, 10, 5, 3);
    
    if (program != NULL) {
        // Create a simple program
        program->instructionCount = 3;
        
        // PUSH 0 (success code)
        program->instructions[0].opcode = MCP_BYTECODE_OP_PUSH_NUM;
        program->instructions[0].operand.numberValue = 0;
        
        // PUSH "Hello, World!"
        program->instructions[1].opcode = MCP_BYTECODE_OP_PUSH_STR;
        program->instructions[1].operand.stringIndex = 0;
        
        // HALT
        program->instructions[2].opcode = MCP_BYTECODE_OP_HALT;
        
        // Add string to pool
        program->stringPoolSize = 1;
        program->stringPool[0] = strdup("Hello, World!");
        
        // Print program size
        printf("Program size: %zu bytes\n", MCP_BytecodeMemGetProgramSize(program));
        
        // Allocate execution context
        printf("Allocating execution context...\n");
        MCP_BytecodeContext* context = MCP_BytecodeMemAllocContext(64);
        
        if (context != NULL) {
            // Print context size
            printf("Context size: %zu bytes\n", MCP_BytecodeMemGetContextSize(context));
            
            // Free context
            printf("Freeing execution context...\n");
            MCP_BytecodeMemFreeContext(context);
        }
        
        // Free program
        printf("Freeing bytecode program...\n");
        MCP_BytecodeMemFreeProgram(program);
    }
    
    // Free memory blocks
    printf("Freeing memory blocks...\n");
    MCP_BytecodeMemFree(block1, 1024);
    MCP_BytecodeMemFree(block2, 2048);
    MCP_BytecodeMemFree(block3, 4096);
    
    // Print memory statistics
    printf("Total allocated after cleanup: %zu bytes\n", MCP_BytecodeConfigGetTotalAllocated());
    
    printf("\n");
}

// Test allocating a program that exceeds limits
static void test_exceed_limits() {
    printf("===== Testing exceeding bytecode limits =====\n");
    
    // Get current configuration
    MCP_BytecodeRuntimeConfig config;
    MCP_BytecodeConfigGet(&config);
    
    // Try to allocate a program that exceeds instruction limit
    uint16_t tooManyInstructions = config.max_bytecode_size / sizeof(MCP_BytecodeInstruction) + 1;
    printf("Trying to allocate program with %u instructions (limit: %u)...\n",
          tooManyInstructions, config.max_bytecode_size / sizeof(MCP_BytecodeInstruction));
    
    MCP_BytecodeProgram* program = MCP_BytecodeMemAllocProgram(
        tooManyInstructions, 10, 10, 5, 3);
    
    if (program == NULL) {
        printf("Allocation correctly failed due to instruction limit\n");
    } else {
        printf("WARNING: Allocation succeeded despite exceeding limit!\n");
        MCP_BytecodeMemFreeProgram(program);
    }
    
    // Try to allocate a program that exceeds string pool limit
    uint16_t tooManyStrings = config.max_string_pool_size + 1;
    printf("Trying to allocate program with %u strings (limit: %u)...\n",
          tooManyStrings, config.max_string_pool_size);
    
    program = MCP_BytecodeMemAllocProgram(
        100, tooManyStrings, 10, 5, 3);
    
    if (program == NULL) {
        printf("Allocation correctly failed due to string pool limit\n");
    } else {
        printf("WARNING: Allocation succeeded despite exceeding limit!\n");
        MCP_BytecodeMemFreeProgram(program);
    }
    
    // Try to allocate an execution context that exceeds stack limit
    uint16_t tooLargeStack = config.max_stack_size + 1;
    printf("Trying to allocate context with stack size %u (limit: %u)...\n",
          tooLargeStack, config.max_stack_size);
    
    MCP_BytecodeContext* context = MCP_BytecodeMemAllocContext(tooLargeStack);
    
    if (context == NULL) {
        printf("Allocation correctly failed due to stack limit\n");
    } else {
        printf("WARNING: Allocation succeeded despite exceeding limit!\n");
        MCP_BytecodeMemFreeContext(context);
    }
    
    printf("\n");
}

int main(void) {
    // Initialize logging
    LogConfig logConfig = {
        .level = LOG_LEVEL_DEBUG,
        .outputs = LOG_OUTPUT_SERIAL,
        .includeTimestamp = true,
        .includeLevelName = true,
        .includeModuleName = true,
        .colorOutput = true
    };
    log_init(&logConfig);
    
    // Initialize bytecode configuration
    MCP_BytecodeConfigInit();
    
    // Demonstrate configuration tool usage
    
    // 1. Get current configuration
    invoke_bytecode_config_tool("getConfig", "");
    
    // 2. Get recommended configuration
    invoke_bytecode_config_tool("getRecommended", "");
    
    // 3. Set a custom configuration
    const char* configJson = "{"
        "\"config\": {"
            "\"max_bytecode_size\": 32768,"
            "\"max_stack_size\": 256,"
            "\"max_string_pool_size\": 512,"
            "\"max_variable_count\": 128,"
            "\"max_function_count\": 64,"
            "\"max_execution_time_ms\": 2000,"
            "\"dynamic_allocation\": true,"
            "\"total_memory_limit\": 65536"
        "}"
    "}";
    invoke_bytecode_config_tool("setConfig", configJson);
    
    // 4. Get memory statistics
    invoke_bytecode_config_tool("getStats", "");
    
    // Demonstrate bytecode memory management
    demonstrate_bytecode_memory_management();
    
    // Test exceeding limits
    test_exceed_limits();
    
    // Reset configuration to defaults
    invoke_bytecode_config_tool("resetConfig", "");
    
    return 0;
}