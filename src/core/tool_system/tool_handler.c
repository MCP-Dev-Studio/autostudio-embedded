#include "tool_handler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    
    // Parse parameters based on definitions
    for (int i = 0; i < paramDefCount; i++) {
        const MCP_ToolParamDef* def = &paramDefs[i];
        
        // Check if required parameter exists
        if (def->required && !json_field_exists(paramsJson, def->name)) {
            // Missing required parameter
            MCP_ToolFreeParams(params);
            return NULL;
        }
        
        // Skip optional parameters that don't exist
        if (!json_field_exists(paramsJson, def->name)) {
            continue;
        }
        
        // Parse parameter based on type
        MCP_ToolParam* param = &params->params[params->paramCount];
        param->name = def->name;
        param->type = def->type;
        
        switch (def->type) {
            case MCP_TOOL_PARAM_TYPE_BOOL:
                param->value.boolValue = json_get_bool_field(paramsJson, def->name, false);
                break;
                
            case MCP_TOOL_PARAM_TYPE_INT:
                param->value.intValue = json_get_int_field(paramsJson, def->name, 0);
                break;
                
            case MCP_TOOL_PARAM_TYPE_FLOAT:
                param->value.floatValue = json_get_float_field(paramsJson, def->name, 0.0f);
                break;
                
            case MCP_TOOL_PARAM_TYPE_STRING: {
                char* strValue = json_get_string_field(paramsJson, def->name);
                if (strValue != NULL) {
                    param->value.stringValue = strValue;
                } else {
                    param->value.stringValue = NULL;
                }
                break;
            }
                
            case MCP_TOOL_PARAM_TYPE_OBJECT:
                param->value.objectValue = json_get_object_field(paramsJson, def->name);
                break;
                
            case MCP_TOOL_PARAM_TYPE_ARRAY:
                param->value.arrayValue = json_get_array_field(paramsJson, def->name);
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
    
    if (params->params != NULL) {
        // Free string values
        for (int i = 0; i < params->paramCount; i++) {
            if (params->params[i].type == MCP_TOOL_PARAM_TYPE_STRING && 
                params->params[i].value.stringValue != NULL) {
                free(params->params[i].value.stringValue);
            }
        }
        
        free(params->params);
    }
    
    free(params);
}

static const MCP_ToolParam* findParam(const MCP_ToolParams* params, const char* name) {
    if (params == NULL || name == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < params->paramCount; i++) {
        if (strcmp(params->params[i].name, name) == 0) {
            return &params->params[i];
        }
    }
    
    return NULL;
}

bool MCP_ToolGetBoolParam(const MCP_ToolParams* params, const char* name, bool defaultValue) {
    const MCP_ToolParam* param = findParam(params, name);
    if (param == NULL || param->type != MCP_TOOL_PARAM_TYPE_BOOL) {
        return defaultValue;
    }
    
    return param->value.boolValue;
}

int32_t MCP_ToolGetIntParam(const MCP_ToolParams* params, const char* name, int32_t defaultValue) {
    const MCP_ToolParam* param = findParam(params, name);
    if (param == NULL || param->type != MCP_TOOL_PARAM_TYPE_INT) {
        return defaultValue;
    }
    
    return param->value.intValue;
}

float MCP_ToolGetFloatParam(const MCP_ToolParams* params, const char* name, float defaultValue) {
    const MCP_ToolParam* param = findParam(params, name);
    if (param == NULL || param->type != MCP_TOOL_PARAM_TYPE_FLOAT) {
        return defaultValue;
    }
    
    return param->value.floatValue;
}

const char* MCP_ToolGetStringParam(const MCP_ToolParams* params, const char* name, const char* defaultValue) {
    const MCP_ToolParam* param = findParam(params, name);
    if (param == NULL || param->type != MCP_TOOL_PARAM_TYPE_STRING || param->value.stringValue == NULL) {
        return defaultValue;
    }
    
    return param->value.stringValue;
}

// Helper function to get type name as string
static const char* getTypeName(MCP_ToolParamType type) {
    switch (type) {
        case MCP_TOOL_PARAM_TYPE_BOOL:
            return "boolean";
        case MCP_TOOL_PARAM_TYPE_INT:
            return "integer";
        case MCP_TOOL_PARAM_TYPE_FLOAT:
            return "number";
        case MCP_TOOL_PARAM_TYPE_STRING:
            return "string";
        case MCP_TOOL_PARAM_TYPE_OBJECT:
            return "object";
        case MCP_TOOL_PARAM_TYPE_ARRAY:
            return "array";
        default:
            return "unknown";
    }
}

char* MCP_ToolCreateSchema(const char* toolName, const char* description, 
                          const MCP_ToolParamDef* paramDefs, int paramDefCount) {
    if (toolName == NULL || description == NULL || paramDefs == NULL || paramDefCount <= 0) {
        return NULL;
    }
    
    // Allocate a larger buffer for the schema
    char* schema = (char*)malloc(4096);  // Adjust size as needed
    if (schema == NULL) {
        return NULL;
    }
    
    // Start schema
    int offset = 0;
    offset += snprintf(schema + offset, 4096 - offset, 
                      "{"
                      "\"name\":\"%s\","
                      "\"description\":\"%s\","
                      "\"params\":{"
                      "\"type\":\"object\","
                      "\"properties\":{", 
                      toolName, description);
    
    // Add parameter definitions
    for (int i = 0; i < paramDefCount; i++) {
        const MCP_ToolParamDef* def = &paramDefs[i];
        
        // Add comma if not first parameter
        if (i > 0) {
            offset += snprintf(schema + offset, 4096 - offset, ",");
        }
        
        // Add parameter definition
        offset += snprintf(schema + offset, 4096 - offset, 
                          "\"%s\":{"
                          "\"type\":\"%s\"", 
                          def->name, getTypeName(def->type));
        
        // Close parameter definition
        offset += snprintf(schema + offset, 4096 - offset, "}");
    }
    
    // Add required parameters
    offset += snprintf(schema + offset, 4096 - offset, "},\"required\":[");
    
    bool firstRequired = true;
    for (int i = 0; i < paramDefCount; i++) {
        if (paramDefs[i].required) {
            if (!firstRequired) {
                offset += snprintf(schema + offset, 4096 - offset, ",");
            }
            firstRequired = false;
            
            offset += snprintf(schema + offset, 4096 - offset, "\"%s\"", paramDefs[i].name);
        }
    }
    
    // Close schema
    offset += snprintf(schema + offset, 4096 - offset, "]}}");
    
    return schema;
}

char* MCP_ToolCreateResult(const MCP_ToolParam* params, int paramCount) {
    if (params == NULL || paramCount <= 0) {
        return strdup("{}");
    }
    
    // Allocate a buffer for the result
    char* result = (char*)malloc(4096);  // Adjust size as needed
    if (result == NULL) {
        return NULL;
    }
    
    // Start result
    int offset = 0;
    offset += snprintf(result + offset, 4096 - offset, "{");
    
    // Add parameters
    for (int i = 0; i < paramCount; i++) {
        const MCP_ToolParam* param = &params[i];
        
        // Add comma if not first parameter
        if (i > 0) {
            offset += snprintf(result + offset, 4096 - offset, ",");
        }
        
        // Add parameter name
        offset += snprintf(result + offset, 4096 - offset, "\"%s\":", param->name);
        
        // Add parameter value based on type
        switch (param->type) {
            case MCP_TOOL_PARAM_TYPE_BOOL:
                offset += snprintf(result + offset, 4096 - offset, 
                                 "%s", param->value.boolValue ? "true" : "false");
                break;
                
            case MCP_TOOL_PARAM_TYPE_INT:
                offset += snprintf(result + offset, 4096 - offset, 
                                 "%d", param->value.intValue);
                break;
                
            case MCP_TOOL_PARAM_TYPE_FLOAT:
                offset += snprintf(result + offset, 4096 - offset, 
                                 "%f", param->value.floatValue);
                break;
                
            case MCP_TOOL_PARAM_TYPE_STRING:
                if (param->value.stringValue != NULL) {
                    offset += snprintf(result + offset, 4096 - offset, 
                                     "\"%s\"", param->value.stringValue);
                } else {
                    offset += snprintf(result + offset, 4096 - offset, "null");
                }
                break;
                
            case MCP_TOOL_PARAM_TYPE_OBJECT:
                // For simplicity, we just use a placeholder for objects
                offset += snprintf(result + offset, 4096 - offset, "{}");
                break;
                
            case MCP_TOOL_PARAM_TYPE_ARRAY:
                // For simplicity, we just use a placeholder for arrays
                offset += snprintf(result + offset, 4096 - offset, "[]");
                break;
        }
    }
    
    // Close result
    offset += snprintf(result + offset, 4096 - offset, "}");
    
    return result;
}