#ifndef MCP_AUTOMATION_ENGINE_H
#define MCP_AUTOMATION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Trigger types for automation rules
 */
typedef enum {
    MCP_TRIGGER_TYPE_CONDITION,   // Trigger when condition is met
    MCP_TRIGGER_TYPE_EVENT,       // Trigger on specific event
    MCP_TRIGGER_TYPE_SCHEDULE,    // Trigger on schedule
    MCP_TRIGGER_TYPE_MANUAL       // Trigger manually
} MCP_TriggerType;

/**
 * @brief Condition op types
 */
typedef enum {
    MCP_CONDITION_OP_EQUAL,
    MCP_CONDITION_OP_NOT_EQUAL,
    MCP_CONDITION_OP_GREATER_THAN,
    MCP_CONDITION_OP_LESS_THAN,
    MCP_CONDITION_OP_GREATER_EQUAL,
    MCP_CONDITION_OP_LESS_EQUAL,
    MCP_CONDITION_OP_CONTAINS,
    MCP_CONDITION_OP_NOT_CONTAINS,
    MCP_CONDITION_OP_STARTS_WITH,
    MCP_CONDITION_OP_ENDS_WITH
} MCP_ConditionOperator;

/**
 * @brief Action types
 */
typedef enum {
    MCP_ACTION_TYPE_ACTUATOR,     // Control an actuator
    MCP_ACTION_TYPE_TOOL,         // Execute a tool
    MCP_ACTION_TYPE_NOTIFICATION, // Send a notification
    MCP_ACTION_TYPE_CUSTOM        // Custom action
} MCP_ActionType;

/**
 * @brief Initialize the automation engine
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_AutomationInit(void);

/**
 * @brief Create a new automation rule from JSON
 * 
 * @param json JSON string with rule definition
 * @param length Length of JSON string
 * @return const char* Rule ID or NULL on failure (caller must NOT free)
 */
const char* MCP_AutomationCreateRule(const char* json, size_t length);

/**
 * @brief Set rule enabled state
 * 
 * @param ruleId Rule ID
 * @param enabled Enabled state
 * @return int 0 on success, negative error code on failure
 */
int MCP_AutomationSetRuleEnabled(const char* ruleId, bool enabled);

/**
 * @brief Delete a rule
 * 
 * @param ruleId Rule ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_AutomationDeleteRule(const char* ruleId);

/**
 * @brief Process automation rules
 * 
 * @param currentTimeMs Current system time in milliseconds
 */
void MCP_AutomationProcess(uint32_t currentTimeMs);

/**
 * @brief Check and evaluate a rule's triggers
 * 
 * @param ruleId Rule ID
 * @return bool True if triggered, false otherwise
 */
bool MCP_AutomationCheckTriggers(const char* ruleId);

/**
 * @brief Execute a rule's actions
 * 
 * @param ruleId Rule ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_AutomationExecuteActions(const char* ruleId);

/**
 * @brief Manually trigger a rule
 * 
 * @param ruleId Rule ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_AutomationTriggerRule(const char* ruleId);

/**
 * @brief Export all rules as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_AutomationExportRules(char* buffer, size_t bufferSize);

/**
 * @brief Import rules from JSON
 * 
 * @param json JSON string with rules
 * @param length Length of JSON string
 * @return int Number of rules imported or negative error code
 */
int MCP_AutomationImportRules(const char* json, size_t length);

#endif /* MCP_AUTOMATION_ENGINE_H */