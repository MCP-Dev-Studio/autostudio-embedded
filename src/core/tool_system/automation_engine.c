#include "automation_engine.h"
#include "tool_registry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef MCP_PLATFORM_ARDUINO
#include "platform_compatibility.h"
#endif

// Forward declaration of dependencies
extern int persistent_storage_write(const char* key, const void* data, size_t size);
extern int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize);
extern char* json_get_string_field(const char* json, const char* field);
extern bool json_validate_schema(const char* json, const char* schema);
extern bool json_get_bool_field(const char* json, const char* field, bool defaultValue);
extern void* json_get_array_field(const char* json, const char* field);

#ifndef MCP_PLATFORM_ARDUINO
// Forward declarations of internal functions
static void freeRuleTrigger(RuleTrigger* trigger);
static void freeRuleAction(RuleAction* action);
static void freeRule(AutomationRule* rule);
static bool evaluateCondition(RuleTrigger* trigger, uint32_t currentTimeMs);
static int executeAction(RuleAction* action);
#endif

// Trigger structure
typedef struct {
    MCP_TriggerType type;
    union {
        struct {
            char* sensor;
            MCP_ConditionOperator op_type; // Changed from 'operator' to 'op_type' to avoid C++ reserved word
            union {
                bool boolValue;
                int32_t intValue;
                float floatValue;
                char* stringValue;
            } value;
            uint32_t checkInterval;
            uint32_t lastCheckTime;
        } condition;
        
        struct {
            char* eventType;
            char* eventSource;
        } event;
        
        struct {
            uint32_t intervalMs;
            uint32_t lastTriggerTime;
        } schedule;
    } config;
} RuleTrigger;

// Action structure
typedef struct {
    MCP_ActionType type;
    union {
        struct {
            char* target;
            char* command;
            char* paramsJson;
        } actuator;
        
        struct {
            char* tool;
            char* paramsJson;
        } tool;
        
        struct {
            char* message;
            char* level;
            char* destination;
        } notification;
        
        struct {
            char* handlerName;
            char* paramsJson;
        } custom;
    } config;
} RuleAction;

// Rule structure
typedef struct {
    char* id;
    char* name;
    char* description;
    RuleTrigger* triggers;
    int triggerCount;
    RuleAction* actions;
    int actionCount;
    bool enabled;
    bool persistent;
} Rule;

// Internal state
static Rule** s_rules = NULL;
static int s_maxRules = 0;
static int s_ruleCount = 0;
static bool s_initialized = false;

static char s_ruleIdCounter[16] = "rule_1";

static void generateNextRuleId(void) {
    int idNumber = 1;
    sscanf(s_ruleIdCounter, "rule_%d", &idNumber);
    idNumber++;
    snprintf(s_ruleIdCounter, sizeof(s_ruleIdCounter), "rule_%d", idNumber);
}

static void freeRuleTrigger(RuleTrigger* trigger) {
    if (trigger == NULL) {
        return;
    }
    
    switch (trigger->type) {
        case MCP_TRIGGER_TYPE_CONDITION:
            free(trigger->config.condition.sensor);
            if (trigger->config.condition.op_type >= MCP_CONDITION_OP_CONTAINS && 
                trigger->config.condition.op_type <= MCP_CONDITION_OP_ENDS_WITH) {
                free(trigger->config.condition.value.stringValue);
            }
            break;
            
        case MCP_TRIGGER_TYPE_EVENT:
            free(trigger->config.event.eventType);
            free(trigger->config.event.eventSource);
            break;
            
        case MCP_TRIGGER_TYPE_SCHEDULE:
        case MCP_TRIGGER_TYPE_MANUAL:
            // No dynamic memory to free
            break;
    }
}

static void freeRuleAction(RuleAction* action) {
    if (action == NULL) {
        return;
    }
    
    switch (action->type) {
        case MCP_ACTION_TYPE_ACTUATOR:
            free(action->config.actuator.target);
            free(action->config.actuator.command);
            free(action->config.actuator.paramsJson);
            break;
            
        case MCP_ACTION_TYPE_TOOL:
            free(action->config.tool.tool);
            free(action->config.tool.paramsJson);
            break;
            
        case MCP_ACTION_TYPE_NOTIFICATION:
            free(action->config.notification.message);
            free(action->config.notification.level);
            free(action->config.notification.destination);
            break;
            
        case MCP_ACTION_TYPE_CUSTOM:
            free(action->config.custom.handlerName);
            free(action->config.custom.paramsJson);
            break;
    }
}

static void freeRule(Rule* rule) {
    if (rule == NULL) {
        return;
    }
    
    free(rule->id);
    free(rule->name);
    free(rule->description);
    
    for (int i = 0; i < rule->triggerCount; i++) {
        freeRuleTrigger(&rule->triggers[i]);
    }
    free(rule->triggers);
    
    for (int i = 0; i < rule->actionCount; i++) {
        freeRuleAction(&rule->actions[i]);
    }
    free(rule->actions);
    
    free(rule);
}

int MCP_AutomationInit(void) {
    if (s_initialized) {
        return -1;  // Already initialized
    }
    
    // Initialize with capacity for 10 rules
    s_maxRules = 10;
    s_ruleCount = 0;
    
    s_rules = (Rule**)calloc(s_maxRules, sizeof(Rule*));
    if (s_rules == NULL) {
        return -2;  // Memory allocation failed
    }
    
    s_initialized = true;
    return 0;
}

static Rule* findRule(const char* ruleId) {
    if (!s_initialized || ruleId == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < s_ruleCount; i++) {
        if (s_rules[i] != NULL && strcmp(s_rules[i]->id, ruleId) == 0) {
            return s_rules[i];
        }
    }
    
    return NULL;
}

// Forward declaration of parser functions
static bool parseTriggers(const char* json, Rule* rule);
static bool parseActions(const char* json, Rule* rule);

const char* MCP_AutomationCreateRule(const char* json, size_t length) {
    if (!s_initialized || json == NULL || length == 0) {
        return NULL;
    }
    
    // Extract rule info from JSON
    char* ruleJson = json_get_string_field(json, "rule");
    if (ruleJson == NULL) {
        return NULL;
    }
    
    // Allocate new rule
    Rule* rule = (Rule*)calloc(1, sizeof(Rule));
    if (rule == NULL) {
        free(ruleJson);
        return NULL;
    }
    
    // Parse rule ID if provided, otherwise generate new ID
    char* id = json_get_string_field(ruleJson, "id");
    if (id != NULL) {
        rule->id = id;
    } else {
        rule->id = strdup(s_ruleIdCounter);
        generateNextRuleId();
    }
    
    // Check if rule with this ID already exists
    if (findRule(rule->id) != NULL) {
        freeRule(rule);
        free(ruleJson);
        return NULL;
    }
    
    // Parse rule name
    rule->name = json_get_string_field(ruleJson, "name");
    if (rule->name == NULL) {
        rule->name = strdup("Unnamed Rule");
    }
    
    // Parse rule description
    rule->description = json_get_string_field(ruleJson, "description");
    if (rule->description == NULL) {
        rule->description = strdup("");
    }
    
    // Parse triggers
    if (!parseTriggers(ruleJson, rule)) {
        freeRule(rule);
        free(ruleJson);
        return NULL;
    }
    
    // Parse actions
    if (!parseActions(ruleJson, rule)) {
        freeRule(rule);
        free(ruleJson);
        return NULL;
    }
    
    // Parse enabled state
    rule->enabled = json_get_bool_field(ruleJson, "enabled", true);
    
    // Parse persistence
    rule->persistent = json_get_bool_field(ruleJson, "persistent", false);
    
    free(ruleJson);
    
    // Add rule to array
    if (s_ruleCount >= s_maxRules) {
        // Expand array
        int newMaxRules = s_maxRules * 2;
        Rule** newRules = (Rule**)realloc(s_rules, newMaxRules * sizeof(Rule*));
        if (newRules == NULL) {
            freeRule(rule);
            return NULL;
        }
        
        s_rules = newRules;
        s_maxRules = newMaxRules;
    }
    
    s_rules[s_ruleCount++] = rule;
    
    // Save to persistent storage if needed
    if (rule->persistent) {
        // TODO: Implement rule serialization and storage
    }
    
    return rule->id;
}

int MCP_AutomationSetRuleEnabled(const char* ruleId, bool enabled) {
    Rule* rule = findRule(ruleId);
    if (rule == NULL) {
        return -1;  // Rule not found
    }
    
    rule->enabled = enabled;
    
    // Update in persistent storage if needed
    if (rule->persistent) {
        // TODO: Implement rule serialization and storage
    }
    
    return 0;
}

int MCP_AutomationDeleteRule(const char* ruleId) {
    if (!s_initialized || ruleId == NULL) {
        return -1;
    }
    
    for (int i = 0; i < s_ruleCount; i++) {
        if (s_rules[i] != NULL && strcmp(s_rules[i]->id, ruleId) == 0) {
            // Delete from persistent storage if needed
            if (s_rules[i]->persistent) {
                // TODO: Implement rule deletion from storage
            }
            
            // Free rule
            freeRule(s_rules[i]);
            
            // Shift remaining rules
            for (int j = i; j < s_ruleCount - 1; j++) {
                s_rules[j] = s_rules[j + 1];
            }
            
            s_rules[s_ruleCount - 1] = NULL;
            s_ruleCount--;
            
            return 0;
        }
    }
    
    return -2;  // Rule not found
}

void MCP_AutomationProcess(uint32_t currentTimeMs) {
    (void)currentTimeMs; // Unused parameter, would be used for time-based triggers
    
    if (!s_initialized) {
        return;
    }
    
    for (int i = 0; i < s_ruleCount; i++) {
        if (s_rules[i] == NULL || !s_rules[i]->enabled) {
            continue;
        }
        
        // Check triggers
        bool triggered = MCP_AutomationCheckTriggers(s_rules[i]->id);
        
        if (triggered) {
            // Execute actions
            MCP_AutomationExecuteActions(s_rules[i]->id);
        }
    }
}

bool MCP_AutomationCheckTriggers(const char* ruleId) {
    Rule* rule = findRule(ruleId);
    if (rule == NULL) {
        return false;
    }
    
    // No triggers means never triggered
    if (rule->triggerCount == 0) {
        return false;
    }
    
    // Check each trigger
    for (int i = 0; i < rule->triggerCount; i++) {
        RuleTrigger* trigger = &rule->triggers[i];
        bool triggered = false;
        
        switch (trigger->type) {
            case MCP_TRIGGER_TYPE_CONDITION:
                // This is a simplified implementation
                // In a real implementation, you'd check sensor values
                triggered = false;  // Placeholder
                break;
                
            case MCP_TRIGGER_TYPE_EVENT:
                // This would be handled through the event system
                triggered = false;  // Placeholder
                break;
                
            case MCP_TRIGGER_TYPE_SCHEDULE: {
                uint32_t currentTimeMs = 0;  // TODO: Get current time
                uint32_t elapsed = currentTimeMs - trigger->config.schedule.lastTriggerTime;
                
                if (elapsed >= trigger->config.schedule.intervalMs) {
                    triggered = true;
                    trigger->config.schedule.lastTriggerTime = currentTimeMs;
                }
                break;
            }
                
            case MCP_TRIGGER_TYPE_MANUAL:
                // Manual triggers are not checked automatically
                triggered = false;
                break;
        }
        
        if (triggered) {
            return true;  // One trigger is enough
        }
    }
    
    return false;
}

int MCP_AutomationExecuteActions(const char* ruleId) {
    Rule* rule = findRule(ruleId);
    if (rule == NULL) {
        return -1;
    }
    
    // Execute each action
    for (int i = 0; i < rule->actionCount; i++) {
        RuleAction* action = &rule->actions[i];
        
        switch (action->type) {
            case MCP_ACTION_TYPE_ACTUATOR:
                // This is a simplified implementation
                // In a real implementation, you'd control actuators
                break;
                
            case MCP_ACTION_TYPE_TOOL:
                if (action->config.tool.tool != NULL) {
                    // Create tool JSON
                    char toolJson[512];
                    snprintf(toolJson, sizeof(toolJson), 
                             "{\"tool\":\"%s\",\"params\":%s}",
                             action->config.tool.tool,
                             action->config.tool.paramsJson ? action->config.tool.paramsJson : "{}");
                    
                    // Execute tool
                    MCP_ToolExecute(toolJson, strlen(toolJson));
                }
                break;
                
            case MCP_ACTION_TYPE_NOTIFICATION:
                // This is a simplified implementation
                // In a real implementation, you'd send notifications
                break;
                
            case MCP_ACTION_TYPE_CUSTOM:
                // This is a simplified implementation
                // In a real implementation, you'd call custom handlers
                break;
        }
    }
    
    return 0;
}

int MCP_AutomationTriggerRule(const char* ruleId) {
    Rule* rule = findRule(ruleId);
    if (rule == NULL) {
        return -1;
    }
    
    if (!rule->enabled) {
        return -2;  // Rule disabled
    }
    
    return MCP_AutomationExecuteActions(ruleId);
}

int MCP_AutomationExportRules(char* buffer, size_t bufferSize) {
    if (!s_initialized || buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // Start JSON array
    int offset = 0;
    offset += snprintf(buffer + offset, bufferSize - offset, "[");
    
    // Add rules
    for (int i = 0; i < s_ruleCount; i++) {
        Rule* rule = s_rules[i];
        if (rule == NULL) {
            continue;
        }
        
        // Add comma if not first rule
        if (i > 0) {
            offset += snprintf(buffer + offset, bufferSize - offset, ",");
        }
        
        // Add rule info (simplified)
        offset += snprintf(buffer + offset, bufferSize - offset, 
                          "{\"id\":\"%s\",\"name\":\"%s\",\"enabled\":%s,\"persistent\":%s}",
                          rule->id,
                          rule->name,
                          rule->enabled ? "true" : "false",
                          rule->persistent ? "true" : "false");
        
        // Check if we're about to overflow
        if ((size_t)offset >= bufferSize - 2) {
            return -2;  // Buffer too small
        }
    }
    
    // End JSON array
    offset += snprintf(buffer + offset, bufferSize - offset, "]");
    
    return offset;
}

int MCP_AutomationImportRules(const char* json, size_t length) {
    // Mark parameters as unused to avoid compiler warnings
    (void)json;
    (void)length;
    
    // Not implemented for simplicity
    // In a real implementation, you'd parse the JSON and create rules
    return -100;  // Not implemented
}

// Parser functions implementation (simplified)
static bool parseTriggers(const char* json, Rule* rule) {
    // Get triggers array
    void* triggersJson = json_get_array_field(json, "triggers");
    if (triggersJson == NULL) {
        return false;
    }
    
    // Get trigger count
    int triggerCount = 1;  // Simplified - in reality would get array length
    
    // Allocate triggers array
    rule->triggers = (RuleTrigger*)calloc(triggerCount, sizeof(RuleTrigger));
    if (rule->triggers == NULL) {
        return false;
    }
    
    rule->triggerCount = triggerCount;
    
    // Parse triggers (simplified - in reality would iterate through array)
    RuleTrigger* trigger = &rule->triggers[0];
    
    // Set default values
    trigger->type = MCP_TRIGGER_TYPE_MANUAL;
    
    return true;
}

static bool parseActions(const char* json, Rule* rule) {
    // Get actions array
    void* actionsJson = json_get_array_field(json, "actions");
    if (actionsJson == NULL) {
        return false;
    }
    
    // Get action count
    int actionCount = 1;  // Simplified - in reality would get array length
    
    // Allocate actions array
    rule->actions = (RuleAction*)calloc(actionCount, sizeof(RuleAction));
    if (rule->actions == NULL) {
        return false;
    }
    
    rule->actionCount = actionCount;
    
    // Parse actions (simplified - in reality would iterate through array)
    RuleAction* action = &rule->actions[0];
    
    // Set default values
    action->type = MCP_ACTION_TYPE_NOTIFICATION;
    action->config.notification.message = strdup("Rule triggered");
    action->config.notification.level = strdup("info");
    action->config.notification.destination = strdup("log");
    
    return true;
}