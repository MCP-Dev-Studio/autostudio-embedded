#ifndef PIN_MAPPING_H
#define PIN_MAPPING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Pin definition structure
 */
typedef struct {
    const char* name;           // Pin name (e.g. "D0", "A0", "LED1", "BUTTON1")
    uint32_t pin;               // Platform-specific pin number
    bool isDigital;             // Is a digital pin
    bool isAnalog;              // Is an analog pin
    bool isPWM;                 // Is a PWM-capable pin
    bool isInterrupt;           // Is an interrupt-capable pin
    uint32_t interfaceMask;     // Interfaces supported (bit mask)
    const char* alias;          // Alternative name/alias
} PinDefinition;

/**
 * @brief Interface types bit flags
 */
enum PinInterfaceFlags {
    PIN_INTERFACE_GPIO = 0x01,
    PIN_INTERFACE_ADC = 0x02,
    PIN_INTERFACE_DAC = 0x04,
    PIN_INTERFACE_PWM = 0x08,
    PIN_INTERFACE_I2C = 0x10,
    PIN_INTERFACE_SPI = 0x20,
    PIN_INTERFACE_UART = 0x40,
    PIN_INTERFACE_CAN = 0x80
};

/**
 * @brief Initialize pin mapping
 * 
 * @return bool True on success, false on failure
 */
bool pin_mapping_init(void);

/**
 * @brief Find pin definition by name
 * 
 * @param name Pin name
 * @return const PinDefinition* Pin definition or NULL if not found
 */
const PinDefinition* pin_mapping_find_by_name(const char* name);

/**
 * @brief Find pin definition by number
 * 
 * @param pin Pin number
 * @return const PinDefinition* Pin definition or NULL if not found
 */
const PinDefinition* pin_mapping_find_by_pin(uint32_t pin);

/**
 * @brief Get pin number from name
 * 
 * @param name Pin name
 * @return uint32_t Pin number or UINT32_MAX if not found
 */
uint32_t pin_mapping_get_pin(const char* name);

/**
 * @brief Get pin name from number
 * 
 * @param pin Pin number
 * @param buffer Buffer to store pin name
 * @param bufferSize Size of buffer
 * @return int Length of pin name or negative error code
 */
int pin_mapping_get_name(uint32_t pin, char* buffer, size_t bufferSize);

/**
 * @brief Check if pin supports interface
 * 
 * @param name Pin name
 * @param interfaceFlag Interface flag
 * @return bool True if supported, false otherwise
 */
bool pin_mapping_supports_interface(const char* name, uint32_t interfaceFlag);

/**
 * @brief Get list of pins supporting interface
 * 
 * @param interfaceFlag Interface flag
 * @param pins Array to store pin definitions
 * @param maxPins Maximum number of pins to return
 * @return int Number of pins found or negative error code
 */
int pin_mapping_get_pins_for_interface(uint32_t interfaceFlag, 
                                     const PinDefinition** pins, uint32_t maxPins);

/**
 * @brief Get alternate pin function
 * 
 * @param name Pin name
 * @param interfaceFlag Interface flag
 * @return uint32_t Alternate function number or UINT32_MAX if not found
 */
uint32_t pin_mapping_get_alternate_function(const char* name, uint32_t interfaceFlag);

/**
 * @brief Get list of pin definitions as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int pin_mapping_get_json(char* buffer, size_t bufferSize);

#endif /* PIN_MAPPING_H */