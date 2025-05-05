#ifndef MCP_BYTECODE_INTERPRETER_H
#define MCP_BYTECODE_INTERPRETER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Bytecode operation codes
 */
typedef enum {
    MCP_BYTECODE_OP_NOP,          // No operation
    MCP_BYTECODE_OP_PUSH_NUM,     // Push number onto stack
    MCP_BYTECODE_OP_PUSH_STR,     // Push string onto stack
    MCP_BYTECODE_OP_PUSH_BOOL,    // Push boolean onto stack
    MCP_BYTECODE_OP_PUSH_VAR,     // Push variable value onto stack
    MCP_BYTECODE_OP_POP,          // Pop value from stack
    MCP_BYTECODE_OP_ADD,          // Add top two values
    MCP_BYTECODE_OP_SUB,          // Subtract top value from second
    MCP_BYTECODE_OP_MUL,          // Multiply top two values
    MCP_BYTECODE_OP_DIV,          // Divide second by top value
    MCP_BYTECODE_OP_MOD,          // Modulo of second by top value
    MCP_BYTECODE_OP_EQ,           // Equality comparison
    MCP_BYTECODE_OP_NEQ,          // Inequality comparison
    MCP_BYTECODE_OP_GT,           // Greater than comparison
    MCP_BYTECODE_OP_LT,           // Less than comparison
    MCP_BYTECODE_OP_GTE,          // Greater than or equal comparison
    MCP_BYTECODE_OP_LTE,          // Less than or equal comparison
    MCP_BYTECODE_OP_AND,          // Logical AND
    MCP_BYTECODE_OP_OR,           // Logical OR
    MCP_BYTECODE_OP_NOT,          // Logical NOT
    MCP_BYTECODE_OP_JUMP,         // Unconditional jump
    MCP_BYTECODE_OP_JUMP_IF,      // Conditional jump if true
    MCP_BYTECODE_OP_JUMP_IF_NOT,  // Conditional jump if false
    MCP_BYTECODE_OP_CALL,         // Call function
    MCP_BYTECODE_OP_RETURN,       // Return from function
    MCP_BYTECODE_OP_SET_VAR,      // Set variable
    MCP_BYTECODE_OP_GET_PROP,     // Get property of object
    MCP_BYTECODE_OP_SET_PROP,     // Set property of object
    MCP_BYTECODE_OP_NEW_ARRAY,    // Create new array
    MCP_BYTECODE_OP_NEW_OBJECT,   // Create new object
    MCP_BYTECODE_OP_HALT          // Stop execution
} MCP_BytecodeOpCode;

/**
 * @brief Bytecode instruction
 */
typedef struct {
    MCP_BytecodeOpCode opcode;
    union {
        double numberValue;
        uint16_t stringIndex;
        bool boolValue;
        uint16_t variableIndex;
        uint16_t jumpAddress;
        uint16_t functionIndex;
        uint16_t propertyIndex;
    } operand;
} MCP_BytecodeInstruction;

/**
 * @brief Bytecode program
 */
typedef struct {
    MCP_BytecodeInstruction* instructions;
    uint16_t instructionCount;
    char** stringPool;
    uint16_t stringPoolSize;
    char** variableNames;
    uint16_t variableCount;
    char** propertyNames;
    uint16_t propertyCount;
    char** functionNames;
    uint16_t functionCount;
} MCP_BytecodeProgram;

/**
 * @brief Bytecode value type
 */
typedef enum {
    MCP_BYTECODE_VALUE_NULL,
    MCP_BYTECODE_VALUE_NUMBER,
    MCP_BYTECODE_VALUE_STRING,
    MCP_BYTECODE_VALUE_BOOL,
    MCP_BYTECODE_VALUE_OBJECT,
    MCP_BYTECODE_VALUE_ARRAY
} MCP_BytecodeValueType;

/**
 * @brief Bytecode execution value
 */
typedef struct MCP_BytecodeValue {
    MCP_BytecodeValueType type;
    union {
        double numberValue;
        char* stringValue;
        bool boolValue;
        void* objectValue;
        struct {
            struct MCP_BytecodeValue* items;
            uint16_t count;
        } arrayValue;
    } value;
} MCP_BytecodeValue;

/**
 * @brief Bytecode execution result
 */
typedef struct {
    bool success;
    MCP_BytecodeValue returnValue;
    uint16_t errorCode;
    char* errorMessage;
} MCP_BytecodeResult;

/**
 * @brief Initialize the bytecode interpreter
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeInterpreterInit(void);

/**
 * @brief Compile JSON to bytecode
 * 
 * @param json JSON string to compile
 * @param jsonLength Length of JSON string
 * @return MCP_BytecodeProgram* Compiled program or NULL on failure
 */
MCP_BytecodeProgram* MCP_BytecodeCompileJson(const char* json, size_t jsonLength);

/**
 * @brief Execute bytecode program
 * 
 * @param program Bytecode program to execute
 * @return MCP_BytecodeResult Execution result
 */
MCP_BytecodeResult MCP_BytecodeExecute(const MCP_BytecodeProgram* program);

/**
 * @brief Free bytecode program
 * 
 * @param program Program to free
 */
void MCP_BytecodeFreeProgram(MCP_BytecodeProgram* program);

/**
 * @brief Free bytecode result
 * 
 * @param result Result to free
 */
void MCP_BytecodeFreeResult(MCP_BytecodeResult* result);

/**
 * @brief Get bytecode instruction size
 * 
 * @param program Bytecode program
 * @return size_t Size in bytes
 */
size_t MCP_BytecodeGetProgramSize(const MCP_BytecodeProgram* program);

/**
 * @brief Serialize bytecode program to binary
 * 
 * @param program Bytecode program to serialize
 * @param buffer Buffer to store serialized program
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_BytecodeSerialize(const MCP_BytecodeProgram* program, uint8_t* buffer, size_t bufferSize);

/**
 * @brief Deserialize bytecode program from binary
 * 
 * @param buffer Buffer containing serialized program
 * @param bufferSize Size of buffer
 * @return MCP_BytecodeProgram* Deserialized program or NULL on failure
 */
MCP_BytecodeProgram* MCP_BytecodeDeserialize(const uint8_t* buffer, size_t bufferSize);

#endif /* MCP_BYTECODE_INTERPRETER_H */