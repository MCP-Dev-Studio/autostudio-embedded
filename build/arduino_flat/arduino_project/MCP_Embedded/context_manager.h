/**
 * @file context_manager.h
 * @brief Execution context and variable management for dynamic tools
 */
#ifndef MCP_CONTEXT_MANAGER_H
#define MCP_CONTEXT_MANAGER_H

#include "tool_registry.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Variable type enumeration
 */
typedef enum {
    MCP_VAR_TYPE_NULL,
    MCP_VAR_TYPE_BOOL,
    MCP_VAR_TYPE_INT,
    MCP_VAR_TYPE_FLOAT,
    MCP_VAR_TYPE_STRING,
    MCP_VAR_TYPE_OBJECT,
    MCP_VAR_TYPE_ARRAY
} MCP_VariableType;

/**
 * @brief Variable value structure
 */
typedef struct {
    MCP_VariableType type;
    union {
        bool boolValue;
        int32_t intValue;
        float floatValue;
        char* stringValue;
        void* objectValue;
        void* arrayValue;
    } value;
} MCP_Variable;

/**
 * @brief Execution context structure
 */
typedef struct MCP_ExecutionContext {
    char* name;                         // Context name
    MCP_Variable* variables;            // Variable array
    char** variableNames;               // Variable names
    int variableCount;                  // Current variable count
    int maxVariables;                   // Maximum variables
    struct MCP_ExecutionContext* parent; // Parent context (for scope)
} MCP_ExecutionContext;

/**
 * @brief Create a new execution context
 *
 * @param name Context name
 * @param parent Parent context (can be NULL)
 * @param maxVariables Maximum number of variables
 * @return MCP_ExecutionContext* New context or NULL on failure
 */
MCP_ExecutionContext* MCP_ContextCreate(const char* name, MCP_ExecutionContext* parent, int maxVariables);

/**
 * @brief Free an execution context
 *
 * @param context Context to free
 */
void MCP_ContextFree(MCP_ExecutionContext* context);

/**
 * @brief Set a variable in the context
 *
 * @param context Execution context
 * @param name Variable name
 * @param value Variable value
 * @return int 0 on success, negative error code on failure
 */
int MCP_ContextSetVariable(MCP_ExecutionContext* context, const char* name, const MCP_Variable* value);

/**
 * @brief Get a variable from the context
 *
 * @param context Execution context
 * @param name Variable name
 * @return MCP_Variable Variable value (type will be NULL if not found)
 */
MCP_Variable MCP_ContextGetVariable(MCP_ExecutionContext* context, const char* name);

/**
 * @brief Check if a variable exists in the context
 *
 * @param context Execution context
 * @param name Variable name
 * @return bool True if variable exists
 */
bool MCP_ContextHasVariable(MCP_ExecutionContext* context, const char* name);

/**
 * @brief Substitute variables in a template string
 *
 * @param context Execution context
 * @param templateStr Template string with ${variable|default} syntax
 * @return char* New string with variables substituted (caller must free)
 */
char* MCP_ContextSubstituteVariables(MCP_ExecutionContext* context, const char* templateStr);

/**
 * @brief Store tool execution result in a variable
 *
 * @param context Execution context
 * @param name Variable name
 * @param result Tool execution result
 * @return int 0 on success, negative error code on failure
 */
int MCP_ContextStoreToolResult(MCP_ExecutionContext* context, const char* name, const MCP_ToolResult* result);

/**
 * @brief Create variable from different types
 */
MCP_Variable MCP_VariableCreateNull(void);
MCP_Variable MCP_VariableCreateBool(bool value);
MCP_Variable MCP_VariableCreateInt(int32_t value);
MCP_Variable MCP_VariableCreateFloat(float value);
MCP_Variable MCP_VariableCreateString(const char* value);
MCP_Variable MCP_VariableCreateObject(void* value);
MCP_Variable MCP_VariableCreateArray(void* value);

/**
 * @brief Free variable resources if needed
 *
 * @param variable Variable to free
 */
void MCP_VariableFree(MCP_Variable* variable);

/**
 * @brief Set the current execution context
 * 
 * @param context Context to set as current
 * @return MCP_ExecutionContext* Previous current context
 */
MCP_ExecutionContext* MCP_ContextSetCurrent(MCP_ExecutionContext* context);

/**
 * @brief Get the current execution context
 * 
 * @return MCP_ExecutionContext* Current context or NULL if none set
 */
MCP_ExecutionContext* MCP_ContextGetCurrent(void);

/**
 * @brief Set a string value in a context
 * 
 * @param context Execution context
 * @param name Variable name
 * @param value String value
 * @return int 0 on success, negative error code on failure
 */
int MCP_ContextSetValue(MCP_ExecutionContext* context, const char* name, const char* value);

/**
 * @brief Get a string value from a context
 * 
 * @param context Execution context
 * @param name Variable name
 * @return const char* String value or NULL if not found or not a string
 */
const char* MCP_ContextGetValue(MCP_ExecutionContext* context, const char* name);

#endif /* MCP_CONTEXT_MANAGER_H */