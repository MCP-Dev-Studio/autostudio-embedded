#include "arduino_compat.h"
#include "bytecode_mem.h"
#include "bytecode_interpreter.h"
#include "bytecode_config.h"
#include "logging.h"
#include <string.h>
#include <stdlib.h>

// Initialization flag
static bool g_initialized = false;

/**
 * @brief Initialize bytecode memory management
 */
int MCP_BytecodeMemInit(void) {
    if (g_initialized) {
        return 0; // Already initialized
    }
    
    // Make sure bytecode configuration is initialized
    MCP_BytecodeConfigInit();
    
    g_initialized = true;
    LOG_INFO("BYTECODE", "Initialized bytecode memory management");
    
    return 0;
}

/**
 * @brief Allocate memory for bytecode use
 */
void* MCP_BytecodeMemAlloc(size_t size) {
    if (!g_initialized) {
        if (MCP_BytecodeMemInit() != 0) {
            return NULL;
        }
    }
    
    // Check if allocation is allowed
    if (!MCP_BytecodeConfigCanAllocate(size)) {
        LOG_ERROR("BYTECODE", "Memory allocation of %zu bytes not allowed", size);
        return NULL;
    }
    
    // Allocate memory
    void* ptr = malloc(size);
    if (ptr == NULL) {
        LOG_ERROR("BYTECODE", "Failed to allocate %zu bytes", size);
        return NULL;
    }
    
    // Track allocation
    MCP_BytecodeConfigTrackAllocation(size);
    
    // Clear memory
    memset(ptr, 0, size);
    
    return ptr;
}

/**
 * @brief Reallocate memory for bytecode use
 */
void* MCP_BytecodeMemRealloc(void* ptr, size_t oldSize, size_t newSize) {
    if (!g_initialized) {
        if (MCP_BytecodeMemInit() != 0) {
            return NULL;
        }
    }
    
    // If ptr is NULL, this is just an allocation
    if (ptr == NULL) {
        return MCP_BytecodeMemAlloc(newSize);
    }
    
    // If new size is 0, this is just a free
    if (newSize == 0) {
        MCP_BytecodeMemFree(ptr, oldSize);
        return NULL;
    }
    
    // Calculate size difference
    size_t sizeDiff = (newSize > oldSize) ? (newSize - oldSize) : 0;
    
    // Check if additional allocation is allowed
    if (sizeDiff > 0 && !MCP_BytecodeConfigCanAllocate(sizeDiff)) {
        LOG_ERROR("BYTECODE", "Memory reallocation to %zu bytes not allowed", newSize);
        return NULL;
    }
    
    // Reallocate memory
    void* newPtr = realloc(ptr, newSize);
    if (newPtr == NULL) {
        LOG_ERROR("BYTECODE", "Failed to reallocate from %zu to %zu bytes", oldSize, newSize);
        return NULL;
    }
    
    // Track allocation change
    if (newSize > oldSize) {
        MCP_BytecodeConfigTrackAllocation(newSize - oldSize);
    } else if (newSize < oldSize) {
        MCP_BytecodeConfigTrackDeallocation(oldSize - newSize);
    }
    
    // Clear any new memory
    if (newSize > oldSize) {
        memset((uint8_t*)newPtr + oldSize, 0, newSize - oldSize);
    }
    
    return newPtr;
}

/**
 * @brief Free memory allocated for bytecode use
 */
void MCP_BytecodeMemFree(void* ptr, size_t size) {
    if (!g_initialized || ptr == NULL) {
        return;
    }
    
    // Free memory
    free(ptr);
    
    // Track deallocation
    MCP_BytecodeConfigTrackDeallocation(size);
}

/**
 * @brief Calculate the memory size of a bytecode program
 */
static size_t calculate_program_size(
    uint16_t instructionCapacity,
    uint16_t stringPoolCapacity,
    uint16_t variableCapacity,
    uint16_t propertyCapacity,
    uint16_t functionCapacity) {
    
    size_t size = sizeof(MCP_BytecodeProgram);
    
    // Instructions
    size += instructionCapacity * sizeof(MCP_BytecodeInstruction);
    
    // String pool pointers
    size += stringPoolCapacity * sizeof(char*);
    
    // Variable names pointers
    size += variableCapacity * sizeof(char*);
    
    // Property names pointers
    size += propertyCapacity * sizeof(char*);
    
    // Function names pointers
    size += functionCapacity * sizeof(char*);
    
    // This doesn't include the actual strings, which will be allocated separately
    
    return size;
}

/**
 * @brief Allocate memory for a bytecode program
 */
MCP_BytecodeProgram* MCP_BytecodeMemAllocProgram(
    uint16_t instructionCapacity, 
    uint16_t stringPoolCapacity,
    uint16_t variableCapacity,
    uint16_t propertyCapacity,
    uint16_t functionCapacity) {
    
    if (!g_initialized) {
        if (MCP_BytecodeMemInit() != 0) {
            return NULL;
        }
    }
    
    // Get bytecode configuration
    MCP_BytecodeRuntimeConfig config;
    MCP_BytecodeConfigGet(&config);
    
    // Validate capacities against configuration limits
    if (instructionCapacity > config.max_bytecode_size / sizeof(MCP_BytecodeInstruction)) {
        LOG_ERROR("BYTECODE", "Instruction capacity %u exceeds limit", instructionCapacity);
        return NULL;
    }
    
    if (stringPoolCapacity > config.max_string_pool_size) {
        LOG_ERROR("BYTECODE", "String pool capacity %u exceeds limit %u", 
                stringPoolCapacity, config.max_string_pool_size);
        return NULL;
    }
    
    if (variableCapacity > config.max_variable_count) {
        LOG_ERROR("BYTECODE", "Variable capacity %u exceeds limit %u", 
                variableCapacity, config.max_variable_count);
        return NULL;
    }
    
    // Calculate total size
    size_t totalSize = calculate_program_size(
        instructionCapacity, stringPoolCapacity, variableCapacity, 
        propertyCapacity, functionCapacity);
    
    // Check if allocation is allowed
    if (!MCP_BytecodeConfigCanAllocate(totalSize)) {
        LOG_ERROR("BYTECODE", "Program allocation of %zu bytes not allowed", totalSize);
        return NULL;
    }
    
    // Allocate program structure
    MCP_BytecodeProgram* program = (MCP_BytecodeProgram*)MCP_BytecodeMemAlloc(sizeof(MCP_BytecodeProgram));
    if (program == NULL) {
        return NULL;
    }
    
    // Allocate arrays
    program->instructions = (MCP_BytecodeInstruction*)MCP_BytecodeMemAlloc(
        instructionCapacity * sizeof(MCP_BytecodeInstruction));
    
    if (program->instructions == NULL) {
        MCP_BytecodeMemFree(program, sizeof(MCP_BytecodeProgram));
        return NULL;
    }
    
    if (stringPoolCapacity > 0) {
        program->stringPool = (char**)MCP_BytecodeMemAlloc(stringPoolCapacity * sizeof(char*));
        if (program->stringPool == NULL) {
            MCP_BytecodeMemFree(program->instructions, 
                              instructionCapacity * sizeof(MCP_BytecodeInstruction));
            MCP_BytecodeMemFree(program, sizeof(MCP_BytecodeProgram));
            return NULL;
        }
    } else {
        program->stringPool = NULL;
    }
    
    if (variableCapacity > 0) {
        program->variableNames = (char**)MCP_BytecodeMemAlloc(variableCapacity * sizeof(char*));
        if (program->variableNames == NULL) {
            if (program->stringPool != NULL) {
                MCP_BytecodeMemFree(program->stringPool, stringPoolCapacity * sizeof(char*));
            }
            MCP_BytecodeMemFree(program->instructions, 
                              instructionCapacity * sizeof(MCP_BytecodeInstruction));
            MCP_BytecodeMemFree(program, sizeof(MCP_BytecodeProgram));
            return NULL;
        }
    } else {
        program->variableNames = NULL;
    }
    
    if (propertyCapacity > 0) {
        program->propertyNames = (char**)MCP_BytecodeMemAlloc(propertyCapacity * sizeof(char*));
        if (program->propertyNames == NULL) {
            if (program->variableNames != NULL) {
                MCP_BytecodeMemFree(program->variableNames, variableCapacity * sizeof(char*));
            }
            if (program->stringPool != NULL) {
                MCP_BytecodeMemFree(program->stringPool, stringPoolCapacity * sizeof(char*));
            }
            MCP_BytecodeMemFree(program->instructions, 
                              instructionCapacity * sizeof(MCP_BytecodeInstruction));
            MCP_BytecodeMemFree(program, sizeof(MCP_BytecodeProgram));
            return NULL;
        }
    } else {
        program->propertyNames = NULL;
    }
    
    if (functionCapacity > 0) {
        program->functionNames = (char**)MCP_BytecodeMemAlloc(functionCapacity * sizeof(char*));
        if (program->functionNames == NULL) {
            if (program->propertyNames != NULL) {
                MCP_BytecodeMemFree(program->propertyNames, propertyCapacity * sizeof(char*));
            }
            if (program->variableNames != NULL) {
                MCP_BytecodeMemFree(program->variableNames, variableCapacity * sizeof(char*));
            }
            if (program->stringPool != NULL) {
                MCP_BytecodeMemFree(program->stringPool, stringPoolCapacity * sizeof(char*));
            }
            MCP_BytecodeMemFree(program->instructions, 
                              instructionCapacity * sizeof(MCP_BytecodeInstruction));
            MCP_BytecodeMemFree(program, sizeof(MCP_BytecodeProgram));
            return NULL;
        }
    } else {
        program->functionNames = NULL;
    }
    
    // Initialize counts
    program->instructionCount = 0;
    program->stringPoolSize = 0;
    program->variableCount = 0;
    program->propertyCount = 0;
    program->functionCount = 0;
    
    LOG_INFO("BYTECODE", "Allocated program with %u instruction capacity, %u string pool capacity",
            instructionCapacity, stringPoolCapacity);
    
    return program;
}

/**
 * @brief Free a bytecode program
 */
void MCP_BytecodeMemFreeProgram(MCP_BytecodeProgram* program) {
    if (!g_initialized || program == NULL) {
        return;
    }
    
    // Calculate program size for tracking
    size_t programSize = MCP_BytecodeMemGetProgramSize(program);
    
    // Free string pool strings
    if (program->stringPool != NULL) {
        for (uint16_t i = 0; i < program->stringPoolSize; i++) {
            if (program->stringPool[i] != NULL) {
                size_t strLen = strlen(program->stringPool[i]) + 1;
                MCP_BytecodeMemFree(program->stringPool[i], strLen);
            }
        }
        MCP_BytecodeMemFree(program->stringPool, program->stringPoolSize * sizeof(char*));
    }
    
    // Free variable names
    if (program->variableNames != NULL) {
        for (uint16_t i = 0; i < program->variableCount; i++) {
            if (program->variableNames[i] != NULL) {
                size_t strLen = strlen(program->variableNames[i]) + 1;
                MCP_BytecodeMemFree(program->variableNames[i], strLen);
            }
        }
        MCP_BytecodeMemFree(program->variableNames, program->variableCount * sizeof(char*));
    }
    
    // Free property names
    if (program->propertyNames != NULL) {
        for (uint16_t i = 0; i < program->propertyCount; i++) {
            if (program->propertyNames[i] != NULL) {
                size_t strLen = strlen(program->propertyNames[i]) + 1;
                MCP_BytecodeMemFree(program->propertyNames[i], strLen);
            }
        }
        MCP_BytecodeMemFree(program->propertyNames, program->propertyCount * sizeof(char*));
    }
    
    // Free function names
    if (program->functionNames != NULL) {
        for (uint16_t i = 0; i < program->functionCount; i++) {
            if (program->functionNames[i] != NULL) {
                size_t strLen = strlen(program->functionNames[i]) + 1;
                MCP_BytecodeMemFree(program->functionNames[i], strLen);
            }
        }
        MCP_BytecodeMemFree(program->functionNames, program->functionCount * sizeof(char*));
    }
    
    // Free instructions
    if (program->instructions != NULL) {
        MCP_BytecodeMemFree(program->instructions, program->instructionCount * sizeof(MCP_BytecodeInstruction));
    }
    
    // Free program structure
    MCP_BytecodeMemFree(program, sizeof(MCP_BytecodeProgram));
    
    LOG_INFO("BYTECODE", "Freed program of size %zu bytes", programSize);
}

/**
 * @brief Get the size of a bytecode program
 */
size_t MCP_BytecodeMemGetProgramSize(const MCP_BytecodeProgram* program) {
    if (program == NULL) {
        return 0;
    }
    
    size_t size = sizeof(MCP_BytecodeProgram);
    
    // Instructions
    size += program->instructionCount * sizeof(MCP_BytecodeInstruction);
    
    // String pool pointers
    size += program->stringPoolSize * sizeof(char*);
    
    // String pool strings
    if (program->stringPool != NULL) {
        for (uint16_t i = 0; i < program->stringPoolSize; i++) {
            if (program->stringPool[i] != NULL) {
                size += strlen(program->stringPool[i]) + 1;
            }
        }
    }
    
    // Variable names pointers
    size += program->variableCount * sizeof(char*);
    
    // Variable name strings
    if (program->variableNames != NULL) {
        for (uint16_t i = 0; i < program->variableCount; i++) {
            if (program->variableNames[i] != NULL) {
                size += strlen(program->variableNames[i]) + 1;
            }
        }
    }
    
    // Property names pointers
    size += program->propertyCount * sizeof(char*);
    
    // Property name strings
    if (program->propertyNames != NULL) {
        for (uint16_t i = 0; i < program->propertyCount; i++) {
            if (program->propertyNames[i] != NULL) {
                size += strlen(program->propertyNames[i]) + 1;
            }
        }
    }
    
    // Function names pointers
    size += program->functionCount * sizeof(char*);
    
    // Function name strings
    if (program->functionNames != NULL) {
        for (uint16_t i = 0; i < program->functionCount; i++) {
            if (program->functionNames[i] != NULL) {
                size += strlen(program->functionNames[i]) + 1;
            }
        }
    }
    
    return size;
}

/**
 * @brief Allocate an execution context
 */
MCP_BytecodeContext* MCP_BytecodeMemAllocContext(uint16_t stackSize) {
    if (!g_initialized) {
        if (MCP_BytecodeMemInit() != 0) {
            return NULL;
        }
    }
    
    // Get bytecode configuration
    MCP_BytecodeRuntimeConfig config;
    MCP_BytecodeConfigGet(&config);
    
    // Validate stack size against configuration limit
    if (stackSize > config.max_stack_size) {
        LOG_ERROR("BYTECODE", "Stack size %u exceeds limit %u", 
                stackSize, config.max_stack_size);
        return NULL;
    }
    
    // Calculate size
    size_t contextSize = sizeof(MCP_BytecodeContext);
    size_t stackSize_bytes = stackSize * sizeof(MCP_BytecodeValue);
    
    // Check if allocation is allowed
    if (!MCP_BytecodeConfigCanAllocate(contextSize + stackSize_bytes)) {
        LOG_ERROR("BYTECODE", "Context allocation of %zu bytes not allowed", 
                contextSize + stackSize_bytes);
        return NULL;
    }
    
    // Allocate context structure
    MCP_BytecodeContext* context = (MCP_BytecodeContext*)MCP_BytecodeMemAlloc(contextSize);
    if (context == NULL) {
        return NULL;
    }
    
    // Allocate stack
    context->stack = (MCP_BytecodeValue*)MCP_BytecodeMemAlloc(stackSize_bytes);
    if (context->stack == NULL) {
        MCP_BytecodeMemFree(context, contextSize);
        return NULL;
    }
    
    // Initialize context
    context->stackSize = stackSize;
    context->stackPointer = 0;
    context->pc = 0;
    context->halt = false;
    context->errorCode = 0;
    context->errorMessage = NULL;
    
    LOG_INFO("BYTECODE", "Allocated execution context with stack size %u", stackSize);
    
    return context;
}

/**
 * @brief Free an execution context
 */
void MCP_BytecodeMemFreeContext(MCP_BytecodeContext* context) {
    if (!g_initialized || context == NULL) {
        return;
    }
    
    // Free stack values
    if (context->stack != NULL) {
        // Free any dynamically allocated values in the stack
        for (uint16_t i = 0; i < context->stackPointer; i++) {
            if (context->stack[i].type == MCP_BYTECODE_VALUE_STRING && 
                context->stack[i].value.stringValue != NULL) {
                size_t strLen = strlen(context->stack[i].value.stringValue) + 1;
                MCP_BytecodeMemFree(context->stack[i].value.stringValue, strLen);
            }
            else if (context->stack[i].type == MCP_BYTECODE_VALUE_ARRAY && 
                    context->stack[i].value.arrayValue.items != NULL) {
                // Free array items
                size_t arraySize = context->stack[i].value.arrayValue.count * sizeof(MCP_BytecodeValue);
                MCP_BytecodeMemFree(context->stack[i].value.arrayValue.items, arraySize);
            }
            // Object values would need similar cleanup if implemented
        }
        
        // Free the stack
        MCP_BytecodeMemFree(context->stack, context->stackSize * sizeof(MCP_BytecodeValue));
    }
    
    // Free error message if present
    if (context->errorMessage != NULL) {
        size_t msgLen = strlen(context->errorMessage) + 1;
        MCP_BytecodeMemFree(context->errorMessage, msgLen);
    }
    
    // Free variables if present
    if (context->variables != NULL) {
        // Get variable count from program
        uint16_t variableCount = 0;
        if (context->program != NULL) {
            variableCount = context->program->variableCount;
        }
        
        // Free any dynamically allocated values in variables
        for (uint16_t i = 0; i < variableCount; i++) {
            if (context->variables[i].type == MCP_BYTECODE_VALUE_STRING && 
                context->variables[i].value.stringValue != NULL) {
                size_t strLen = strlen(context->variables[i].value.stringValue) + 1;
                MCP_BytecodeMemFree(context->variables[i].value.stringValue, strLen);
            }
            else if (context->variables[i].type == MCP_BYTECODE_VALUE_ARRAY && 
                    context->variables[i].value.arrayValue.items != NULL) {
                // Free array items
                size_t arraySize = context->variables[i].value.arrayValue.count * sizeof(MCP_BytecodeValue);
                MCP_BytecodeMemFree(context->variables[i].value.arrayValue.items, arraySize);
            }
            // Object values would need similar cleanup if implemented
        }
        
        // Free variables array
        MCP_BytecodeMemFree(context->variables, variableCount * sizeof(MCP_BytecodeValue));
    }
    
    // Free context structure
    MCP_BytecodeMemFree(context, sizeof(MCP_BytecodeContext));
    
    LOG_INFO("BYTECODE", "Freed execution context");
}

/**
 * @brief Get the size of an execution context
 */
size_t MCP_BytecodeMemGetContextSize(const MCP_BytecodeContext* context) {
    if (context == NULL) {
        return 0;
    }
    
    size_t size = sizeof(MCP_BytecodeContext);
    
    // Stack size
    size += context->stackSize * sizeof(MCP_BytecodeValue);
    
    // Stack values
    for (uint16_t i = 0; i < context->stackPointer; i++) {
        if (context->stack[i].type == MCP_BYTECODE_VALUE_STRING && 
            context->stack[i].value.stringValue != NULL) {
            size += strlen(context->stack[i].value.stringValue) + 1;
        }
        else if (context->stack[i].type == MCP_BYTECODE_VALUE_ARRAY && 
                context->stack[i].value.arrayValue.items != NULL) {
            // Array items
            size += context->stack[i].value.arrayValue.count * sizeof(MCP_BytecodeValue);
            
            // Recurse into array items if needed
            // This is simplified and doesn't account for nested arrays/objects
        }
        // Object values would need similar calculation if implemented
    }
    
    // Error message if present
    if (context->errorMessage != NULL) {
        size += strlen(context->errorMessage) + 1;
    }
    
    // Variables if present
    if (context->variables != NULL && context->program != NULL) {
        size += context->program->variableCount * sizeof(MCP_BytecodeValue);
        
        // Variable values
        for (uint16_t i = 0; i < context->program->variableCount; i++) {
            if (context->variables[i].type == MCP_BYTECODE_VALUE_STRING && 
                context->variables[i].value.stringValue != NULL) {
                size += strlen(context->variables[i].value.stringValue) + 1;
            }
            else if (context->variables[i].type == MCP_BYTECODE_VALUE_ARRAY && 
                    context->variables[i].value.arrayValue.items != NULL) {
                // Array items
                size += context->variables[i].value.arrayValue.count * sizeof(MCP_BytecodeValue);
                
                // Recurse into array items if needed
                // This is simplified and doesn't account for nested arrays/objects
            }
            // Object values would need similar calculation if implemented
        }
    }
    
    return size;
}
