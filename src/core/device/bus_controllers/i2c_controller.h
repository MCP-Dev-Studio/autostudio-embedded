#ifndef MCP_I2C_CONTROLLER_H
#define MCP_I2C_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief I2C bus speed modes
 */
typedef enum {
    MCP_I2C_SPEED_STANDARD = 100000,  // 100 kHz
    MCP_I2C_SPEED_FAST = 400000,      // 400 kHz
    MCP_I2C_SPEED_FAST_PLUS = 1000000 // 1 MHz
} MCP_I2CBusSpeed;

/**
 * @brief I2C bus configuration
 */
typedef struct {
    uint8_t busNumber;        // I2C bus number
    MCP_I2CBusSpeed speed;    // Bus speed in Hz
    bool pullups;             // Enable internal pull-ups
    uint16_t timeout;         // Operation timeout in milliseconds
} MCP_I2CConfig;

/**
 * @brief I2C transfer result
 */
typedef struct {
    bool success;             // Transfer success flag
    uint8_t errorCode;        // Error code (0 = no error)
    uint16_t bytesTransferred; // Number of bytes transferred
} MCP_I2CResult;

/**
 * @brief Initialize I2C controller
 * 
 * @param config Bus configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_I2CInit(const MCP_I2CConfig* config);

/**
 * @brief Deinitialize I2C controller
 * 
 * @param busNumber Bus number to deinitialize
 * @return int 0 on success, negative error code on failure
 */
int MCP_I2CDeinit(uint8_t busNumber);

/**
 * @brief Write data to I2C device
 * 
 * @param busNumber Bus number
 * @param deviceAddress 7-bit device address
 * @param data Data to write
 * @param length Length of data
 * @param stop Generate stop condition after transfer
 * @return MCP_I2CResult Transfer result
 */
MCP_I2CResult MCP_I2CWrite(uint8_t busNumber, uint8_t deviceAddress, 
                          const uint8_t* data, size_t length, bool stop);

/**
 * @brief Read data from I2C device
 * 
 * @param busNumber Bus number
 * @param deviceAddress 7-bit device address
 * @param data Buffer to store read data
 * @param length Length of data to read
 * @return MCP_I2CResult Transfer result
 */
MCP_I2CResult MCP_I2CRead(uint8_t busNumber, uint8_t deviceAddress, 
                         uint8_t* data, size_t length);

/**
 * @brief Write then read from I2C device (for register access)
 * 
 * @param busNumber Bus number
 * @param deviceAddress 7-bit device address
 * @param writeData Data to write
 * @param writeLength Length of write data
 * @param readData Buffer to store read data
 * @param readLength Length of data to read
 * @return MCP_I2CResult Transfer result
 */
MCP_I2CResult MCP_I2CWriteRead(uint8_t busNumber, uint8_t deviceAddress, 
                              const uint8_t* writeData, size_t writeLength,
                              uint8_t* readData, size_t readLength);

/**
 * @brief Scan I2C bus for devices
 * 
 * @param busNumber Bus number
 * @param devices Array to store detected device addresses
 * @param maxDevices Maximum number of devices to detect
 * @return int Number of devices found or negative error code
 */
int MCP_I2CScan(uint8_t busNumber, uint8_t* devices, uint8_t maxDevices);

/**
 * @brief Check if I2C device is present
 * 
 * @param busNumber Bus number
 * @param deviceAddress 7-bit device address
 * @return bool True if device present, false otherwise
 */
bool MCP_I2CCheckDevice(uint8_t busNumber, uint8_t deviceAddress);

/**
 * @brief Set I2C bus speed
 * 
 * @param busNumber Bus number
 * @param speed Bus speed in Hz
 * @return int 0 on success, negative error code on failure
 */
int MCP_I2CSetSpeed(uint8_t busNumber, MCP_I2CBusSpeed speed);

/**
 * @brief Get I2C controller error description
 * 
 * @param errorCode Error code
 * @return const char* Error description
 */
const char* MCP_I2CGetErrorString(uint8_t errorCode);

#endif /* MCP_I2C_CONTROLLER_H */