#include "bytecode_interpreter.h"
#include <stdlib.h>
#include <string.h>

// Maximum stack size for bytecode execution
#define MAX_STACK_SIZE 64

// Execution context
typedef struct {
    const MCP_BytecodeProgram* program;
    MCP_BytecodeValue stack[MAX_STACK_SIZE];
    uint16_t stackTop;
    MCP_BytecodeValue* variables;
    uint16_t pc;  // Program counter
    bool running;
    uint16_t errorCode;
    char* errorMessage;
} BytecodeContext;

// Internal state
static bool s_initialized = false;

int MCP_BytecodeInterpreterInit(void) {
    if (s_initialized) {
        return -1;  // Already initialized
    }
    
    s_initialized = true;
    return 0;
}

static void initContext(BytecodeContext* ctx, const MCP_BytecodeProgram* program) {
    ctx->program = program;
    ctx->stackTop = 0;
    ctx->pc = 0;
    ctx->running = true;
    ctx->errorCode = 0;
    ctx->errorMessage = NULL;
    
    // Allocate variable storage
    ctx->variables = (MCP_BytecodeValue*)calloc(program->variableCount, sizeof(MCP_BytecodeValue));
    if (ctx->variables == NULL) {
        ctx->errorCode = 1;
        ctx->errorMessage = strdup("Memory allocation failed");
        ctx->running = false;
    }
}

static void freeContext(BytecodeContext* ctx) {
    if (ctx->variables != NULL) {
        // Free variable values
        for (uint16_t i = 0; i < ctx->program->variableCount; i++) {
            if (ctx->variables[i].type == MCP_BYTECODE_VALUE_STRING && 
                ctx->variables[i].value.stringValue != NULL) {
                free(ctx->variables[i].value.stringValue);
            }
        }
        
        free(ctx->variables);
    }
    
    if (ctx->errorMessage != NULL) {
        free(ctx->errorMessage);
    }
}

static void pushValue(BytecodeContext* ctx, MCP_BytecodeValue value) {
    if (ctx->stackTop >= MAX_STACK_SIZE) {
        ctx->errorCode = 2;
        ctx->errorMessage = strdup("Stack overflow");
        ctx->running = false;
        return;
    }
    
    ctx->stack[ctx->stackTop++] = value;
}

static MCP_BytecodeValue popValue(BytecodeContext* ctx) {
    if (ctx->stackTop == 0) {
        ctx->errorCode = 3;
        ctx->errorMessage = strdup("Stack underflow");
        ctx->running = false;
        return (MCP_BytecodeValue){ .type = MCP_BYTECODE_VALUE_NULL };
    }
    
    return ctx->stack[--ctx->stackTop];
}

static MCP_BytecodeValue copyValue(const MCP_BytecodeValue* value) {
    MCP_BytecodeValue result;
    result.type = value->type;
    
    switch (value->type) {
        case MCP_BYTECODE_VALUE_NUMBER:
            result.value.numberValue = value->value.numberValue;
            break;
            
        case MCP_BYTECODE_VALUE_BOOL:
            result.value.boolValue = value->value.boolValue;
            break;
            
        case MCP_BYTECODE_VALUE_STRING:
            if (value->value.stringValue != NULL) {
                result.value.stringValue = strdup(value->value.stringValue);
                if (result.value.stringValue == NULL) {
                    result.type = MCP_BYTECODE_VALUE_NULL;
                }
            } else {
                result.value.stringValue = NULL;
            }
            break;
            
        case MCP_BYTECODE_VALUE_OBJECT:
        case MCP_BYTECODE_VALUE_ARRAY:
            // Not implemented for simplicity
            result.type = MCP_BYTECODE_VALUE_NULL;
            break;
            
        case MCP_BYTECODE_VALUE_NULL:
        default:
            result.type = MCP_BYTECODE_VALUE_NULL;
            break;
    }
    
    return result;
}

static void freeValue(MCP_BytecodeValue* value) {
    if (value->type == MCP_BYTECODE_VALUE_STRING && value->value.stringValue != NULL) {
        free(value->value.stringValue);
        value->value.stringValue = NULL;
    }
    
    value->type = MCP_BYTECODE_VALUE_NULL;
}

static MCP_BytecodeValue createNumberValue(double value) {
    MCP_BytecodeValue result;
    result.type = MCP_BYTECODE_VALUE_NUMBER;
    result.value.numberValue = value;
    return result;
}

static MCP_BytecodeValue createBoolValue(bool value) {
    MCP_BytecodeValue result;
    result.type = MCP_BYTECODE_VALUE_BOOL;
    result.value.boolValue = value;
    return result;
}

static MCP_BytecodeValue createStringValue(const char* value) {
    MCP_BytecodeValue result;
    
    if (value != NULL) {
        result.type = MCP_BYTECODE_VALUE_STRING;
        result.value.stringValue = strdup(value);
        
        if (result.value.stringValue == NULL) {
            result.type = MCP_BYTECODE_VALUE_NULL;
        }
    } else {
        result.type = MCP_BYTECODE_VALUE_NULL;
    }
    
    return result;
}

static MCP_BytecodeValue executeInstruction(BytecodeContext* ctx, const MCP_BytecodeInstruction* instr) {
    MCP_BytecodeValue result = { .type = MCP_BYTECODE_VALUE_NULL };
    
    switch (instr->opcode) {
        case MCP_BYTECODE_OP_NOP:
            // No operation
            break;
            
        case MCP_BYTECODE_OP_PUSH_NUM:
            pushValue(ctx, createNumberValue(instr->operand.numberValue));
            break;
            
        case MCP_BYTECODE_OP_PUSH_STR:
            if (instr->operand.stringIndex < ctx->program->stringPoolSize) {
                pushValue(ctx, createStringValue(ctx->program->stringPool[instr->operand.stringIndex]));
            } else {
                ctx->errorCode = 4;
                ctx->errorMessage = strdup("Invalid string index");
                ctx->running = false;
            }
            break;
            
        case MCP_BYTECODE_OP_PUSH_BOOL:
            pushValue(ctx, createBoolValue(instr->operand.boolValue));
            break;
            
        case MCP_BYTECODE_OP_PUSH_VAR:
            if (instr->operand.variableIndex < ctx->program->variableCount) {
                pushValue(ctx, copyValue(&ctx->variables[instr->operand.variableIndex]));
            } else {
                ctx->errorCode = 5;
                ctx->errorMessage = strdup("Invalid variable index");
                ctx->running = false;
            }
            break;
            
        case MCP_BYTECODE_OP_POP:
            result = popValue(ctx);
            freeValue(&result);
            break;
            
        case MCP_BYTECODE_OP_ADD: {
            MCP_BytecodeValue b = popValue(ctx);
            MCP_BytecodeValue a = popValue(ctx);
            
            if (a.type == MCP_BYTECODE_VALUE_NUMBER && b.type == MCP_BYTECODE_VALUE_NUMBER) {
                result = createNumberValue(a.value.numberValue + b.value.numberValue);
            } else if (a.type == MCP_BYTECODE_VALUE_STRING && b.type == MCP_BYTECODE_VALUE_STRING) {
                // String concatenation
                size_t len1 = strlen(a.value.stringValue);
                size_t len2 = strlen(b.value.stringValue);
                
                char* newStr = (char*)malloc(len1 + len2 + 1);
                if (newStr != NULL) {
                    strcpy(newStr, a.value.stringValue);
                    strcat(newStr, b.value.stringValue);
                    
                    result.type = MCP_BYTECODE_VALUE_STRING;
                    result.value.stringValue = newStr;
                } else {
                    result.type = MCP_BYTECODE_VALUE_NULL;
                }
            } else {
                result.type = MCP_BYTECODE_VALUE_NULL;
            }
            
            freeValue(&a);
            freeValue(&b);
            
            pushValue(ctx, result);
            break;
        }
            
        case MCP_BYTECODE_OP_SUB: {
            MCP_BytecodeValue b = popValue(ctx);
            MCP_BytecodeValue a = popValue(ctx);
            
            if (a.type == MCP_BYTECODE_VALUE_NUMBER && b.type == MCP_BYTECODE_VALUE_NUMBER) {
                result = createNumberValue(a.value.numberValue - b.value.numberValue);
            } else {
                result.type = MCP_BYTECODE_VALUE_NULL;
            }
            
            freeValue(&a);
            freeValue(&b);
            
            pushValue(ctx, result);
            break;
        }
            
        case MCP_BYTECODE_OP_MUL: {
            MCP_BytecodeValue b = popValue(ctx);
            MCP_BytecodeValue a = popValue(ctx);
            
            if (a.type == MCP_BYTECODE_VALUE_NUMBER && b.type == MCP_BYTECODE_VALUE_NUMBER) {
                result = createNumberValue(a.value.numberValue * b.value.numberValue);
            } else {
                result.type = MCP_BYTECODE_VALUE_NULL;
            }
            
            freeValue(&a);
            freeValue(&b);
            
            pushValue(ctx, result);
            break;
        }
            
        case MCP_BYTECODE_OP_DIV: {
            MCP_BytecodeValue b = popValue(ctx);
            MCP_BytecodeValue a = popValue(ctx);
            
            if (a.type == MCP_BYTECODE_VALUE_NUMBER && b.type == MCP_BYTECODE_VALUE_NUMBER) {
                if (b.value.numberValue != 0) {
                    result = createNumberValue(a.value.numberValue / b.value.numberValue);
                } else {
                    ctx->errorCode = 6;
                    ctx->errorMessage = strdup("Division by zero");
                    ctx->running = false;
                }
            } else {
                result.type = MCP_BYTECODE_VALUE_NULL;
            }
            
            freeValue(&a);
            freeValue(&b);
            
            pushValue(ctx, result);
            break;
        }
            
        case MCP_BYTECODE_OP_EQ: {
            MCP_BytecodeValue b = popValue(ctx);
            MCP_BytecodeValue a = popValue(ctx);
            
            if (a.type == MCP_BYTECODE_VALUE_NUMBER && b.type == MCP_BYTECODE_VALUE_NUMBER) {
                result = createBoolValue(a.value.numberValue == b.value.numberValue);
            } else if (a.type == MCP_BYTECODE_VALUE_BOOL && b.type == MCP_BYTECODE_VALUE_BOOL) {
                result = createBoolValue(a.value.boolValue == b.value.boolValue);
            } else if (a.type == MCP_BYTECODE_VALUE_STRING && b.type == MCP_BYTECODE_VALUE_STRING) {
                result = createBoolValue(strcmp(a.value.stringValue, b.value.stringValue) == 0);
            } else {
                result = createBoolValue(false);
            }
            
            freeValue(&a);
            freeValue(&b);
            
            pushValue(ctx, result);
            break;
        }
            
        case MCP_BYTECODE_OP_JUMP:
            ctx->pc = instr->operand.jumpAddress;
            return result;  // Don't increment PC
            
        case MCP_BYTECODE_OP_JUMP_IF: {
            MCP_BytecodeValue condition = popValue(ctx);
            
            if (condition.type == MCP_BYTECODE_VALUE_BOOL && condition.value.boolValue) {
                ctx->pc = instr->operand.jumpAddress;
                freeValue(&condition);
                return result;  // Don't increment PC
            }
            
            freeValue(&condition);
            break;
        }
            
        case MCP_BYTECODE_OP_JUMP_IF_NOT: {
            MCP_BytecodeValue condition = popValue(ctx);
            
            if (condition.type != MCP_BYTECODE_VALUE_BOOL || !condition.value.boolValue) {
                ctx->pc = instr->operand.jumpAddress;
                freeValue(&condition);
                return result;  // Don't increment PC
            }
            
            freeValue(&condition);
            break;
        }
            
        case MCP_BYTECODE_OP_SET_VAR: {
            MCP_BytecodeValue value = popValue(ctx);
            
            if (instr->operand.variableIndex < ctx->program->variableCount) {
                // Free existing value
                freeValue(&ctx->variables[instr->operand.variableIndex]);
                
                // Assign new value
                ctx->variables[instr->operand.variableIndex] = value;
            } else {
                ctx->errorCode = 5;
                ctx->errorMessage = strdup("Invalid variable index");
                ctx->running = false;
                freeValue(&value);
            }
            break;
        }
            
        case MCP_BYTECODE_OP_HALT:
            ctx->running = false;
            break;
            
        default:
            // Unsupported operation
            ctx->errorCode = 7;
            ctx->errorMessage = strdup("Unsupported operation");
            ctx->running = false;
            break;
    }
    
    // Increment program counter
    ctx->pc++;
    
    return result;
}

MCP_BytecodeProgram* MCP_BytecodeCompileJson(const char* json, size_t jsonLength) {
    // Mark parameters as unused to avoid compiler warnings
    (void)json;
    (void)jsonLength;
    
    // Not implemented for simplicity
    return NULL;
}

MCP_BytecodeResult MCP_BytecodeExecute(const MCP_BytecodeProgram* program) {
    MCP_BytecodeResult result;
    memset(&result, 0, sizeof(result));
    
    if (!s_initialized || program == NULL) {
        result.success = false;
        result.errorCode = 1;
        result.errorMessage = strdup("Invalid program or interpreter not initialized");
        return result;
    }
    
    // Initialize execution context
    BytecodeContext ctx;
    initContext(&ctx, program);
    
    // Execute instructions
    while (ctx.running && ctx.pc < program->instructionCount) {
        executeInstruction(&ctx, &program->instructions[ctx.pc]);
    }
    
    // Prepare result
    result.success = (ctx.errorCode == 0);
    result.errorCode = ctx.errorCode;
    
    if (ctx.errorMessage != NULL) {
        result.errorMessage = strdup(ctx.errorMessage);
    }
    
    // Set return value (top of stack or null)
    if (ctx.stackTop > 0) {
        result.returnValue = copyValue(&ctx.stack[ctx.stackTop - 1]);
    } else {
        result.returnValue.type = MCP_BYTECODE_VALUE_NULL;
    }
    
    // Clean up
    freeContext(&ctx);
    
    return result;
}

void MCP_BytecodeFreeProgram(MCP_BytecodeProgram* program) {
    if (program == NULL) {
        return;
    }
    
    // Free instructions
    if (program->instructions != NULL) {
        free(program->instructions);
    }
    
    // Free string pool
    if (program->stringPool != NULL) {
        for (uint16_t i = 0; i < program->stringPoolSize; i++) {
            if (program->stringPool[i] != NULL) {
                free(program->stringPool[i]);
            }
        }
        free(program->stringPool);
    }
    
    // Free variable names
    if (program->variableNames != NULL) {
        for (uint16_t i = 0; i < program->variableCount; i++) {
            if (program->variableNames[i] != NULL) {
                free(program->variableNames[i]);
            }
        }
        free(program->variableNames);
    }
    
    // Free property names
    if (program->propertyNames != NULL) {
        for (uint16_t i = 0; i < program->propertyCount; i++) {
            if (program->propertyNames[i] != NULL) {
                free(program->propertyNames[i]);
            }
        }
        free(program->propertyNames);
    }
    
    // Free function names
    if (program->functionNames != NULL) {
        for (uint16_t i = 0; i < program->functionCount; i++) {
            if (program->functionNames[i] != NULL) {
                free(program->functionNames[i]);
            }
        }
        free(program->functionNames);
    }
    
    free(program);
}

void MCP_BytecodeFreeResult(MCP_BytecodeResult* result) {
    if (result == NULL) {
        return;
    }
    
    // Free return value
    if (result->returnValue.type == MCP_BYTECODE_VALUE_STRING && 
        result->returnValue.value.stringValue != NULL) {
        free(result->returnValue.value.stringValue);
    }
    
    // Free error message
    if (result->errorMessage != NULL) {
        free(result->errorMessage);
    }
    
    // Reset result
    memset(result, 0, sizeof(MCP_BytecodeResult));
}

size_t MCP_BytecodeGetProgramSize(const MCP_BytecodeProgram* program) {
    if (program == NULL) {
        return 0;
    }
    
    // Basic size
    size_t size = sizeof(MCP_BytecodeProgram);
    
    // Instructions size
    size += program->instructionCount * sizeof(MCP_BytecodeInstruction);
    
    // String pool size
    for (uint16_t i = 0; i < program->stringPoolSize; i++) {
        if (program->stringPool[i] != NULL) {
            size += strlen(program->stringPool[i]) + 1;
        }
    }
    
    // Variable names size
    for (uint16_t i = 0; i < program->variableCount; i++) {
        if (program->variableNames[i] != NULL) {
            size += strlen(program->variableNames[i]) + 1;
        }
    }
    
    // Property names size
    for (uint16_t i = 0; i < program->propertyCount; i++) {
        if (program->propertyNames[i] != NULL) {
            size += strlen(program->propertyNames[i]) + 1;
        }
    }
    
    // Function names size
    for (uint16_t i = 0; i < program->functionCount; i++) {
        if (program->functionNames[i] != NULL) {
            size += strlen(program->functionNames[i]) + 1;
        }
    }
    
    return size;
}

int MCP_BytecodeSerialize(const MCP_BytecodeProgram* program, uint8_t* buffer, size_t bufferSize) {
    // Mark parameters as unused to avoid compiler warnings
    (void)program;
    (void)buffer;
    (void)bufferSize;
    
    // Not implemented for simplicity
    return -1;
}

MCP_BytecodeProgram* MCP_BytecodeDeserialize(const uint8_t* buffer, size_t bufferSize) {
    // Mark parameters as unused to avoid compiler warnings
    (void)buffer;
    (void)bufferSize;
    
    // Not implemented for simplicity
    return NULL;
}