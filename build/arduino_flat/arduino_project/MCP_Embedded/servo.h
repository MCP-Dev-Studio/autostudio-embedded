#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Servo configuration
 */
typedef struct {
    uint8_t pin;                // PWM pin number
    uint8_t channel;            // PWM channel (platform-specific)
    uint16_t minPulseWidth;     // Minimum pulse width in microseconds (usually 500-1000)
    uint16_t maxPulseWidth;     // Maximum pulse width in microseconds (usually 2000-2500)
    uint16_t frequency;         // PWM frequency in Hz (usually 50Hz)
    uint8_t minAngle;           // Minimum angle in degrees (usually 0)
    uint8_t maxAngle;           // Maximum angle in degrees (usually 180)
    uint8_t initialAngle;       // Initial angle in degrees
    bool reverse;               // Reverse servo direction
    bool continuousRotation;    // Continuous rotation servo
    uint32_t moveTime;          // Default move time in milliseconds
} ServoConfig;

/**
 * @brief Motion profile type
 */
typedef enum {
    SERVO_MOTION_INSTANT,       // Instant movement (no profile)
    SERVO_MOTION_LINEAR,        // Linear profile
    SERVO_MOTION_SMOOTH,        // Smooth acceleration/deceleration
    SERVO_MOTION_EASE_IN,       // Ease in (slow start, fast end)
    SERVO_MOTION_EASE_OUT,      // Ease out (fast start, slow end)
    SERVO_MOTION_EASE_IN_OUT    // Ease in/out (slow start, fast middle, slow end)
} ServoMotionType;

/**
 * @brief Servo initialization
 * 
 * @param config Servo configuration
 * @return int 0 on success, negative error code on failure
 */
int Servo_Init(const ServoConfig* config);

/**
 * @brief Servo deinitialization
 * 
 * @return int 0 on success, negative error code on failure
 */
int Servo_Deinit(void);

/**
 * @brief Set servo angle
 * 
 * @param angle Angle in degrees
 * @return int 0 on success, negative error code on failure
 */
int Servo_SetAngle(uint8_t angle);

/**
 * @brief Get current servo angle
 * 
 * @return uint8_t Current angle in degrees
 */
uint8_t Servo_GetAngle(void);

/**
 * @brief Set servo pulse width directly
 * 
 * @param pulseWidth Pulse width in microseconds
 * @return int 0 on success, negative error code on failure
 */
int Servo_SetPulseWidth(uint16_t pulseWidth);

/**
 * @brief Get current pulse width
 * 
 * @return uint16_t Current pulse width in microseconds
 */
uint16_t Servo_GetPulseWidth(void);

/**
 * @brief Set continuous rotation speed
 * 
 * @param speed Speed (-100 to +100, 0 for stop)
 * @return int 0 on success, negative error code on failure
 */
int Servo_SetSpeed(int8_t speed);

/**
 * @brief Move servo to angle with specified motion profile
 * 
 * @param angle Target angle
 * @param duration Movement duration in milliseconds
 * @param motionType Motion profile type
 * @return int 0 on success, negative error code on failure
 */
int Servo_MoveTo(uint8_t angle, uint32_t duration, ServoMotionType motionType);

/**
 * @brief Check if servo is moving
 * 
 * @return bool True if servo is moving, false otherwise
 */
bool Servo_IsMoving(void);

/**
 * @brief Stop servo movement
 * 
 * @return int 0 on success, negative error code on failure
 */
int Servo_Stop(void);

/**
 * @brief Process servo motion (call in main loop)
 * 
 * @return int 0 on success, negative error code on failure
 */
int Servo_Process(void);

/**
 * @brief Detach servo (disable PWM output)
 * 
 * @return int 0 on success, negative error code on failure
 */
int Servo_Detach(void);

/**
 * @brief Attach servo (enable PWM output)
 * 
 * @return int 0 on success, negative error code on failure
 */
int Servo_Attach(void);

/**
 * @brief Check if servo is attached
 * 
 * @return bool True if attached, false otherwise
 */
bool Servo_IsAttached(void);

/**
 * @brief Set servo trim adjustment
 * 
 * @param trim Trim value in microseconds (-100 to +100)
 * @return int 0 on success, negative error code on failure
 */
int Servo_SetTrim(int16_t trim);

/**
 * @brief Get servo trim adjustment
 * 
 * @return int16_t Trim value in microseconds
 */
int16_t Servo_GetTrim(void);

#endif /* SERVO_H */