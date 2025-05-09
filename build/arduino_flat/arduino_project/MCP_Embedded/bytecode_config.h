#ifndef MCP_BYTECODE_CONFIG_H
#define MCP_BYTECODE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tool name for bytecode configuration
 */
#define MCP_BYTECODE_CONFIG_TOOL_NAME "system.bytecodeConfig"

/**
 * @brief Bytecode runtime configuration structure
 * Contains configurable parameters for bytecode execution
 */
typedef struct {
    uint32_t max_bytecode_size;     // Maximum bytecode program size in bytes
    uint16_t max_stack_size;        // Maximum stack size for bytecode execution
    uint16_t max_string_pool_size;  // Maximum string pool entries
    uint16_t max_variable_count;    // Maximum variables per program
    uint16_t max_function_count;    // Maximum functions per program
    uint16_t max_execution_time_ms; // Maximum execution time in milliseconds
    bool dynamic_allocation;        // Whether to use dynamic allocation for bytecode
    uint32_t total_memory_limit;    // Total memory limit for all bytecode programs
} MCP_BytecodeRuntimeConfig;

/**
 * @brief Initialize bytecode configuration with defaults
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeConfigInit(void);

/**
 * @brief Get the current bytecode configuration
 * 
 * @param config Pointer to configuration structure to fill
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeConfigGet(MCP_BytecodeRuntimeConfig* config);

/**
 * @brief Set bytecode configuration
 * 
 * @param config New configuration values
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeConfigSet(const MCP_BytecodeRuntimeConfig* config);

/**
 * @brief Reset bytecode configuration to defaults
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeConfigReset(void);

/**
 * @brief Validate if a memory allocation is allowed under current limits
 * 
 * @param size Size in bytes to allocate
 * @return bool True if allocation is allowed, false otherwise
 */
bool MCP_BytecodeConfigCanAllocate(size_t size);

/**
 * @brief Track a successful memory allocation
 * 
 * @param size Size in bytes allocated
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeConfigTrackAllocation(size_t size);

/**
 * @brief Track a memory deallocation
 * 
 * @param size Size in bytes deallocated
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeConfigTrackDeallocation(size_t size);

/**
 * @brief Get total memory currently allocated for bytecode
 * 
 * @return size_t Total bytes allocated
 */
size_t MCP_BytecodeConfigGetTotalAllocated(void);

/**
 * @brief Register the bytecode configuration tool
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeConfigToolRegister(void);

/**
 * @brief Get the default bytecode runtime configuration for the current platform
 * 
 * @return MCP_BytecodeRuntimeConfig Default configuration
 */
MCP_BytecodeRuntimeConfig MCP_BytecodeConfigGetDefault(void);

/**
 * @brief Get recommended safe configuration values based on available system resources
 * 
 * @param config Pointer to configuration structure to fill with recommendations
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeConfigGetRecommended(MCP_BytecodeRuntimeConfig* config);

#ifdef __cplusplus
}
#endif

#endif /* MCP_BYTECODE_CONFIG_H */