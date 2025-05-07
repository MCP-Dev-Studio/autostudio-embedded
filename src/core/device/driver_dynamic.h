#ifndef MCP_DRIVER_DYNAMIC_H
#define MCP_DRIVER_DYNAMIC_H

#include "driver_manager.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize the dynamic driver system
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_DynamicDriverInit(void);

/**
 * @brief Register a driver from JSON definition
 * 
 * @param json JSON definition of the driver
 * @param length Length of the JSON string
 * @return int 0 on success, negative error code on failure
 */
int MCP_DynamicDriverRegister(const char* json, size_t length);

/**
 * @brief Unregister a dynamic driver
 * 
 * @param id Driver ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_DynamicDriverUnregister(const char* id);

/**
 * @brief Save a dynamic driver to persistent storage
 * 
 * @param id Driver ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_DynamicDriverSave(const char* id);

/**
 * @brief Load a dynamic driver from persistent storage
 * 
 * @param id Driver ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_DynamicDriverLoad(const char* id);

/**
 * @brief Load all dynamic drivers from persistent storage
 * 
 * @return int Number of drivers loaded or negative error code
 */
int MCP_DynamicDriverLoadAll(void);

/**
 * @brief Execute a function defined in dynamic driver's script
 * 
 * @param driverId Driver ID
 * @param funcName Function name
 * @param params Parameters as JSON string
 * @param paramsLength Length of parameters string
 * @param result Buffer to store result
 * @param maxResultSize Maximum size of result buffer
 * @return int 0 on success, negative error code on failure
 */
int MCP_DynamicDriverExecuteFunction(const char* driverId, const char* funcName, 
                                   const char* params, size_t paramsLength,
                                   void* result, size_t maxResultSize);

#endif /* MCP_DRIVER_DYNAMIC_H */