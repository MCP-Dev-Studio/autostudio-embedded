#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief DS18B20 resolution enumeration
 */
typedef enum {
    DS18B20_RESOLUTION_9BIT = 0,  // 0.5°C (93.75ms conversion time)
    DS18B20_RESOLUTION_10BIT = 1, // 0.25°C (187.5ms conversion time)
    DS18B20_RESOLUTION_11BIT = 2, // 0.125°C (375ms conversion time)
    DS18B20_RESOLUTION_12BIT = 3  // 0.0625°C (750ms conversion time)
} DS18B20Resolution;

/**
 * @brief DS18B20 configuration
 */
typedef struct {
    uint8_t pin;                 // OneWire pin number
    uint64_t address;            // DS18B20 ROM address (0 for single device mode)
    DS18B20Resolution resolution; // Temperature resolution
    bool useCRC;                 // Enable CRC checking
    uint32_t conversionTimeout;  // Conversion timeout in milliseconds
} DS18B20Config;

/**
 * @brief DS18B20 initialization
 * 
 * @param config DS18B20 configuration
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_Init(const DS18B20Config* config);

/**
 * @brief DS18B20 deinitialization
 * 
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_Deinit(void);

/**
 * @brief Start temperature conversion
 * 
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_StartConversion(void);

/**
 * @brief Read temperature
 * 
 * @param temperature Pointer to store temperature in Celsius
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_ReadTemperature(float* temperature);

/**
 * @brief Set temperature resolution
 * 
 * @param resolution Temperature resolution
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_SetResolution(DS18B20Resolution resolution);

/**
 * @brief Read power supply mode
 * 
 * @param parasite Pointer to store parasite power mode flag
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_ReadPowerSupply(bool* parasite);

/**
 * @brief Get conversion time for current resolution
 * 
 * @return uint32_t Conversion time in milliseconds
 */
uint32_t DS18B20_GetConversionTime(void);

/**
 * @brief Scan for DS18B20 devices on the bus
 * 
 * @param addresses Array to store device addresses
 * @param maxDevices Maximum number of devices to find
 * @return int Number of devices found or negative error code
 */
int DS18B20_ScanDevices(uint64_t* addresses, uint8_t maxDevices);

/**
 * @brief Select device by address
 * 
 * @param address Device address (0 for single device mode)
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_SelectDevice(uint64_t address);

/**
 * @brief Read device address
 * 
 * @param address Pointer to store device address
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_ReadAddress(uint64_t* address);

/**
 * @brief Read device scratchpad
 * 
 * @param scratchpad Buffer to store scratchpad data (9 bytes)
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_ReadScratchpad(uint8_t* scratchpad);

/**
 * @brief Write device scratchpad (TH, TL, configuration registers)
 * 
 * @param th High temperature alarm register
 * @param tl Low temperature alarm register
 * @param config Configuration register
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_WriteScratchpad(uint8_t th, uint8_t tl, uint8_t config);

/**
 * @brief Copy scratchpad to EEPROM
 * 
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_CopyScratchpad(void);

/**
 * @brief Recall settings from EEPROM
 * 
 * @return int 0 on success, negative error code on failure
 */
int DS18B20_RecallEEPROM(void);

#endif /* DS18B20_H */