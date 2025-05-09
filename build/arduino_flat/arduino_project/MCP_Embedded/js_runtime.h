#ifndef MCP_JS_RUNTIME_H
#define MCP_JS_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Initialize JavaScript runtime
 * 
 * @return int 0 on success, negative error code on failure
 */
int js_init(void);

/**
 * @brief Clean up JavaScript runtime
 * 
 * @return int 0 on success, negative error code on failure
 */
int js_cleanup(void);

/**
 * @brief Evaluate JavaScript code
 * 
 * @param script JavaScript code to evaluate
 * @param scriptLength Length of script
 * @param result Buffer to store result
 * @param maxResultLen Maximum length of result buffer
 * @return int 0 on success, negative error code on failure
 */
int js_eval(const char* script, size_t scriptLength, char* result, size_t maxResultLen);

/**
 * @brief Call a JavaScript function
 * 
 * @param moduleName Module/context name
 * @param funcName Function name
 * @param params Parameters as JSON string
 * @param result Buffer to store result
 * @param maxResultLen Maximum length of result buffer
 * @return int 0 on success, negative error code on failure
 */
int js_call_function(const char* moduleName, const char* funcName, const char* params, 
                   char* result, size_t maxResultLen);

/**
 * @brief Register a native C function to be callable from JavaScript
 * 
 * @param name Function name
 * @param funcPtr Function pointer
 * @return int 0 on success, negative error code on failure
 */
int js_register_native_function(const char* name, void* funcPtr);

/**
 * @brief Create a new JavaScript module/context
 * 
 * @param moduleName Module name
 * @param script Module source code
 * @param scriptLength Length of source code
 * @return int 0 on success, negative error code on failure
 */
int js_create_module(const char* moduleName, const char* script, size_t scriptLength);

/**
 * @brief Delete a JavaScript module/context
 * 
 * @param moduleName Module name
 * @return int 0 on success, negative error code on failure
 */
int js_delete_module(const char* moduleName);

#endif /* MCP_JS_RUNTIME_H */