#ifndef MCP_DRIVER_BYTECODE_H
#define MCP_DRIVER_BYTECODE_H

#include "driver_manager.h"
#include "../tool_system/bytecode_interpreter.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize the bytecode driver system
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeDriverInit(void);

/**
 * @brief Register a driver from JSON definition with bytecode implementation
 * 
 * @param json JSON definition of the driver
 * @param length Length of the JSON string
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeDriverRegister(const char* json, size_t length);

/**
 * @brief Unregister a bytecode driver
 * 
 * @param id Driver ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeDriverUnregister(const char* id);

/**
 * @brief Save a bytecode driver to persistent storage
 * 
 * @param id Driver ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeDriverSave(const char* id);

/**
 * @brief Load a bytecode driver from persistent storage
 * 
 * @param id Driver ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeDriverLoad(const char* id);

/**
 * @brief Load all bytecode drivers from persistent storage
 * 
 * @return int Number of drivers loaded or negative error code
 */
int MCP_BytecodeDriverLoadAll(void);

/**
 * @brief Execute a function defined in bytecode driver
 * 
 * @param driverId Driver ID
 * @param funcName Function name
 * @param params Parameters as JSON string
 * @param paramsLength Length of parameters string
 * @param result Buffer to store result
 * @param maxResultSize Maximum size of result buffer
 * @return int 0 on success, negative error code on failure
 */
int MCP_BytecodeDriverExecuteFunction(const char* driverId, const char* funcName, 
                                   const char* params, size_t paramsLength,
                                   void* result, size_t maxResultSize);

#endif /* MCP_DRIVER_BYTECODE_H */