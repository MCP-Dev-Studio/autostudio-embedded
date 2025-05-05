#ifndef HAL_ESP32_H
#define HAL_ESP32_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ESP32 pin modes
 */
enum HAL_ESP32PinMode {
    HAL_ESP32_PIN_MODE_INPUT = 0,
    HAL_ESP32_PIN_MODE_OUTPUT = 1,
    HAL_ESP32_PIN_MODE_INPUT_PULLUP = 2,
    HAL_ESP32_PIN_MODE_INPUT_PULLDOWN = 3,
    HAL_ESP32_PIN_MODE_OUTPUT_OPEN_DRAIN = 4,
    HAL_ESP32_PIN_MODE_ANALOG = 5,
    HAL_ESP32_PIN_MODE_SPECIAL = 6
};

/**
 * @brief ESP32 interrupt modes
 */
enum HAL_ESP32InterruptMode {
    HAL_ESP32_INT_DISABLE = 0,
    HAL_ESP32_INT_RISING = 1,
    HAL_ESP32_INT_FALLING = 2,
    HAL_ESP32_INT_CHANGE = 3,
    HAL_ESP32_INT_ONLOW = 4,
    HAL_ESP32_INT_ONHIGH = 5,
    HAL_ESP32_INT_ONLOW_WE = 6,
    HAL_ESP32_INT_ONHIGH_WE = 7
};

/**
 * @brief Initialize the ESP32 HAL
 * 
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32Init(void);

/**
 * @brief Deinitialize the ESP32 HAL
 * 
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32Deinit(void);

/**
 * @brief Configure a digital pin
 * 
 * @param pin Pin number
 * @param mode Pin mode
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32PinMode(uint8_t pin, uint8_t mode);

/**
 * @brief Write to a digital pin
 * 
 * @param pin Pin number
 * @param value Pin value (0 = low, 1 = high)
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32DigitalWrite(uint8_t pin, uint8_t value);

/**
 * @brief Read from a digital pin
 * 
 * @param pin Pin number
 * @return int Pin value (0 or 1) or negative error code
 */
int HAL_ESP32DigitalRead(uint8_t pin);

/**
 * @brief Write analog value (DAC) to a pin
 * 
 * @param pin Pin number
 * @param value Analog value (0-255)
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32AnalogWrite(uint8_t pin, uint8_t value);

/**
 * @brief Read analog value from a pin
 * 
 * @param pin Pin number
 * @param attenuationDb ADC attenuation in dB (0, 2.5, 6, 11)
 * @param width ADC width in bits (9-12)
 * @return int Analog value or negative error code
 */
int HAL_ESP32AnalogRead(uint8_t pin, uint8_t attenuationDb, uint8_t width);

/**
 * @brief Configure PWM for a pin
 * 
 * @param pin Pin number
 * @param channel PWM channel (0-15)
 * @param frequency PWM frequency in Hz
 * @param resolution PWM resolution in bits
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32PWMConfig(uint8_t pin, uint8_t channel, uint32_t frequency, uint8_t resolution);

/**
 * @brief Set PWM duty cycle
 * 
 * @param channel PWM channel (0-15)
 * @param dutyCycle Duty cycle value
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32PWMSetDutyCycle(uint8_t channel, uint32_t dutyCycle);

/**
 * @brief Attach interrupt to a pin
 * 
 * @param pin Pin number
 * @param mode Interrupt mode
 * @param callback Interrupt callback function
 * @param arg Callback argument
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32AttachInterrupt(uint8_t pin, uint8_t mode, void (*callback)(void*), void* arg);

/**
 * @brief Detach interrupt from a pin
 * 
 * @param pin Pin number
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32DetachInterrupt(uint8_t pin);

/**
 * @brief Configure a UART interface
 * 
 * @param uart UART number (0-2)
 * @param txPin TX pin number
 * @param rxPin RX pin number
 * @param baudRate Baud rate
 * @param config UART configuration (platform-specific)
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32UARTConfig(uint8_t uart, uint8_t txPin, uint8_t rxPin, 
                        uint32_t baudRate, uint32_t config);

/**
 * @brief Read from a UART interface
 * 
 * @param uart UART number (0-2)
 * @param buffer Buffer to store read data
 * @param size Maximum number of bytes to read
 * @param timeout Read timeout in milliseconds
 * @return int Number of bytes read or negative error code
 */
int HAL_ESP32UARTRead(uint8_t uart, uint8_t* buffer, size_t size, uint32_t timeout);

/**
 * @brief Write to a UART interface
 * 
 * @param uart UART number (0-2)
 * @param data Data to write
 * @param size Number of bytes to write
 * @return int Number of bytes written or negative error code
 */
int HAL_ESP32UARTWrite(uint8_t uart, const uint8_t* data, size_t size);

/**
 * @brief Configure an I2C interface
 * 
 * @param i2c I2C number (0-1)
 * @param sdaPin SDA pin number
 * @param sclPin SCL pin number
 * @param frequency Clock frequency in Hz
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32I2CConfig(uint8_t i2c, uint8_t sdaPin, uint8_t sclPin, uint32_t frequency);

/**
 * @brief Write to an I2C device
 * 
 * @param i2c I2C number (0-1)
 * @param address Device address
 * @param data Data to write
 * @param size Number of bytes to write
 * @param stop Generate stop condition
 * @return int Number of bytes written or negative error code
 */
int HAL_ESP32I2CWrite(uint8_t i2c, uint8_t address, const uint8_t* data, size_t size, bool stop);

/**
 * @brief Read from an I2C device
 * 
 * @param i2c I2C number (0-1)
 * @param address Device address
 * @param data Buffer to store read data
 * @param size Number of bytes to read
 * @return int Number of bytes read or negative error code
 */
int HAL_ESP32I2CRead(uint8_t i2c, uint8_t address, uint8_t* data, size_t size);

/**
 * @brief Configure an SPI interface
 * 
 * @param spi SPI number (0-3)
 * @param mosiPin MOSI pin number
 * @param misoPin MISO pin number
 * @param sckPin SCK pin number
 * @param csPin CS pin number (or -1 for software CS)
 * @param frequency Clock frequency in Hz
 * @param mode SPI mode (0-3)
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32SPIConfig(uint8_t spi, int8_t mosiPin, int8_t misoPin, int8_t sckPin, 
                      int8_t csPin, uint32_t frequency, uint8_t mode);

/**
 * @brief Transfer data over SPI
 * 
 * @param spi SPI number (0-3)
 * @param txData Data to transmit
 * @param rxData Buffer to store received data
 * @param size Number of bytes to transfer
 * @return int Number of bytes transferred or negative error code
 */
int HAL_ESP32SPITransfer(uint8_t spi, const uint8_t* txData, uint8_t* rxData, size_t size);

/**
 * @brief Read from flash memory
 * 
 * @param address Flash address
 * @param data Buffer to store read data
 * @param size Number of bytes to read
 * @return int Number of bytes read or negative error code
 */
int HAL_ESP32FlashRead(uint32_t address, uint8_t* data, size_t size);

/**
 * @brief Write to flash memory
 * 
 * @param address Flash address
 * @param data Data to write
 * @param size Number of bytes to write
 * @return int Number of bytes written or negative error code
 */
int HAL_ESP32FlashWrite(uint32_t address, const uint8_t* data, size_t size);

/**
 * @brief Erase flash sector
 * 
 * @param sector Sector number
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32FlashEraseSector(uint32_t sector);

/**
 * @brief Get free heap memory size
 * 
 * @return uint32_t Free heap memory in bytes
 */
uint32_t HAL_ESP32GetFreeHeap(void);

/**
 * @brief Get system time in milliseconds
 * 
 * @return uint32_t System time in milliseconds
 */
uint32_t HAL_ESP32Millis(void);

/**
 * @brief Delay for specified milliseconds
 * 
 * @param ms Number of milliseconds to delay
 */
void HAL_ESP32Delay(uint32_t ms);

/**
 * @brief Restart the ESP32
 */
void HAL_ESP32Restart(void);

/**
 * @brief Get ESP32 chip information
 * 
 * @param model Pointer to store model number
 * @param revision Pointer to store revision number
 * @param cores Pointer to store number of cores
 * @param features Pointer to store feature flags
 * @return int 0 on success, negative error code on failure
 */
int HAL_ESP32GetChipInfo(uint32_t* model, uint32_t* revision, uint32_t* cores, uint32_t* features);

#ifdef __cplusplus
}
#endif

#endif /* HAL_ESP32_H */