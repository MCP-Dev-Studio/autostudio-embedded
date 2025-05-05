#ifndef PIR_H
#define PIR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief PIR sensor configuration
 */
typedef struct {
    uint8_t pin;                // Sensor pin number
    bool activeHigh;            // Active high (true) or active low (false)
    uint32_t stabilizationTime; // Warm-up time in milliseconds
    uint32_t holdTime;          // Minimum motion hold time in milliseconds
    bool useInterrupt;          // Use interrupt for detection
} PIRConfig;

/**
 * @brief Motion detection callback function type
 */
typedef void (*PIRMotionCallback)(bool motionDetected, void* context);

/**
 * @brief PIR sensor initialization
 * 
 * @param config PIR sensor configuration
 * @return int 0 on success, negative error code on failure
 */
int PIR_Init(const PIRConfig* config);

/**
 * @brief PIR sensor deinitialization
 * 
 * @return int 0 on success, negative error code on failure
 */
int PIR_Deinit(void);

/**
 * @brief Check for motion
 * 
 * @return bool True if motion detected, false otherwise
 */
bool PIR_IsMotionDetected(void);

/**
 * @brief Set motion detection callback
 * 
 * @param callback Callback function pointer
 * @param context User context pointer
 * @return int 0 on success, negative error code on failure
 */
int PIR_SetCallback(PIRMotionCallback callback, void* context);

/**
 * @brief Reset PIR sensor
 * 
 * @return int 0 on success, negative error code on failure
 */
int PIR_Reset(void);

/**
 * @brief Get last motion detection time
 * 
 * @return uint32_t Last motion detection timestamp
 */
uint32_t PIR_GetLastMotionTime(void);

/**
 * @brief Get time since last motion detection
 * 
 * @return uint32_t Time since last motion detection in milliseconds
 */
uint32_t PIR_GetTimeSinceLastMotion(void);

/**
 * @brief Process PIR sensor (call in main loop when not using interrupts)
 * 
 * @return int 0 on success, negative error code on failure
 */
int PIR_Process(void);

/**
 * @brief Enable PIR sensor
 * 
 * @return int 0 on success, negative error code on failure
 */
int PIR_Enable(void);

/**
 * @brief Disable PIR sensor
 * 
 * @return int 0 on success, negative error code on failure
 */
int PIR_Disable(void);

/**
 * @brief Check if PIR sensor is stabilized
 * 
 * @return bool True if stabilized, false otherwise
 */
bool PIR_IsStabilized(void);

/**
 * @brief Configure PIR sensor sensitivity (if hardware supports it)
 * 
 * @param sensitivity Sensitivity level (0-100)
 * @return int 0 on success, negative error code on failure
 */
int PIR_SetSensitivity(uint8_t sensitivity);

/**
 * @brief Get current sensitivity level
 * 
 * @return int Sensitivity level (0-100) or negative error code
 */
int PIR_GetSensitivity(void);

#endif /* PIR_H */