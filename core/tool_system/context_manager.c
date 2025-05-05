/**
 * @file context_manager.c
 * @brief Implementation of execution context and variable management
 */
#include "context_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create a new execution context
MCP_ExecutionContext* MCP_ContextCreate(const char* name, MCP_ExecutionContext* parent, int maxVariables) {
    if (maxVariables <= 0) {
        maxVariables = 32; // Default size
    }

    MCP_ExecutionContext* context = (MCP_ExecutionContext*)malloc(sizeof(MCP_ExecutionContext));
    if (context == NULL) {
        return NULL;
    }

    // Allocate variable arrays
    context->variables = (MCP_Variable*)calloc(maxVariables, sizeof(MCP_Variable));
    context->variableNames = (char**)calloc(maxVariables, sizeof(char*));

    if (context->variables == NULL || context->variableNames == NULL) {
        free(context->variables);
        free(context->variableNames);
        free(context);
        return NULL;
    }

    // Initialize fields
    context->name = name ? strdup(name) : NULL;
    context->variableCount = 0;
    context->maxVariables = maxVariables;
    context->parent = parent;

    return context;
}

// Free an execution context
void MCP_ContextFree(MCP_ExecutionContext* context) {
    if (context == NULL) {
        return;
    }

    // Free name
    if (context->name != NULL) {
        free(context->name);
    }

    // Free variables
    if (context->variables != NULL) {
        for (int i = 0; i < context->variableCount; i++) {
            MCP_VariableFree(&context->variables[i]);
        }
        free(context->variables);
    }

    // Free variable names
    if (context->variableNames != NULL) {
        for (int i = 0; i < context->variableCount; i++) {
            if (context->variableNames[i] != NULL) {
                free(context->variableNames[i]);
            }
        }
        free(context->variableNames);
    }

    // Free context
    free(context);
}

// Find a variable in the context
static int findVariable(MCP_ExecutionContext* context, const char* name) {
    if (context == NULL || name == NULL) {
        return -1;
    }

    for (int i = 0; i < context->variableCount; i++) {
        if (context->variableNames[i] != NULL && strcmp(context->variableNames[i], name) == 0) {
            return i;
        }
    }

    return -1;
}

// Set a variable in the context
int MCP_ContextSetVariable(MCP_ExecutionContext* context, const char* name, const MCP_Variable* value) {
    if (context == NULL || name == NULL || value == NULL) {
        return -1;
    }

    // Check if variable already exists
    int index = findVariable(context, name);

    if (index >= 0) {
        // Variable exists, update it
        MCP_VariableFree(&context->variables[index]);

        // Copy value
        context->variables[index].type = value->type;
        switch (value->type) {
            case MCP_VAR_TYPE_BOOL:
                context->variables[index].value.boolValue = value->value.boolValue;
                break;

            case MCP_VAR_TYPE_INT:
                context->variables[index].value.intValue = value->value.intValue;
                break;

            case MCP_VAR_TYPE_FLOAT:
                context->variables[index].value.floatValue = value->value.floatValue;
                break;

            case MCP_VAR_TYPE_STRING:
                if (value->value.stringValue != NULL) {
                    context->variables[index].value.stringValue = strdup(value->value.stringValue);
                } else {
                    context->variables[index].value.stringValue = NULL;
                }
                break;

            case MCP_VAR_TYPE_OBJECT:
                context->variables[index].value.objectValue = value->value.objectValue;
                break;

            case MCP_VAR_TYPE_ARRAY:
                context->variables[index].value.arrayValue = value->value.arrayValue;
                break;

            case MCP_VAR_TYPE_NULL:
            default:
                // Nothing to copy
                break;
        }
    } else {
        // Variable doesn't exist, create it
        if (context->variableCount >= context->maxVariables) {
            return -2; // No space for new variable
        }

        // Set name
        context->variableNames[context->variableCount] = strdup(name);
        if (context->variableNames[context->variableCount] == NULL) {
            return -3; // Memory allocation failed
        }

        // Copy value
        context->variables[context->variableCount].type = value->type;
        switch (value->type) {
            case MCP_VAR_TYPE_BOOL:
                context->variables[context->variableCount].value.boolValue = value->value.boolValue;
                break;

            case MCP_VAR_TYPE_INT:
                context->variables[context->variableCount].value.intValue = value->value.intValue;
                break;

            case MCP_VAR_TYPE_FLOAT:
                context->variables[context->variableCount].value.floatValue = value->value.floatValue;
                break;

            case MCP_VAR_TYPE_STRING:
                if (value->value.stringValue != NULL) {
                    context->variables[context->variableCount].value.stringValue = strdup(value->value.stringValue);
                } else {
                    context->variables[context->variableCount].value.stringValue = NULL;
                }
                break;

            case MCP_VAR_TYPE_OBJECT:
                context->variables[context->variableCount].value.objectValue = value->value.objectValue;
                break;

            case MCP_VAR_TYPE_ARRAY:
                context->variables[context->variableCount].value.arrayValue = value->value.arrayValue;
                break;

            case MCP_VAR_TYPE_NULL:
            default:
                // Nothing to copy
                break;
        }

        context->variableCount++;
    }

    return 0;
}

// Get a variable from the context
MCP_Variable MCP_ContextGetVariable(MCP_ExecutionContext* context, const char* name) {
    MCP_Variable nullVar = { .type = MCP_VAR_TYPE_NULL };

    if (context == NULL || name == NULL) {
        return nullVar;
    }

    // Search in current context
    int index = findVariable(context, name);
    if (index >= 0) {
        return context->variables[index];
    }

    // Search in parent context if exists
    if (context->parent != NULL) {
        return MCP_ContextGetVariable(context->parent, name);
    }

    // Not found
    return nullVar;
}

// Check if a variable exists in the context
bool MCP_ContextHasVariable(MCP_ExecutionContext* context, const char* name) {
    if (context == NULL || name == NULL) {
        return false;
    }

    // Search in current context
    int index = findVariable(context, name);
    if (index >= 0) {
        return true;
    }

    // Search in parent context if exists
    if (context->parent != NULL) {
        return MCP_ContextHasVariable(context->parent, name);
    }

    return false;
}

// Parse variable template pattern: ${varName|defaultValue}
static bool parseVarPattern(const char* pattern, char** varName, char** defaultValue) {
    if (pattern == NULL || varName == NULL || defaultValue == NULL) {
        return false;
    }

    // Check for pattern start
    if (strncmp(pattern, "${", 2) != 0) {
        return false;
    }

    // Find closing brace
    const char* end = strchr(pattern + 2, '}');
    if (end == NULL) {
        return false;
    }

    // Find pipe (|) for default value
    const char* pipe = strchr(pattern + 2, '|');

    if (pipe != NULL && pipe < end) {
        // Pattern has default value
        size_t nameLen = pipe - (pattern + 2);
        size_t defaultLen = end - (pipe + 1);

        *varName = (char*)malloc(nameLen + 1);
        if (*varName == NULL) {
            return false;
        }

        *defaultValue = (char*)malloc(defaultLen + 1);
        if (*defaultValue == NULL) {
            free(*varName);
            *varName = NULL;
            return false;
        }

        strncpy(*varName, pattern + 2, nameLen);
        (*varName)[nameLen] = '\0';

        strncpy(*defaultValue, pipe + 1, defaultLen);
        (*defaultValue)[defaultLen] = '\0';
    } else {
        // Pattern has no default value
        size_t nameLen = end - (pattern + 2);

        *varName = (char*)malloc(nameLen + 1);
        if (*varName == NULL) {
            return false;
        }

        strncpy(*varName, pattern + 2, nameLen);
        (*varName)[nameLen] = '\0';

        *defaultValue = NULL;
    }

    return true;
}

// Convert variable to string
static char* variableToString(const MCP_Variable* variable) {
    if (variable == NULL) {
        return strdup("null");
    }

    char buffer[64];

    switch (variable->type) {
        case MCP_VAR_TYPE_BOOL:
            return strdup(variable->value.boolValue ? "true" : "false");

        case MCP_VAR_TYPE_INT:
            snprintf(buffer, sizeof(buffer), "%d", variable->value.intValue);
            return strdup(buffer);

        case MCP_VAR_TYPE_FLOAT:
            snprintf(buffer, sizeof(buffer), "%f", variable->value.floatValue);
            return strdup(buffer);

        case MCP_VAR_TYPE_STRING:
            return variable->value.stringValue ? strdup(variable->value.stringValue) : strdup("");

        case MCP_VAR_TYPE_OBJECT:
            return strdup("{}");  // Simplified representation

        case MCP_VAR_TYPE_ARRAY:
            return strdup("[]");  // Simplified representation

        case MCP_VAR_TYPE_NULL:
        default:
            return strdup("null");
    }
}

// Substitute variables in a template string
char* MCP_ContextSubstituteVariables(MCP_ExecutionContext* context, const char* template) {
    if (context == NULL || template == NULL) {
        return NULL;
    }

    // Initial allocation
    size_t resultSize = strlen(template) * 2;  // Start with double the template size
    char* result = (char*)malloc(resultSize);
    if (result == NULL) {
        return NULL;
    }

    size_t resultOffset = 0;
    const char* current = template;

    while (*current != '\0') {
        // Look for variable pattern start
        const char* varStart = strstr(current, "${");

        if (varStart == NULL) {
            // No more variables, copy the rest of the template
            size_t remainingLen = strlen(current);

            // Ensure result buffer has enough space
            if (resultOffset + remainingLen + 1 > resultSize) {
                resultSize = resultOffset + remainingLen + 1;
                char* newResult = (char*)realloc(result, resultSize);
                if (newResult == NULL) {
                    free(result);
                    return NULL;
                }
                result = newResult;
            }

            strcpy(result + resultOffset, current);
            resultOffset += remainingLen;
            break;
        }

        // Copy text before variable
        size_t prefixLen = varStart - current;

        // Ensure result buffer has enough space
        if (resultOffset + prefixLen > resultSize) {
            resultSize = resultOffset + prefixLen + 1;
            char* newResult = (char*)realloc(result, resultSize);
            if (newResult == NULL) {
                free(result);
                return NULL;
            }
            result = newResult;
        }

        strncpy(result + resultOffset, current, prefixLen);
        resultOffset += prefixLen;

        // Parse variable pattern
        char* varName = NULL;
        char* defaultValue = NULL;

        if (parseVarPattern(varStart, &varName, &defaultValue)) {
            // Look up variable
            MCP_Variable var = MCP_ContextGetVariable(context, varName);

            // Convert variable to string
            char* varStr = NULL;

            if (var.type == MCP_VAR_TYPE_NULL && defaultValue != NULL) {
                // Use default value
                varStr = strdup(defaultValue);
            } else {
                // Use variable value
                varStr = variableToString(&var);
            }

            if (varStr != NULL) {
                // Append variable value to result
                size_t varStrLen = strlen(varStr);

                // Ensure result buffer has enough space
                if (resultOffset + varStrLen + 1 > resultSize) {
                    resultSize = resultOffset + varStrLen + 1;
                    char* newResult = (char*)realloc(result, resultSize);
                    if (newResult == NULL) {
                        free(result);
                        free(varName);
                        free(defaultValue);
                        free(varStr);
                        return NULL;
                    }
                    result = newResult;
                }

                strcpy(result + resultOffset, varStr);
                resultOffset += varStrLen;

                free(varStr);
            }

            // Move current position past variable pattern
            current = strchr(varStart, '}') + 1;

            // Free memory
            free(varName);
            if (defaultValue != NULL) {
                free(defaultValue);
            }
        } else {
            // Invalid pattern, copy it as-is
            size_t patternLen = 2;  // ${

            // Ensure result buffer has enough space
            if (resultOffset + patternLen + 1 > resultSize) {
                resultSize = resultOffset + patternLen + 1;
                char* newResult = (char*)realloc(result, resultSize);
                if (newResult == NULL) {
                    free(result);
                    return NULL;
                }
                result = newResult;
            }

            strncpy(result + resultOffset, varStart, patternLen);
            resultOffset += patternLen;

            // Move current position past ${ 
            current = varStart + 2;
        }
    }

    // Ensure null termination
    result[resultOffset] = '\0';

    return result;
}

// Store tool result in a variable
int MCP_ContextStoreToolResult(MCP_ExecutionContext* context, const char* name, const MCP_ToolResult* result) {
    if (context == NULL || name == NULL || result == NULL) {
        return -1;
    }

    MCP_Variable var;

    if (result->status == MCP_TOOL_RESULT_SUCCESS) {
        if (result->resultJson != NULL) {
            // Store as string
            var.type = MCP_VAR_TYPE_STRING;
            var.value.stringValue = strdup(result->resultJson);
        } else if (result->resultData != NULL) {
            // Store as object
            var.type = MCP_VAR_TYPE_OBJECT;
            var.value.objectValue = result->resultData;
        } else {
            // Empty result
            var.type = MCP_VAR_TYPE_NULL;
        }
    } else {
        // Error result
        var.type = MCP_VAR_TYPE_NULL;
    }

    int ret = MCP_ContextSetVariable(context, name, &var);

    // Free string if created
    if (var.type == MCP_VAR_TYPE_STRING && var.value.stringValue != NULL) {
        free(var.value.stringValue);
    }

    return ret;
}

// Create variables from different types
MCP_Variable MCP_VariableCreateNull(void) {
    MCP_Variable var;
    var.type = MCP_VAR_TYPE_NULL;
    return var;
}

MCP_Variable MCP_VariableCreateBool(bool value) {
    MCP_Variable var;
    var.type = MCP_VAR_TYPE_BOOL;
    var.value.boolValue = value;
    return var;
}

MCP_Variable MCP_VariableCreateInt(int32_t value) {
    MCP_Variable var;
    var.type = MCP_VAR_TYPE_INT;
    var.value.intValue = value;
    return var;
}

MCP_Variable MCP_VariableCreateFloat(float value) {
    MCP_Variable var;
    var.type = MCP_VAR_TYPE_FLOAT;
    var.value.floatValue = value;
    return var;
}

MCP_Variable MCP_VariableCreateString(const char* value) {
    MCP_Variable var;

    if (value != NULL) {
        var.type = MCP_VAR_TYPE_STRING;
        var.value.stringValue = strdup(value);

        if (var.value.stringValue == NULL) {
            var.type = MCP_VAR_TYPE_NULL;
        }
    } else {
        var.type = MCP_VAR_TYPE_STRING;
        var.value.stringValue = NULL;
    }

    return var;
}

MCP_Variable MCP_VariableCreateObject(void* value) {
    MCP_Variable var;
    var.type = MCP_VAR_TYPE_OBJECT;
    var.value.objectValue = value;
    return var;
}

MCP_Variable MCP_VariableCreateObject(void* value) {
    MCP_Variable var;
    var.type = MCP_VAR_TYPE_OBJECT;
    var.value.objectValue = value;
    return var;
}

MCP_Variable MCP_VariableCreateArray(void* value) {
    MCP_Variable var;
    var.type = MCP_VAR_TYPE_ARRAY;
    var.value.arrayValue = value;
    return var;
}

// Free variable resources
void MCP_VariableFree(MCP_Variable* variable) {
    if (variable == NULL) {
        return;
    }

    // Free based on type
    switch (variable->type) {
        case MCP_VAR_TYPE_STRING:
            if (variable->value.stringValue != NULL) {
                free(variable->value.stringValue);
                variable->value.stringValue = NULL;
            }
            break;

            // For other types, no dynamic memory to free
        case MCP_VAR_TYPE_BOOL:
        case MCP_VAR_TYPE_INT:
        case MCP_VAR_TYPE_FLOAT:
        case MCP_VAR_TYPE_OBJECT:
        case MCP_VAR_TYPE_ARRAY:
        case MCP_VAR_TYPE_NULL:
        default:
            break;
    }

    // Reset type to null
    variable->type = MCP_VAR_TYPE_NULL;
}
