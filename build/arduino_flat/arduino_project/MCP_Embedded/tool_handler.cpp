#include "arduino_compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
// HOST platform implementation
// These functions are defined in tool_registry.c to avoid duplicate symbol errors

/**
 * @brief Initialize tool handler for HOST platform
 * 
 * @return int 0 on success, negative error code on failure
 */
extern int MCP_ToolHandlerInit(void);

/**
 * @brief Handle tool invocation request from protocol handler
 * 
 * This is a stub implementation for the HOST platform.
 * 
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @param contentJson Tool content as JSON
 * @param contentLength Content length
 * @return int 0 on success, negative error code on failure
 */
extern int MCP_ToolHandleInvocation(const char* sessionId, const char* operationId,
                         const uint8_t* contentJson, size_t contentLength);

/**
 * @brief Handle get tool list request
 * 
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @return int 0 on success, negative error code on failure
 */
extern int MCP_ToolHandleGetList(const char* sessionId, const char* operationId);

/**
 * @brief Handle get tool schema request
 * 
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @param toolName Tool name
 * @return int 0 on success, negative error code on failure
 */
extern int MCP_ToolHandleGetSchema(const char* sessionId, const char* operationId, const char* toolName);

#else
// Regular platform implementation
#include "tool_handler.h"

// External JSON utility functions (minimal implementation)
extern char* json_get_string_field(const char* json, const char* field);
extern int json_get_int_field(const char* json, const char* field, int defaultValue);
extern bool json_get_bool_field(const char* json, const char* field, bool defaultValue);
extern float json_get_float_field(const char* json, const char* field, float defaultValue);
extern void* json_get_object_field(const char* json, const char* field);
extern void* json_get_array_field(const char* json, const char* field);
extern bool json_field_exists(const char* json, const char* field);

MCP_ToolParams* MCP_ToolParseParams(const char* json, size_t length, 
                                 const MCP_ToolParamDef* paramDefs, int paramDefCount) {
    if (json == NULL || length == 0 || paramDefs == NULL || paramDefCount <= 0) {
        return NULL;
    }
    
    // Extract params object from JSON
    void* paramsJson = json_get_object_field(json, "params");
    if (paramsJson == NULL) {
        return NULL;  // No params found
    }
    
    // Allocate parameters structure
    MCP_ToolParams* params = (MCP_ToolParams*)malloc(sizeof(MCP_ToolParams));
    if (params == NULL) {
        return NULL;  // Memory allocation failed
    }
    
    // Allocate parameter array
    params->params = (MCP_ToolParam*)calloc(paramDefCount, sizeof(MCP_ToolParam));
    if (params->params == NULL) {
        free(params);
        return NULL;  // Memory allocation failed
    }
    
    params->paramCount = 0;
    
    // Process each parameter definition
    for (int i = 0; i < paramDefCount; i++) {
        const MCP_ToolParamDef* def = &paramDefs[i];
        MCP_ToolParam* param = &params->params[params->paramCount];
        
        // Initialize parameter
        param->name = def->name;
        param->type = def->type;
        param->isPresent = false;
        param->isValid = false;
        
        // Check if parameter is present in JSON
        if (!json_field_exists(paramsJson, def->name)) {
            // Check if parameter is required
            if (def->flags & MCP_PARAM_FLAG_REQUIRED) {
                // Missing required parameter
                // Cleanup and return NULL
                MCP_ToolFreeParams(params);
                return NULL;
            }
            
            // Optional parameter not present, use default value if available
            if (def->flags & MCP_PARAM_FLAG_HAS_DEFAULT) {
                param->isPresent = true;
                param->isValid = true;
                
                // Copy default value
                switch (def->type) {
                    case MCP_PARAM_TYPE_BOOLEAN:
                        param->value.boolValue = def->defaultValue.boolValue;
                        break;
                    case MCP_PARAM_TYPE_INTEGER:
                        param->value.intValue = def->defaultValue.intValue;
                        break;
                    case MCP_PARAM_TYPE_FLOAT:
                        param->value.floatValue = def->defaultValue.floatValue;
                        break;
                    case MCP_PARAM_TYPE_STRING:
                        if (def->defaultValue.stringValue != NULL) {
                            param->value.stringValue = strdup(def->defaultValue.stringValue);
                        } else {
                            param->value.stringValue = NULL;
                        }
                        break;
                    default:
                        param->isValid = false;
                        break;
                }
                
                params->paramCount++;
            }
            
            continue;  // Skip to next parameter
        }
        
        // Parameter is present, get value based on type
        param->isPresent = true;
        param->isValid = true;
        
        switch (def->type) {
            case MCP_PARAM_TYPE_BOOLEAN:
                param->value.boolValue = json_get_bool_field(paramsJson, def->name, false);
                break;
                
            case MCP_PARAM_TYPE_INTEGER:
                param->value.intValue = json_get_int_field(paramsJson, def->name, 0);
                break;
                
            case MCP_PARAM_TYPE_FLOAT:
                param->value.floatValue = json_get_float_field(paramsJson, def->name, 0.0f);
                break;
                
            case MCP_PARAM_TYPE_STRING:
                param->value.stringValue = json_get_string_field(paramsJson, def->name);
                break;
                
            case MCP_PARAM_TYPE_OBJECT:
                param->value.objectValue = json_get_object_field(paramsJson, def->name);
                break;
                
            case MCP_PARAM_TYPE_ARRAY:
                param->value.arrayValue = json_get_array_field(paramsJson, def->name);
                break;
                
            default:
                param->isValid = false;
                break;
        }
        
        params->paramCount++;
    }
    
    return params;
}

void MCP_ToolFreeParams(MCP_ToolParams* params) {
    if (params == NULL) {
        return;
    }
    
    // Free string values (other types don't need freeing)
    for (int i = 0; i < params->paramCount; i++) {
        MCP_ToolParam* param = &params->params[i];
        if (param->type == MCP_PARAM_TYPE_STRING && param->value.stringValue != NULL) {
            free(param->value.stringValue);
            param->value.stringValue = NULL;
        }
    }
    
    // Free parameter array
    if (params->params != NULL) {
        free(params->params);
    }
    
    // Free parameters structure
    free(params);
}

#endif /* MCP_OS_HOST || MCP_PLATFORM_HOST */
