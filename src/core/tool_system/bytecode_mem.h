#ifndef MCP_BYTECODE_MEM_H
#define MCP_BYTECODE_MEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize bytecode memory management
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeMemInit(void);

/**
 * @brief Allocate memory for bytecode use
 * 
 * This function allocates memory for bytecode use and tracks the allocation.
 * It respects the bytecode memory limits configured in the bytecode configuration.
 * 
 * @param size Size to allocate in bytes
 * @return void* Allocated memory or NULL on failure
 */
void* MCP_BytecodeMemAlloc(size_t size);

/**
 * @brief Reallocate memory for bytecode use
 * 
 * This function reallocates memory for bytecode use and tracks the allocation.
 * It respects the bytecode memory limits configured in the bytecode configuration.
 * 
 * @param ptr Pointer to existing allocation or NULL
 * @param oldSize Size of existing allocation
 * @param newSize New size to allocate
 * @return void* Reallocated memory or NULL on failure
 */
void* MCP_BytecodeMemRealloc(void* ptr, size_t oldSize, size_t newSize);

/**
 * @brief Free memory allocated for bytecode use
 * 
 * This function frees memory allocated for bytecode use and tracks the deallocation.
 * 
 * @param ptr Pointer to memory to free
 * @param size Size of the allocation
 */
void MCP_BytecodeMemFree(void* ptr, size_t size);

/**
 * @brief Allocate memory for a bytecode program
 * 
 * This function allocates memory for a bytecode program and initializes
 * the program structure with the specified capacities.
 * 
 * @param instructionCapacity Maximum number of instructions
 * @param stringPoolCapacity Maximum number of strings in pool
 * @param variableCapacity Maximum number of variables
 * @param propertyCapacity Maximum number of properties
 * @param functionCapacity Maximum number of functions
 * @return MCP_BytecodeProgram* Allocated program or NULL on failure
 */
MCP_BytecodeProgram* MCP_BytecodeMemAllocProgram(
    uint16_t instructionCapacity, 
    uint16_t stringPoolCapacity,
    uint16_t variableCapacity,
    uint16_t propertyCapacity,
    uint16_t functionCapacity);

/**
 * @brief Free a bytecode program
 * 
 * This function frees a bytecode program and all its components.
 * 
 * @param program Program to free
 */
void MCP_BytecodeMemFreeProgram(MCP_BytecodeProgram* program);

/**
 * @brief Get the size of a bytecode program
 * 
 * This function calculates the total memory size of a bytecode program.
 * 
 * @param program Program to measure
 * @return size_t Size in bytes
 */
size_t MCP_BytecodeMemGetProgramSize(const MCP_BytecodeProgram* program);

/**
 * @brief Allocate an execution context
 * 
 * This function allocates an execution context with the specified stack size.
 * 
 * @param stackSize Maximum stack size
 * @return MCP_BytecodeContext* Allocated context or NULL on failure
 */
MCP_BytecodeContext* MCP_BytecodeMemAllocContext(uint16_t stackSize);

/**
 * @brief Free an execution context
 * 
 * This function frees an execution context.
 * 
 * @param context Context to free
 */
void MCP_BytecodeMemFreeContext(MCP_BytecodeContext* context);

/**
 * @brief Get the size of an execution context
 * 
 * This function calculates the total memory size of an execution context.
 * 
 * @param context Context to measure
 * @return size_t Size in bytes
 */
size_t MCP_BytecodeMemGetContextSize(const MCP_BytecodeContext* context);

#ifdef __cplusplus
}
#endif

#endif /* MCP_BYTECODE_MEM_H */