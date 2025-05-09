#ifndef HAL_MBED_H
#define HAL_MBED_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize the mbed HAL
 * 
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedInit(void);

/**
 * @brief Deinitialize the mbed HAL
 * 
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedDeinit(void);

/**
 * @brief Configure a GPIO pin
 * 
 * @param pin Pin number
 * @param mode Pin mode (platform-specific)
 * @param initialState Initial state for output pins
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedGPIOConfig(uint32_t pin, uint32_t mode, bool initialState);

/**
 * @brief Write to a GPIO pin
 * 
 * @param pin Pin number
 * @param value Pin value (0 = low, 1 = high)
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedGPIOWrite(uint32_t pin, uint32_t value);

/**
 * @brief Read from a GPIO pin
 * 
 * @param pin Pin number
 * @param value Pointer to store pin value (0 = low, 1 = high)
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedGPIORead(uint32_t pin, uint32_t* value);

/**
 * @brief Configure I2C interface
 * 
 * @param instance I2C instance number
 * @param sclPin SCL pin number
 * @param sdaPin SDA pin number
 * @param frequency Clock frequency in Hz
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedI2CConfig(uint32_t instance, uint32_t sclPin, uint32_t sdaPin, uint32_t frequency);

/**
 * @brief Write to I2C device
 * 
 * @param instance I2C instance number
 * @param address Device address
 * @param data Data to write
 * @param size Data size
 * @param stop Generate stop condition
 * @return int Number of bytes written or negative error code
 */
int HAL_MbedI2CWrite(uint32_t instance, uint8_t address, const uint8_t* data, size_t size, bool stop);

/**
 * @brief Read from I2C device
 * 
 * @param instance I2C instance number
 * @param address Device address
 * @param data Buffer to store read data
 * @param size Data size to read
 * @return int Number of bytes read or negative error code
 */
int HAL_MbedI2CRead(uint32_t instance, uint8_t address, uint8_t* data, size_t size);

/**
 * @brief Configure SPI interface
 * 
 * @param instance SPI instance number
 * @param mosiPin MOSI pin number
 * @param misoPin MISO pin number
 * @param sckPin SCK pin number
 * @param frequency Clock frequency in Hz
 * @param mode SPI mode (0-3)
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedSPIConfig(uint32_t instance, uint32_t mosiPin, uint32_t misoPin, 
                      uint32_t sckPin, uint32_t frequency, uint32_t mode);

/**
 * @brief Transfer data over SPI
 * 
 * @param instance SPI instance number
 * @param txData Data to transmit (can be NULL for read-only)
 * @param rxData Buffer to store received data (can be NULL for write-only)
 * @param size Data size
 * @return int Number of bytes transferred or negative error code
 */
int HAL_MbedSPITransfer(uint32_t instance, const uint8_t* txData, uint8_t* rxData, size_t size);

/**
 * @brief Configure ADC for analog reading
 * 
 * @param instance ADC instance number
 * @param pin Pin number
 * @param resolution Resolution in bits
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedADCConfig(uint32_t instance, uint32_t pin, uint32_t resolution);

/**
 * @brief Read ADC value
 * 
 * @param instance ADC instance number
 * @param pin Pin number
 * @param value Pointer to store ADC value
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedADCRead(uint32_t instance, uint32_t pin, uint32_t* value);

/**
 * @brief Configure DAC for analog output
 * 
 * @param instance DAC instance number
 * @param pin Pin number
 * @param resolution Resolution in bits
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedDACConfig(uint32_t instance, uint32_t pin, uint32_t resolution);

/**
 * @brief Write DAC value
 * 
 * @param instance DAC instance number
 * @param pin Pin number
 * @param value DAC value
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedDACWrite(uint32_t instance, uint32_t pin, uint32_t value);

/**
 * @brief Configure PWM output
 * 
 * @param instance PWM instance number
 * @param pin Pin number
 * @param frequency PWM frequency in Hz
 * @param dutyCycle Initial duty cycle (0-1.0)
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedPWMConfig(uint32_t instance, uint32_t pin, uint32_t frequency, float dutyCycle);

/**
 * @brief Set PWM duty cycle
 * 
 * @param instance PWM instance number
 * @param pin Pin number
 * @param dutyCycle Duty cycle (0-1.0)
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedPWMSetDutyCycle(uint32_t instance, uint32_t pin, float dutyCycle);

/**
 * @brief Get system time in milliseconds
 * 
 * @return uint32_t System time in milliseconds
 */
uint32_t HAL_MbedGetTimeMs(void);

/**
 * @brief Sleep for specified milliseconds
 * 
 * @param ms Milliseconds to sleep
 */
void HAL_MbedSleepMs(uint32_t ms);

/**
 * @brief Read from flash memory
 * 
 * @param address Flash address
 * @param data Buffer to store read data
 * @param size Data size to read
 * @return int Number of bytes read or negative error code
 */
int HAL_MbedFlashRead(uint32_t address, uint8_t* data, size_t size);

/**
 * @brief Write to flash memory
 * 
 * @param address Flash address
 * @param data Data to write
 * @param size Data size
 * @return int Number of bytes written or negative error code
 */
int HAL_MbedFlashWrite(uint32_t address, const uint8_t* data, size_t size);

/**
 * @brief Erase flash sector
 * 
 * @param sector Sector number
 * @return int 0 on success, negative error code on failure
 */
int HAL_MbedFlashEraseSector(uint32_t sector);

#endif /* HAL_MBED_H */