#ifndef MCP_GPIO_CONTROLLER_H
#define MCP_GPIO_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief GPIO pin modes
 */
typedef enum {
    MCP_GPIO_MODE_INPUT,          // Input
    MCP_GPIO_MODE_INPUT_PULLUP,   // Input with pull-up
    MCP_GPIO_MODE_INPUT_PULLDOWN, // Input with pull-down
    MCP_GPIO_MODE_OUTPUT,         // Output
    MCP_GPIO_MODE_OPEN_DRAIN,     // Open-drain output
    MCP_GPIO_MODE_ANALOG,         // Analog input/output
    MCP_GPIO_MODE_ALTERNATE       // Alternate function
} MCP_GPIOMode;

/**
 * @brief GPIO pin interrupt modes
 */
typedef enum {
    MCP_GPIO_INT_NONE,       // No interrupt
    MCP_GPIO_INT_RISING,     // Rising edge
    MCP_GPIO_INT_FALLING,    // Falling edge
    MCP_GPIO_INT_CHANGE,     // Any edge change
    MCP_GPIO_INT_LOW,        // Low level
    MCP_GPIO_INT_HIGH        // High level
} MCP_GPIOInterruptMode;

/**
 * @brief GPIO pin configuration
 */
typedef struct {
    MCP_GPIOMode mode;                  // Pin mode
    MCP_GPIOInterruptMode interruptMode; // Interrupt mode
    uint8_t alternateFunction;          // Alternate function number (if applicable)
    bool initialState;                  // Initial state for output pins
} MCP_GPIOConfig;

/**
 * @brief GPIO pin state
 */
typedef enum {
    MCP_GPIO_LOW = 0,
    MCP_GPIO_HIGH = 1
} MCP_GPIOState;

/**
 * @brief GPIO interrupt callback function type
 */
typedef void (*MCP_GPIOInterruptCallback)(uint32_t pin, void* context);

/**
 * @brief Initialize GPIO controller
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIOInit(void);

/**
 * @brief Deinitialize GPIO controller
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIODeinit(void);

/**
 * @brief Configure a GPIO pin
 * 
 * @param pin Pin number
 * @param config Pin configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIOConfigure(uint32_t pin, const MCP_GPIOConfig* config);

/**
 * @brief Set GPIO pin state
 * 
 * @param pin Pin number
 * @param state Pin state (MCP_GPIO_LOW or MCP_GPIO_HIGH)
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIOWrite(uint32_t pin, MCP_GPIOState state);

/**
 * @brief Read GPIO pin state
 * 
 * @param pin Pin number
 * @param state Pointer to store pin state
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIORead(uint32_t pin, MCP_GPIOState* state);

/**
 * @brief Toggle GPIO pin state
 * 
 * @param pin Pin number
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIOToggle(uint32_t pin);

/**
 * @brief Set GPIO interrupt callback
 * 
 * @param pin Pin number
 * @param callback Interrupt callback function
 * @param context User context pointer passed to callback
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIOSetInterruptCallback(uint32_t pin, MCP_GPIOInterruptCallback callback, void* context);

/**
 * @brief Enable GPIO interrupt
 * 
 * @param pin Pin number
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIOEnableInterrupt(uint32_t pin);

/**
 * @brief Disable GPIO interrupt
 * 
 * @param pin Pin number
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIODisableInterrupt(uint32_t pin);

/**
 * @brief Map a pin identifier string to pin number
 * 
 * @param pinName Pin name string (e.g., "PA5", "D13", "GPIO21")
 * @return uint32_t Pin number or -1 if not found
 */
uint32_t MCP_GPIOGetPinNumber(const char* pinName);

/**
 * @brief Get pin identifier string from pin number
 * 
 * @param pin Pin number
 * @param buffer Buffer to store pin name
 * @param bufferSize Size of buffer
 * @return int Length of pin name or negative error code
 */
int MCP_GPIOGetPinName(uint32_t pin, char* buffer, size_t bufferSize);

/**
 * @brief Get range of valid pin numbers
 * 
 * @param minPin Pointer to store minimum pin number
 * @param maxPin Pointer to store maximum pin number
 * @return int 0 on success, negative error code on failure
 */
int MCP_GPIOGetPinRange(uint32_t* minPin, uint32_t* maxPin);

#endif /* MCP_GPIO_CONTROLLER_H */