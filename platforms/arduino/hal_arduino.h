#ifndef HAL_ARDUINO_H
#define HAL_ARDUINO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Arduino pin modes
 */
enum HAL_ArduinoPinMode {
    HAL_ARDUINO_PIN_MODE_INPUT = 0,
    HAL_ARDUINO_PIN_MODE_OUTPUT = 1,
    HAL_ARDUINO_PIN_MODE_INPUT_PULLUP = 2,
    HAL_ARDUINO_PIN_MODE_INPUT_PULLDOWN = 3,
    HAL_ARDUINO_PIN_MODE_OUTPUT_OPEN_DRAIN = 4,
    HAL_ARDUINO_PIN_MODE_ANALOG = 5
};

/**
 * @brief Initialize the Arduino HAL
 * 
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoInit(void);

/**
 * @brief Deinitialize the Arduino HAL
 * 
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoDeinit(void);

/**
 * @brief Configure a digital pin
 * 
 * @param pin Pin number
 * @param mode Pin mode
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoPinMode(uint8_t pin, uint8_t mode);

/**
 * @brief Write to a digital pin
 * 
 * @param pin Pin number
 * @param value Pin value (0 = low, 1 = high)
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoDigitalWrite(uint8_t pin, uint8_t value);

/**
 * @brief Read from a digital pin
 * 
 * @param pin Pin number
 * @return int Pin value (0 or 1) or negative error code
 */
int HAL_ArduinoDigitalRead(uint8_t pin);

/**
 * @brief Write analog value (PWM) to a pin
 * 
 * @param pin Pin number
 * @param value Analog value (0-255)
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoAnalogWrite(uint8_t pin, uint8_t value);

/**
 * @brief Read analog value from a pin
 * 
 * @param pin Pin number
 * @return int Analog value (0-1023) or negative error code
 */
int HAL_ArduinoAnalogRead(uint8_t pin);

/**
 * @brief Attach interrupt to a pin
 * 
 * @param pin Pin number
 * @param mode Interrupt mode
 * @param callback Interrupt callback function
 * @param arg Callback argument
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoAttachInterrupt(uint8_t pin, uint8_t mode, void (*callback)(void*), void* arg);

/**
 * @brief Detach interrupt from a pin
 * 
 * @param pin Pin number
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoDetachInterrupt(uint8_t pin);

/**
 * @brief Configure a software serial port
 * 
 * @param instance Serial instance number
 * @param rxPin RX pin number
 * @param txPin TX pin number
 * @param baudRate Baud rate
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoSerialConfig(uint8_t instance, uint8_t rxPin, uint8_t txPin, uint32_t baudRate);

/**
 * @brief Read from a serial port
 * 
 * @param instance Serial instance number
 * @param buffer Buffer to store read data
 * @param size Maximum number of bytes to read
 * @param timeout Read timeout in milliseconds
 * @return int Number of bytes read or negative error code
 */
int HAL_ArduinoSerialRead(uint8_t instance, uint8_t* buffer, size_t size, uint32_t timeout);

/**
 * @brief Write to a serial port
 * 
 * @param instance Serial instance number
 * @param data Data to write
 * @param size Number of bytes to write
 * @return int Number of bytes written or negative error code
 */
int HAL_ArduinoSerialWrite(uint8_t instance, const uint8_t* data, size_t size);

/**
 * @brief Configure an I2C interface
 * 
 * @param instance I2C instance number
 * @param sdaPin SDA pin number
 * @param sclPin SCL pin number
 * @param frequency Clock frequency in Hz
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoI2CConfig(uint8_t instance, uint8_t sdaPin, uint8_t sclPin, uint32_t frequency);

/**
 * @brief Write to an I2C device
 * 
 * @param instance I2C instance number
 * @param address Device address
 * @param data Data to write
 * @param size Number of bytes to write
 * @param stop Generate stop condition
 * @return int Number of bytes written or negative error code
 */
int HAL_ArduinoI2CWrite(uint8_t instance, uint8_t address, const uint8_t* data, size_t size, bool stop);

/**
 * @brief Read from an I2C device
 * 
 * @param instance I2C instance number
 * @param address Device address
 * @param data Buffer to store read data
 * @param size Number of bytes to read
 * @return int Number of bytes read or negative error code
 */
int HAL_ArduinoI2CRead(uint8_t instance, uint8_t address, uint8_t* data, size_t size);

/**
 * @brief Configure an SPI interface
 * 
 * @param instance SPI instance number
 * @param mosiPin MOSI pin number
 * @param misoPin MISO pin number
 * @param sckPin SCK pin number
 * @param frequency Clock frequency in Hz
 * @param mode SPI mode (0-3)
 * @return int 0 on success, negative error code on failure
 */
int HAL_ArduinoSPIConfig(uint8_t instance, uint8_t mosiPin, uint8_t misoPin, uint8_t sckPin, 
                         uint32_t frequency, uint8_t mode);

/**
 * @brief Transfer data over SPI
 * 
 * @param instance SPI instance number
 * @param txData Data to transmit
 * @param rxData Buffer to store received data
 * @param size Number of bytes to transfer
 * @return int Number of bytes transferred or negative error code
 */
int HAL_ArduinoSPITransfer(uint8_t instance, const uint8_t* txData, uint8_t* rxData, size_t size);

/**
 * @brief Read from EEPROM
 * 
 * @param address EEPROM address
 * @param data Buffer to store read data
 * @param size Number of bytes to read
 * @return int Number of bytes read or negative error code
 */
int HAL_ArduinoEEPROMRead(uint16_t address, uint8_t* data, size_t size);

/**
 * @brief Write to EEPROM
 * 
 * @param address EEPROM address
 * @param data Data to write
 * @param size Number of bytes to write
 * @return int Number of bytes written or negative error code
 */
int HAL_ArduinoEEPROMWrite(uint16_t address, const uint8_t* data, size_t size);

/**
 * @brief Get system time in milliseconds
 * 
 * @return uint32_t System time in milliseconds
 */
uint32_t HAL_ArduinoMillis(void);

/**
 * @brief Delay for specified milliseconds
 * 
 * @param ms Number of milliseconds to delay
 */
void HAL_ArduinoDelay(uint32_t ms);

#endif /* HAL_ARDUINO_H */