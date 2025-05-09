#ifndef MCP_SPI_CONTROLLER_H
#define MCP_SPI_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief SPI modes
 */
typedef enum {
    MCP_SPI_MODE_0,  // CPOL=0, CPHA=0
    MCP_SPI_MODE_1,  // CPOL=0, CPHA=1
    MCP_SPI_MODE_2,  // CPOL=1, CPHA=0
    MCP_SPI_MODE_3   // CPOL=1, CPHA=1
} MCP_SPIMode;

/**
 * @brief SPI bit order
 */
typedef enum {
    MCP_SPI_BIT_ORDER_MSB_FIRST,
    MCP_SPI_BIT_ORDER_LSB_FIRST
} MCP_SPIBitOrder;

/**
 * @brief SPI chip select mode
 */
typedef enum {
    MCP_SPI_CS_MODE_AUTO,    // Automatic chip select control
    MCP_SPI_CS_MODE_MANUAL   // Manual chip select control
} MCP_SPICSMode;

/**
 * @brief SPI bus configuration
 */
typedef struct {
    uint8_t busNumber;         // SPI bus number
    uint32_t clockSpeed;       // Clock speed in Hz
    MCP_SPIMode mode;          // SPI mode
    MCP_SPIBitOrder bitOrder;  // Bit order
    MCP_SPICSMode csMode;      // Chip select mode
    uint16_t timeout;          // Operation timeout in milliseconds
} MCP_SPIConfig;

/**
 * @brief SPI transfer result
 */
typedef struct {
    bool success;              // Transfer success flag
    uint8_t errorCode;         // Error code (0 = no error)
    uint16_t bytesTransferred; // Number of bytes transferred
} MCP_SPIResult;

/**
 * @brief Initialize SPI controller
 * 
 * @param config Bus configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_SPIInit(const MCP_SPIConfig* config);

/**
 * @brief Deinitialize SPI controller
 * 
 * @param busNumber Bus number to deinitialize
 * @return int 0 on success, negative error code on failure
 */
int MCP_SPIDeinit(uint8_t busNumber);

/**
 * @brief Transfer data over SPI bus
 * 
 * @param busNumber Bus number
 * @param csPin Chip select pin number (or -1 for auto/software CS)
 * @param txData Data to transmit (can be NULL for read-only)
 * @param rxData Buffer to store received data (can be NULL for write-only)
 * @param length Length of data to transfer
 * @return MCP_SPIResult Transfer result
 */
MCP_SPIResult MCP_SPITransfer(uint8_t busNumber, int csPin, 
                             const uint8_t* txData, uint8_t* rxData, size_t length);

/**
 * @brief Write data to SPI device
 * 
 * @param busNumber Bus number
 * @param csPin Chip select pin number (or -1 for auto/software CS)
 * @param data Data to write
 * @param length Length of data
 * @return MCP_SPIResult Transfer result
 */
MCP_SPIResult MCP_SPIWrite(uint8_t busNumber, int csPin, const uint8_t* data, size_t length);

/**
 * @brief Read data from SPI device
 * 
 * @param busNumber Bus number
 * @param csPin Chip select pin number (or -1 for auto/software CS)
 * @param data Buffer to store read data
 * @param length Length of data to read
 * @return MCP_SPIResult Transfer result
 */
MCP_SPIResult MCP_SPIRead(uint8_t busNumber, int csPin, uint8_t* data, size_t length);

/**
 * @brief Set chip select pin state manually
 * 
 * @param busNumber Bus number
 * @param csPin Chip select pin number
 * @param state Chip select state (true = active/low, false = inactive/high)
 * @return int 0 on success, negative error code on failure
 */
int MCP_SPISetCS(uint8_t busNumber, int csPin, bool state);

/**
 * @brief Set SPI bus parameters
 * 
 * @param busNumber Bus number
 * @param clockSpeed Clock speed in Hz
 * @param mode SPI mode
 * @param bitOrder Bit order
 * @return int 0 on success, negative error code on failure
 */
int MCP_SPISetParameters(uint8_t busNumber, uint32_t clockSpeed, 
                        MCP_SPIMode mode, MCP_SPIBitOrder bitOrder);

/**
 * @brief Get SPI controller error description
 * 
 * @param errorCode Error code
 * @return const char* Error description
 */
const char* MCP_SPIGetErrorString(uint8_t errorCode);

#endif /* MCP_SPI_CONTROLLER_H */