#ifndef MCP_TOOL_HANDLER_H
#define MCP_TOOL_HANDLER_H

#include "tool_registry.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Tool parameter type
 */
typedef enum {
    MCP_TOOL_PARAM_TYPE_BOOL,
    MCP_TOOL_PARAM_TYPE_INT,
    MCP_TOOL_PARAM_TYPE_FLOAT,
    MCP_TOOL_PARAM_TYPE_STRING,
    MCP_TOOL_PARAM_TYPE_OBJECT,
    MCP_TOOL_PARAM_TYPE_ARRAY
} MCP_ToolParamType;

/**
 * @brief Tool parameter value
 */
typedef union {
    bool boolValue;
    int32_t intValue;
    float floatValue;
    char* stringValue;
    void* objectValue;
    void* arrayValue;
} MCP_ToolParamValue;

/**
 * @brief Tool parameter definition
 */
typedef struct {
    const char* name;
    MCP_ToolParamType type;
    bool required;
} MCP_ToolParamDef;

/**
 * @brief Tool parameter structure
 */
typedef struct {
    const char* name;
    MCP_ToolParamType type;
    MCP_ToolParamValue value;
} MCP_ToolParam;

/**
 * @brief Tool execution parameters
 */
typedef struct {
    MCP_ToolParam* params;
    int paramCount;
} MCP_ToolParams;

/**
 * @brief Parse parameters from JSON string
 * 
 * @param json JSON string with parameters
 * @param length Length of JSON string
 * @param paramDefs Parameter definitions array
 * @param paramDefCount Number of parameter definitions
 * @return MCP_ToolParams* Parsed parameters or NULL on error
 */
MCP_ToolParams* MCP_ToolParseParams(const char* json, size_t length, 
                                   const MCP_ToolParamDef* paramDefs, int paramDefCount);

/**
 * @brief Free parsed parameters
 * 
 * @param params Parameters to free
 */
void MCP_ToolFreeParams(MCP_ToolParams* params);

/**
 * @brief Get a boolean parameter value
 * 
 * @param params Parameters structure
 * @param name Parameter name
 * @param defaultValue Default value if parameter not found
 * @return bool Parameter value or default value
 */
bool MCP_ToolGetBoolParam(const MCP_ToolParams* params, const char* name, bool defaultValue);

/**
 * @brief Get an integer parameter value
 * 
 * @param params Parameters structure
 * @param name Parameter name
 * @param defaultValue Default value if parameter not found
 * @return int32_t Parameter value or default value
 */
int32_t MCP_ToolGetIntParam(const MCP_ToolParams* params, const char* name, int32_t defaultValue);

/**
 * @brief Get a float parameter value
 * 
 * @param params Parameters structure
 * @param name Parameter name
 * @param defaultValue Default value if parameter not found
 * @return float Parameter value or default value
 */
float MCP_ToolGetFloatParam(const MCP_ToolParams* params, const char* name, float defaultValue);

/**
 * @brief Get a string parameter value
 * 
 * @param params Parameters structure
 * @param name Parameter name
 * @param defaultValue Default value if parameter not found
 * @return const char* Parameter value or default value
 */
const char* MCP_ToolGetStringParam(const MCP_ToolParams* params, const char* name, const char* defaultValue);

/**
 * @brief Create a JSON schema for a tool
 * 
 * @param toolName Tool name
 * @param description Tool description
 * @param paramDefs Parameter definitions array
 * @param paramDefCount Number of parameter definitions
 * @return char* JSON schema string (caller must free)
 */
char* MCP_ToolCreateSchema(const char* toolName, const char* description, 
                          const MCP_ToolParamDef* paramDefs, int paramDefCount);

/**
 * @brief Create a tool result with parameters
 * 
 * @param params Parameters array
 * @param paramCount Number of parameters
 * @return char* JSON result string (caller must free)
 */
char* MCP_ToolCreateResult(const MCP_ToolParam* params, int paramCount);

#endif /* MCP_TOOL_HANDLER_H */