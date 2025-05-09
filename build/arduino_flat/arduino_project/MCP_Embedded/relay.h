#ifndef RELAY_H
#define RELAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Relay type enumeration
 */
typedef enum {
    RELAY_TYPE_MECHANICAL,   // Mechanical relay
    RELAY_TYPE_SOLID_STATE,  // Solid state relay
    RELAY_TYPE_LATCHING      // Latching relay
} RelayType;

/**
 * @brief Relay configuration
 */
typedef struct {
    uint8_t pin;              // Control pin number
    uint8_t resetPin;         // Reset pin number (for latching relays)
    RelayType type;           // Relay type
    bool activeHigh;          // Active high (true) or active low (false)
    bool initialState;        // Initial state (on/off)
    uint32_t minPulseTime;    // Minimum pulse time in milliseconds (for latching relays)
    uint32_t debounceTime;    // Debounce time in milliseconds
    uint32_t maxOnTime;       // Maximum on time in milliseconds (0 for unlimited)
} RelayConfig;

/**
 * @brief State change callback function type
 */
typedef void (*RelayStateCallback)(bool state, void* context);

/**
 * @brief Relay initialization
 * 
 * @param config Relay configuration
 * @return int 0 on success, negative error code on failure
 */
int Relay_Init(const RelayConfig* config);

/**
 * @brief Relay deinitialization
 * 
 * @return int 0 on success, negative error code on failure
 */
int Relay_Deinit(void);

/**
 * @brief Turn relay on
 * 
 * @return int 0 on success, negative error code on failure
 */
int Relay_On(void);

/**
 * @brief Turn relay off
 * 
 * @return int 0 on success, negative error code on failure
 */
int Relay_Off(void);

/**
 * @brief Toggle relay state
 * 
 * @return int 0 on success, negative error code on failure
 */
int Relay_Toggle(void);

/**
 * @brief Set relay state
 * 
 * @param state Desired state (true = on, false = off)
 * @return int 0 on success, negative error code on failure
 */
int Relay_SetState(bool state);

/**
 * @brief Get relay state
 * 
 * @return bool True if relay is on, false if off
 */
bool Relay_GetState(void);

/**
 * @brief Pulse relay for specified time
 * 
 * @param state State to pulse to (true = on, false = off)
 * @param durationMs Pulse duration in milliseconds
 * @return int 0 on success, negative error code on failure
 */
int Relay_Pulse(bool state, uint32_t durationMs);

/**
 * @brief Set state change callback
 * 
 * @param callback Callback function pointer
 * @param context User context pointer
 * @return int 0 on success, negative error code on failure
 */
int Relay_SetCallback(RelayStateCallback callback, void* context);

/**
 * @brief Get time since last state change
 * 
 * @return uint32_t Time since last state change in milliseconds
 */
uint32_t Relay_GetTimeSinceLastChange(void);

/**
 * @brief Get time in current state
 * 
 * @return uint32_t Time in current state in milliseconds
 */
uint32_t Relay_GetTimeInCurrentState(void);

/**
 * @brief Process relay (call in main loop)
 * 
 * @return int 0 on success, negative error code on failure
 */
int Relay_Process(void);

/**
 * @brief Get cycle count (number of on/off cycles)
 * 
 * @return uint32_t Cycle count
 */
uint32_t Relay_GetCycleCount(void);

/**
 * @brief Reset cycle count
 * 
 * @return int 0 on success, negative error code on failure
 */
int Relay_ResetCycleCount(void);

#endif /* RELAY_H */