#include "rule_interpreter.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

// Variable structure
typedef struct Variable {
    char* name;
    MCP_RuleValue value;
    struct Variable* next;
} Variable;

// Function structure
typedef struct Function {
    char* name;
    MCP_RuleFunctionHandler handler;
    struct Function* next;
} Function;

// Internal state
static Variable* s_variables = NULL;
static Function* s_functions = NULL;
static bool s_initialized = false;

// Tokenizer context
typedef struct {
    const char* input;
    size_t position;
    MCP_Token current;
} TokenizerContext;

// Helper function to check if character is operator
static bool isOperatorChar(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || 
           c == '=' || c == '!' || c == '<' || c == '>' || c == '&' || c == '|';
}

// Helper function to check if operator is binary
static bool isBinaryOperator(MCP_RuleOperator op) {
    return op >= MCP_RULE_OP_ADD && op <= MCP_RULE_OP_LESS_EQUAL;
}

// Helper function to get operator precedence
static int getOperatorPrecedence(MCP_RuleOperator op) {
    switch (op) {
        case MCP_RULE_OP_MULTIPLY:
        case MCP_RULE_OP_DIVIDE:
        case MCP_RULE_OP_MODULO:
            return 5;
            
        case MCP_RULE_OP_ADD:
        case MCP_RULE_OP_SUBTRACT:
            return 4;
            
        case MCP_RULE_OP_GREATER_THAN:
        case MCP_RULE_OP_LESS_THAN:
        case MCP_RULE_OP_GREATER_EQUAL:
        case MCP_RULE_OP_LESS_EQUAL:
            return 3;
            
        case MCP_RULE_OP_EQUAL:
        case MCP_RULE_OP_NOT_EQUAL:
            return 2;
            
        case MCP_RULE_OP_AND:
            return 1;
            
        case MCP_RULE_OP_OR:
            return 0;
            
        case MCP_RULE_OP_NOT:
            return 6;
            
        default:
            return -1;
    }
}

int MCP_RuleInterpreterInit(void) {
    if (s_initialized) {
        return -1;  // Already initialized
    }
    
    s_variables = NULL;
    s_functions = NULL;
    s_initialized = true;
    
    return 0;
}

static Variable* findVariable(const char* name) {
    Variable* current = s_variables;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static Function* findFunction(const char* name) {
    Function* current = s_functions;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int MCP_RuleRegisterVariable(const char* name, MCP_RuleValue value) {
    if (!s_initialized || name == NULL) {
        return -1;
    }
    
    // Check if variable already exists
    Variable* existingVar = findVariable(name);
    if (existingVar != NULL) {
        // Update existing variable
        MCP_RuleFreeValue(existingVar->value);
        existingVar->value = value;
        return 0;
    }
    
    // Create new variable
    Variable* newVar = (Variable*)malloc(sizeof(Variable));
    if (newVar == NULL) {
        return -2;  // Memory allocation failed
    }
    
    newVar->name = strdup(name);
    if (newVar->name == NULL) {
        free(newVar);
        return -3;  // Memory allocation failed
    }
    
    newVar->value = value;
    
    // Add to linked list
    newVar->next = s_variables;
    s_variables = newVar;
    
    return 0;
}

int MCP_RuleRegisterFunction(const char* name, MCP_RuleFunctionHandler handler) {
    if (!s_initialized || name == NULL || handler == NULL) {
        return -1;
    }
    
    // Check if function already exists
    Function* existingFunc = findFunction(name);
    if (existingFunc != NULL) {
        // Update existing function
        existingFunc->handler = handler;
        return 0;
    }
    
    // Create new function
    Function* newFunc = (Function*)malloc(sizeof(Function));
    if (newFunc == NULL) {
        return -2;  // Memory allocation failed
    }
    
    newFunc->name = strdup(name);
    if (newFunc->name == NULL) {
        free(newFunc);
        return -3;  // Memory allocation failed
    }
    
    newFunc->handler = handler;
    
    // Add to linked list
    newFunc->next = s_functions;
    s_functions = newFunc;
    
    return 0;
}

// Token functions
static void initTokenizer(TokenizerContext* ctx, const char* input) {
    ctx->input = input;
    ctx->position = 0;
}

static void getNextToken(TokenizerContext* ctx) {
    const char* input = ctx->input;
    size_t pos = ctx->position;
    
    // Skip whitespace
    while (input[pos] != '\0' && isspace(input[pos])) {
        pos++;
    }
    
    // End of input
    if (input[pos] == '\0') {
        ctx->current.type = MCP_TOKEN_TYPE_END;
        ctx->position = pos;
        return;
    }
    
    // Number
    if (isdigit(input[pos]) || (input[pos] == '.' && isdigit(input[pos + 1]))) {
        char* endPtr;
        double value = strtod(&input[pos], &endPtr);
        
        ctx->current.type = MCP_TOKEN_TYPE_NUMBER;
        ctx->current.value.numberValue = value;
        ctx->position = pos + (endPtr - &input[pos]);
        return;
    }
    
    // String
    if (input[pos] == '"' || input[pos] == '\'') {
        char quoteChar = input[pos];
        pos++;  // Skip opening quote
        
        size_t start = pos;
        
        // Find closing quote
        while (input[pos] != '\0' && input[pos] != quoteChar) {
            pos++;
        }
        
        if (input[pos] == quoteChar) {
            // Compute string length
            size_t len = pos - start;
            
            // Allocate and copy string
            char* str = (char*)malloc(len + 1);
            if (str != NULL) {
                strncpy(str, &input[start], len);
                str[len] = '\0';
                
                ctx->current.type = MCP_TOKEN_TYPE_STRING;
                ctx->current.value.stringValue = str;
            } else {
                // Memory allocation failed
                ctx->current.type = MCP_TOKEN_TYPE_END;
            }
            
            pos++;  // Skip closing quote
        } else {
            // Unterminated string
            ctx->current.type = MCP_TOKEN_TYPE_END;
        }
        
        ctx->position = pos;
        return;
    }
    
    // Identifier (variable or function or bool)
    if (isalpha(input[pos]) || input[pos] == '_') {
        size_t start = pos;
        
        // Find end of identifier
        while (input[pos] != '\0' && (isalnum(input[pos]) || input[pos] == '_')) {
            pos++;
        }
        
        // Compute identifier length
        size_t len = pos - start;
        
        // Check for known identifiers
        if (len == 4 && strncmp(&input[start], "true", 4) == 0) {
            ctx->current.type = MCP_TOKEN_TYPE_BOOL;
            ctx->current.value.boolValue = true;
        } else if (len == 5 && strncmp(&input[start], "false", 5) == 0) {
            ctx->current.type = MCP_TOKEN_TYPE_BOOL;
            ctx->current.value.boolValue = false;
        } else {
            // Check if it's a function (followed by opening parenthesis)
            size_t tempPos = pos;
            
            // Skip whitespace
            while (input[tempPos] != '\0' && isspace(input[tempPos])) {
                tempPos++;
            }
            
            if (input[tempPos] == '(') {
                // It's a function
                char* name = (char*)malloc(len + 1);
                if (name != NULL) {
                    strncpy(name, &input[start], len);
                    name[len] = '\0';
                    
                    ctx->current.type = MCP_TOKEN_TYPE_FUNCTION;
                    ctx->current.value.functionName = name;
                } else {
                    // Memory allocation failed
                    ctx->current.type = MCP_TOKEN_TYPE_END;
                }
            } else {
                // It's a variable
                char* name = (char*)malloc(len + 1);
                if (name != NULL) {
                    strncpy(name, &input[start], len);
                    name[len] = '\0';
                    
                    ctx->current.type = MCP_TOKEN_TYPE_VARIABLE;
                    ctx->current.value.variableName = name;
                } else {
                    // Memory allocation failed
                    ctx->current.type = MCP_TOKEN_TYPE_END;
                }
            }
        }
        
        ctx->position = pos;
        return;
    }
    
    // Operator
    if (isOperatorChar(input[pos])) {
        size_t start = pos;
        
        // Multi-character operators
        if (input[pos] == '=' && input[pos + 1] == '=') {
            ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
            ctx->current.value.operatorType = MCP_RULE_OP_EQUAL;
            ctx->position = pos + 2;
            return;
        } else if (input[pos] == '!' && input[pos + 1] == '=') {
            ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
            ctx->current.value.operatorType = MCP_RULE_OP_NOT_EQUAL;
            ctx->position = pos + 2;
            return;
        } else if (input[pos] == '>' && input[pos + 1] == '=') {
            ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
            ctx->current.value.operatorType = MCP_RULE_OP_GREATER_EQUAL;
            ctx->position = pos + 2;
            return;
        } else if (input[pos] == '<' && input[pos + 1] == '=') {
            ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
            ctx->current.value.operatorType = MCP_RULE_OP_LESS_EQUAL;
            ctx->position = pos + 2;
            return;
        } else if (input[pos] == '&' && input[pos + 1] == '&') {
            ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
            ctx->current.value.operatorType = MCP_RULE_OP_AND;
            ctx->position = pos + 2;
            return;
        } else if (input[pos] == '|' && input[pos + 1] == '|') {
            ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
            ctx->current.value.operatorType = MCP_RULE_OP_OR;
            ctx->position = pos + 2;
            return;
        }
        
        // Single-character operators
        switch (input[pos]) {
            case '+':
                ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
                ctx->current.value.operatorType = MCP_RULE_OP_ADD;
                break;
                
            case '-':
                ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
                ctx->current.value.operatorType = MCP_RULE_OP_SUBTRACT;
                break;
                
            case '*':
                ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
                ctx->current.value.operatorType = MCP_RULE_OP_MULTIPLY;
                break;
                
            case '/':
                ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
                ctx->current.value.operatorType = MCP_RULE_OP_DIVIDE;
                break;
                
            case '%':
                ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
                ctx->current.value.operatorType = MCP_RULE_OP_MODULO;
                break;
                
            case '>':
                ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
                ctx->current.value.operatorType = MCP_RULE_OP_GREATER_THAN;
                break;
                
            case '<':
                ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
                ctx->current.value.operatorType = MCP_RULE_OP_LESS_THAN;
                break;
                
            case '!':
                ctx->current.type = MCP_TOKEN_TYPE_OPERATOR;
                ctx->current.value.operatorType = MCP_RULE_OP_NOT;
                break;
                
            default:
                // Unknown operator
                ctx->current.type = MCP_TOKEN_TYPE_END;
                break;
        }
        
        ctx->position = pos + 1;
        return;
    }
    
    // Parenthesis and comma
    if (input[pos] == '(') {
        ctx->current.type = MCP_TOKEN_TYPE_PARENTHESIS_OPEN;
        ctx->position = pos + 1;
        return;
    } else if (input[pos] == ')') {
        ctx->current.type = MCP_TOKEN_TYPE_PARENTHESIS_CLOSE;
        ctx->position = pos + 1;
        return;
    } else if (input[pos] == ',') {
        ctx->current.type = MCP_TOKEN_TYPE_COMMA;
        ctx->position = pos + 1;
        return;
    }
    
    // Unknown token
    ctx->current.type = MCP_TOKEN_TYPE_END;
    ctx->position = pos + 1;
}

// Helper function to resolve variable values
static MCP_RuleValue resolveVariable(const char* name) {
    Variable* var = findVariable(name);
    if (var != NULL) {
        // Return a copy of the value
        switch (var->value.type) {
            case MCP_RULE_VALUE_NUMBER:
                return MCP_RuleCreateNumberValue(var->value.value.numberValue);
                
            case MCP_RULE_VALUE_STRING:
                return MCP_RuleCreateStringValue(var->value.value.stringValue);
                
            case MCP_RULE_VALUE_BOOL:
                return MCP_RuleCreateBoolValue(var->value.value.boolValue);
                
            case MCP_RULE_VALUE_NULL:
                // Fall through to default
                break;
        }
    }
    
    // Variable not found or null value
    return (MCP_RuleValue){ .type = MCP_RULE_VALUE_NULL };
}

// Helper function to handle function calls
static MCP_RuleValue callFunction(const char* name, TokenizerContext* ctx) {
    Function* func = findFunction(name);
    if (func == NULL) {
        // Function not found
        return (MCP_RuleValue){ .type = MCP_RULE_VALUE_NULL };
    }
    
    // Parse arguments
    MCP_RuleValue params[10];  // Maximum 10 parameters for simplicity
    int paramCount = 0;
    
    getNextToken(ctx);  // Consume opening parenthesis
    
    if (ctx->current.type != MCP_TOKEN_TYPE_PARENTHESIS_CLOSE) {
        // Parse first parameter
        params[paramCount++] = MCP_RuleEvaluate(ctx->input + ctx->position);
        
        // Parse additional parameters
        while (ctx->current.type == MCP_TOKEN_TYPE_COMMA && paramCount < 10) {
            getNextToken(ctx);  // Consume comma
            params[paramCount++] = MCP_RuleEvaluate(ctx->input + ctx->position);
        }
    }
    
    if (ctx->current.type != MCP_TOKEN_TYPE_PARENTHESIS_CLOSE) {
        // Invalid syntax
        for (int i = 0; i < paramCount; i++) {
            MCP_RuleFreeValue(params[i]);
        }
        return (MCP_RuleValue){ .type = MCP_RULE_VALUE_NULL };
    }
    
    getNextToken(ctx);  // Consume closing parenthesis
    
    // Call function handler
    MCP_RuleValue result = func->handler(params, paramCount);
    
    // Free parameter values
    for (int i = 0; i < paramCount; i++) {
        MCP_RuleFreeValue(params[i]);
    }
    
    return result;
}

// Recursive descent parser
static MCP_RuleValue parseExpression(TokenizerContext* ctx);
static MCP_RuleValue parseTerm(TokenizerContext* ctx);
static MCP_RuleValue parseFactor(TokenizerContext* ctx);

static MCP_RuleValue parseExpression(TokenizerContext* ctx) {
    MCP_RuleValue left = parseTerm(ctx);
    
    while (ctx->current.type == MCP_TOKEN_TYPE_OPERATOR) {
        MCP_RuleOperator op = ctx->current.value.operatorType;
        
        if (getOperatorPrecedence(op) < getOperatorPrecedence(MCP_RULE_OP_MULTIPLY)) {
            getNextToken(ctx);
            MCP_RuleValue right = parseTerm(ctx);
            
            // Apply operator
            // This is a simplified implementation
            // In a real implementation, you'd handle different types and operations
            
            // For now, just handle numeric values
            if (left.type == MCP_RULE_VALUE_NUMBER && right.type == MCP_RULE_VALUE_NUMBER) {
                switch (op) {
                    case MCP_RULE_OP_ADD:
                        left.value.numberValue += right.value.numberValue;
                        break;
                        
                    case MCP_RULE_OP_SUBTRACT:
                        left.value.numberValue -= right.value.numberValue;
                        break;
                        
                    default:
                        // Unsupported operator
                        break;
                }
            }
            
            MCP_RuleFreeValue(right);
        } else {
            break;
        }
    }
    
    return left;
}

static MCP_RuleValue parseTerm(TokenizerContext* ctx) {
    MCP_RuleValue left = parseFactor(ctx);
    
    while (ctx->current.type == MCP_TOKEN_TYPE_OPERATOR) {
        MCP_RuleOperator op = ctx->current.value.operatorType;
        
        if (getOperatorPrecedence(op) == getOperatorPrecedence(MCP_RULE_OP_MULTIPLY)) {
            getNextToken(ctx);
            MCP_RuleValue right = parseFactor(ctx);
            
            // Apply operator
            // This is a simplified implementation
            // In a real implementation, you'd handle different types and operations
            
            // For now, just handle numeric values
            if (left.type == MCP_RULE_VALUE_NUMBER && right.type == MCP_RULE_VALUE_NUMBER) {
                switch (op) {
                    case MCP_RULE_OP_MULTIPLY:
                        left.value.numberValue *= right.value.numberValue;
                        break;
                        
                    case MCP_RULE_OP_DIVIDE:
                        if (right.value.numberValue != 0) {
                            left.value.numberValue /= right.value.numberValue;
                        } else {
                            // Division by zero
                            left.type = MCP_RULE_VALUE_NULL;
                        }
                        break;
                        
                    case MCP_RULE_OP_MODULO:
                        if (right.value.numberValue != 0) {
                            left.value.numberValue = fmod(left.value.numberValue, right.value.numberValue);
                        } else {
                            // Division by zero
                            left.type = MCP_RULE_VALUE_NULL;
                        }
                        break;
                        
                    default:
                        // Unsupported operator
                        break;
                }
            }
            
            MCP_RuleFreeValue(right);
        } else {
            break;
        }
    }
    
    return left;
}

static MCP_RuleValue parseFactor(TokenizerContext* ctx) {
    MCP_RuleValue result;
    
    switch (ctx->current.type) {
        case MCP_TOKEN_TYPE_NUMBER:
            result = MCP_RuleCreateNumberValue(ctx->current.value.numberValue);
            getNextToken(ctx);
            break;
            
        case MCP_TOKEN_TYPE_STRING:
            result = MCP_RuleCreateStringValue(ctx->current.value.stringValue);
            getNextToken(ctx);
            break;
            
        case MCP_TOKEN_TYPE_BOOL:
            result = MCP_RuleCreateBoolValue(ctx->current.value.boolValue);
            getNextToken(ctx);
            break;
            
        case MCP_TOKEN_TYPE_VARIABLE:
            result = resolveVariable(ctx->current.value.variableName);
            getNextToken(ctx);
            break;
            
        case MCP_TOKEN_TYPE_FUNCTION:
            result = callFunction(ctx->current.value.functionName, ctx);
            break;
            
        case MCP_TOKEN_TYPE_PARENTHESIS_OPEN:
            getNextToken(ctx);
            result = parseExpression(ctx);
            
            if (ctx->current.type != MCP_TOKEN_TYPE_PARENTHESIS_CLOSE) {
                // Missing closing parenthesis
                MCP_RuleFreeValue(result);
                result.type = MCP_RULE_VALUE_NULL;
            } else {
                getNextToken(ctx);
            }
            break;
            
        default:
            // Invalid token
            result.type = MCP_RULE_VALUE_NULL;
            break;
    }
    
    return result;
}

MCP_RuleValue MCP_RuleEvaluate(const char* expression) {
    if (!s_initialized || expression == NULL) {
        return (MCP_RuleValue){ .type = MCP_RULE_VALUE_NULL };
    }
    
    TokenizerContext ctx;
    initTokenizer(&ctx, expression);
    
    getNextToken(&ctx);
    MCP_RuleValue result = parseExpression(&ctx);
    
    return result;
}

MCP_RuleValue MCP_RuleCreateNumberValue(double value) {
    MCP_RuleValue result;
    result.type = MCP_RULE_VALUE_NUMBER;
    result.value.numberValue = value;
    return result;
}

MCP_RuleValue MCP_RuleCreateStringValue(const char* value) {
    MCP_RuleValue result;
    
    if (value != NULL) {
        result.type = MCP_RULE_VALUE_STRING;
        result.value.stringValue = strdup(value);
        
        if (result.value.stringValue == NULL) {
            result.type = MCP_RULE_VALUE_NULL;
        }
    } else {
        result.type = MCP_RULE_VALUE_NULL;
    }
    
    return result;
}

MCP_RuleValue MCP_RuleCreateBoolValue(bool value) {
    MCP_RuleValue result;
    result.type = MCP_RULE_VALUE_BOOL;
    result.value.boolValue = value;
    return result;
}

void MCP_RuleFreeValue(MCP_RuleValue value) {
    if (value.type == MCP_RULE_VALUE_STRING && value.value.stringValue != NULL) {
        free(value.value.stringValue);
    }
}