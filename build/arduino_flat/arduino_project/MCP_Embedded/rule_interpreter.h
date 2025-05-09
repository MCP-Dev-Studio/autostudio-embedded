#ifndef MCP_RULE_INTERPRETER_H
#define MCP_RULE_INTERPRETER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Rule token types
 */
typedef enum {
    MCP_TOKEN_TYPE_NUMBER,
    MCP_TOKEN_TYPE_STRING,
    MCP_TOKEN_TYPE_BOOL,
    MCP_TOKEN_TYPE_VARIABLE,
    MCP_TOKEN_TYPE_OPERATOR,
    MCP_TOKEN_TYPE_FUNCTION,
    MCP_TOKEN_TYPE_PARENTHESIS_OPEN,
    MCP_TOKEN_TYPE_PARENTHESIS_CLOSE,
    MCP_TOKEN_TYPE_COMMA,
    MCP_TOKEN_TYPE_END
} MCP_TokenType;

/**
 * @brief Rule op types
 */
typedef enum {
    MCP_RULE_OP_ADD,
    MCP_RULE_OP_SUBTRACT,
    MCP_RULE_OP_MULTIPLY,
    MCP_RULE_OP_DIVIDE,
    MCP_RULE_OP_MODULO,
    MCP_RULE_OP_EQUAL,
    MCP_RULE_OP_NOT_EQUAL,
    MCP_RULE_OP_GREATER_THAN,
    MCP_RULE_OP_LESS_THAN,
    MCP_RULE_OP_GREATER_EQUAL,
    MCP_RULE_OP_LESS_EQUAL,
    MCP_RULE_OP_AND,
    MCP_RULE_OP_OR,
    MCP_RULE_OP_NOT
} MCP_RuleOperator;

/**
 * @brief Rule token
 */
typedef struct {
    MCP_TokenType type;
    union {
        double numberValue;
        char* stringValue;
        bool boolValue;
        char* variableName;
        MCP_RuleOperator operatorType;
        char* functionName;
    } value;
} MCP_Token;

/**
 * @brief Rule value types
 */
typedef enum {
    MCP_RULE_VALUE_NULL,
    MCP_RULE_VALUE_NUMBER,
    MCP_RULE_VALUE_STRING,
    MCP_RULE_VALUE_BOOL
} MCP_RuleValueType;

/**
 * @brief Rule value
 */
typedef struct {
    MCP_RuleValueType type;
    union {
        double numberValue;
        char* stringValue;
        bool boolValue;
    } value;
} MCP_RuleValue;

/**
 * @brief Initialize the rule interpreter
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_RuleInterpreterInit(void);

/**
 * @brief Evaluate a rule expression
 * 
 * @param expression Rule expression string
 * @return MCP_RuleValue Result of evaluation
 */
MCP_RuleValue MCP_RuleEvaluate(const char* expression);

/**
 * @brief Register a variable for rule evaluation
 * 
 * @param name Variable name
 * @param value Variable value
 * @return int 0 on success, negative error code on failure
 */
int MCP_RuleRegisterVariable(const char* name, MCP_RuleValue value);

/**
 * @brief Register a function for rule evaluation
 * 
 * @param name Function name
 * @param handler Function handler
 * @return int 0 on success, negative error code on failure
 */
typedef MCP_RuleValue (*MCP_RuleFunctionHandler)(MCP_RuleValue* params, int paramCount);
int MCP_RuleRegisterFunction(const char* name, MCP_RuleFunctionHandler handler);

/**
 * @brief Create a number rule value
 * 
 * @param value Number value
 * @return MCP_RuleValue Rule value
 */
MCP_RuleValue MCP_RuleCreateNumberValue(double value);

/**
 * @brief Create a string rule value
 * 
 * @param value String value
 * @return MCP_RuleValue Rule value
 */
MCP_RuleValue MCP_RuleCreateStringValue(const char* value);

/**
 * @brief Create a boolean rule value
 * 
 * @param value Boolean value
 * @return MCP_RuleValue Rule value
 */
MCP_RuleValue MCP_RuleCreateBoolValue(bool value);

/**
 * @brief Free a rule value (if it contains dynamically allocated memory)
 * 
 * @param value Rule value to free
 */
void MCP_RuleFreeValue(MCP_RuleValue value);

#endif /* MCP_RULE_INTERPRETER_H */