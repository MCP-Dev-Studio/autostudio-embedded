#ifndef MCP_ACTUATOR_MANAGER_H
#define MCP_ACTUATOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Actuator types
 */
typedef enum {
    MCP_ACTUATOR_TYPE_RELAY,
    MCP_ACTUATOR_TYPE_LED,
    MCP_ACTUATOR_TYPE_SERVO,
    MCP_ACTUATOR_TYPE_STEPPER,
    MCP_ACTUATOR_TYPE_DC_MOTOR,
    MCP_ACTUATOR_TYPE_LCD,
    MCP_ACTUATOR_TYPE_BUZZER,
    MCP_ACTUATOR_TYPE_CUSTOM
} MCP_ActuatorType;

/**
 * @brief Actuator interface types
 */
typedef enum {
    MCP_ACTUATOR_INTERFACE_DIGITAL,
    MCP_ACTUATOR_INTERFACE_ANALOG,
    MCP_ACTUATOR_INTERFACE_PWM,
    MCP_ACTUATOR_INTERFACE_I2C,
    MCP_ACTUATOR_INTERFACE_SPI,
    MCP_ACTUATOR_INTERFACE_UART,
    MCP_ACTUATOR_INTERFACE_CUSTOM
} MCP_ActuatorInterface;

/**
 * @brief Actuator state type enumeration
 */
typedef enum {
    MCP_ACTUATOR_STATE_TYPE_BOOL,
    MCP_ACTUATOR_STATE_TYPE_INT,
    MCP_ACTUATOR_STATE_TYPE_FLOAT,
    MCP_ACTUATOR_STATE_TYPE_STRING,
    MCP_ACTUATOR_STATE_TYPE_OBJECT
} MCP_ActuatorStateType;

/**
 * @brief Actuator state value union
 */
typedef union {
    bool boolValue;
    int32_t intValue;
    float floatValue;
    char* stringValue;
    void* objectValue;
} MCP_ActuatorStateValue;

/**
 * @brief Actuator state structure
 */
typedef struct {
    MCP_ActuatorStateType type;
    MCP_ActuatorStateValue value;
    uint32_t timestamp;
} MCP_ActuatorState;

/**
 * @brief Actuator configuration structure
 */
typedef struct {
    char* id;                       // Actuator ID
    char* name;                     // Actuator name
    MCP_ActuatorType type;          // Actuator type
    MCP_ActuatorInterface iface;    // Actuator interface
    char* pin;                      // Pin name
    char* driverId;                 // Driver ID
    char* configJson;               // Actuator-specific configuration
    MCP_ActuatorState initialState; // Initial state
} MCP_ActuatorConfig;

/**
 * @brief Actuator status structure
 */
typedef struct {
    char* id;                       // Actuator ID
    bool connected;                 // Is actuator connected
    bool enabled;                   // Is actuator enabled
    uint32_t lastUpdateTime;        // Last state update timestamp
    uint32_t updateCount;           // Number of state updates
    MCP_ActuatorState currentState; // Current actuator state
} MCP_ActuatorStatus;

/**
 * @brief Initialize the actuator manager
 * 
 * @param maxActuators Maximum number of actuators to manage
 * @return int 0 on success, negative error code on failure
 */
int MCP_ActuatorManagerInit(uint16_t maxActuators);

/**
 * @brief Register an actuator
 * 
 * @param config Actuator configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_ActuatorRegister(const MCP_ActuatorConfig* config);

/**
 * @brief Unregister an actuator
 * 
 * @param id Actuator ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_ActuatorUnregister(const char* id);

/**
 * @brief Find an actuator by ID
 * 
 * @param id Actuator ID
 * @return MCP_ActuatorConfig* Actuator configuration or NULL if not found
 */
const MCP_ActuatorConfig* MCP_ActuatorFind(const char* id);

/**
 * @brief Get list of actuators by type
 * 
 * @param type Actuator type to filter (or -1 for all)
 * @param actuators Array to store actuator pointers
 * @param maxActuators Maximum number of actuators to return
 * @return int Number of actuators found or negative error code
 */
int MCP_ActuatorGetByType(int type, const MCP_ActuatorConfig** actuators, uint16_t maxActuators);

/**
 * @brief Enable an actuator
 * 
 * @param id Actuator ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_ActuatorEnable(const char* id);

/**
 * @brief Disable an actuator
 * 
 * @param id Actuator ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_ActuatorDisable(const char* id);

/**
 * @brief Set actuator state
 * 
 * @param id Actuator ID
 * @param state New state
 * @return int 0 on success, negative error code on failure
 */
int MCP_ActuatorSetState(const char* id, const MCP_ActuatorState* state);

/**
 * @brief Get actuator state
 * 
 * @param id Actuator ID
 * @param state Pointer to store actuator state
 * @return int 0 on success, negative error code on failure
 */
int MCP_ActuatorGetState(const char* id, MCP_ActuatorState* state);

/**
 * @brief Get actuator status
 * 
 * @param id Actuator ID
 * @param status Pointer to store actuator status
 * @return int 0 on success, negative error code on failure
 */
int MCP_ActuatorGetStatus(const char* id, MCP_ActuatorStatus* status);

/**
 * @brief Send command to actuator
 * 
 * @param id Actuator ID
 * @param command Command string
 * @param params Command parameters (JSON format)
 * @return int 0 on success, negative error code on failure
 */
int MCP_ActuatorSendCommand(const char* id, const char* command, const char* params);

/**
 * @brief Create actuator state from different types
 */
MCP_ActuatorState MCP_ActuatorCreateBoolState(bool value);
MCP_ActuatorState MCP_ActuatorCreateIntState(int32_t value);
MCP_ActuatorState MCP_ActuatorCreateFloatState(float value);
MCP_ActuatorState MCP_ActuatorCreateStringState(const char* value);

/**
 * @brief Free actuator state resources if needed
 * 
 * @param state Actuator state to free
 */
void MCP_ActuatorFreeState(MCP_ActuatorState* state);

#endif /* MCP_ACTUATOR_MANAGER_H */