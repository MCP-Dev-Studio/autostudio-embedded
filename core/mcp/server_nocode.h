#ifndef MCP_SERVER_NOCODE_H
#define MCP_SERVER_NOCODE_H

#include "server.h"
#include "content.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief No-code configuration structure
 */
typedef struct {
    bool enableConfig;          // Enable configuration via no-code interface
    bool enableSensors;         // Enable sensors via no-code interface
    bool enableActuators;       // Enable actuators via no-code interface
    bool enableAutomation;      // Enable automation via no-code interface
    bool enableTools;           // Enable tools via no-code interface
    bool enableAPI;             // Enable API access via no-code interface
    bool enableDiscovery;       // Enable hardware discovery
    bool enableDebug;           // Enable debug features
} MCP_NoCodeConfig;

/**
 * @brief No-code sensor configuration
 */
typedef struct {
    char* id;                   // Sensor ID
    char* type;                 // Sensor type
    char* interface;            // Interface type
    char* pin;                  // Pin identifier
    char* configJson;           // Additional configuration (JSON)
} MCP_NoCodeSensorConfig;

/**
 * @brief No-code actuator configuration
 */
typedef struct {
    char* id;                   // Actuator ID
    char* type;                 // Actuator type
    char* interface;            // Interface type
    char* pin;                  // Pin identifier
    char* initialState;         // Initial state (JSON)
    char* configJson;           // Additional configuration (JSON)
} MCP_NoCodeActuatorConfig;

/**
 * @brief No-code rule condition
 */
typedef struct {
    char* sensor;               // Sensor ID
    char* operator;             // Operator (e.g., "less_than", "greater_than")
    char* valueJson;            // Value (JSON format)
    uint32_t checkInterval;     // Check interval in milliseconds
} MCP_NoCodeRuleCondition;

/**
 * @brief No-code rule action
 */
typedef struct {
    char* type;                 // Action type (e.g., "actuator", "notification")
    char* target;               // Target ID
    char* command;              // Command
    char* paramsJson;           // Parameters (JSON format)
} MCP_NoCodeRuleAction;

/**
 * @brief No-code automation rule
 */
typedef struct {
    char* id;                   // Rule ID
    char* name;                 // Rule name
    char* description;          // Rule description
    MCP_NoCodeRuleCondition* conditions; // Rule conditions
    uint16_t conditionCount;    // Number of conditions
    MCP_NoCodeRuleAction* actions; // Rule actions
    uint16_t actionCount;       // Number of actions
    bool enabled;               // Rule enabled state
    bool persistent;            // Save rule to persistent storage
} MCP_NoCodeRule;

/**
 * @brief Initialize the no-code extension
 * 
 * @param config No-code configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_NoCodeInit(const MCP_NoCodeConfig* config);

/**
 * @brief Handle system initialization request
 * 
 * @param requestContent Request content (JSON)
 * @param responseContent Pointer to store response content
 * @return int 0 on success, negative error code on failure
 */
int MCP_NoCodeHandleSystemInit(const MCP_Content* requestContent, 
                            MCP_Content** responseContent);

/**
 * @brief Configure sensors based on JSON configuration
 * 
 * @param configJson JSON configuration string
 * @param length JSON string length
 * @return int Number of sensors configured or negative error code
 */
int MCP_NoCodeConfigureSensors(const char* configJson, size_t length);

/**
 * @brief Configure actuators based on JSON configuration
 * 
 * @param configJson JSON configuration string
 * @param length JSON string length
 * @return int Number of actuators configured or negative error code
 */
int MCP_NoCodeConfigureActuators(const char* configJson, size_t length);

/**
 * @brief Create automation rule from JSON
 * 
 * @param ruleJson JSON rule definition
 * @param length JSON string length
 * @return const char* Rule ID or NULL on failure (caller must NOT free)
 */
const char* MCP_NoCodeCreateRule(const char* ruleJson, size_t length);

/**
 * @brief Delete automation rule
 * 
 * @param ruleId Rule ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_NoCodeDeleteRule(const char* ruleId);

/**
 * @brief Generate sensor list as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_NoCodeGetSensorList(char* buffer, size_t bufferSize);

/**
 * @brief Generate actuator list as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_NoCodeGetActuatorList(char* buffer, size_t bufferSize);

/**
 * @brief Generate rule list as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_NoCodeGetRuleList(char* buffer, size_t bufferSize);

/**
 * @brief Register no-code tools
 * 
 * @return int Number of tools registered or negative error code
 */
int MCP_NoCodeRegisterTools(void);

#endif /* MCP_SERVER_NOCODE_H */