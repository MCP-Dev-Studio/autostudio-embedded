#ifndef HAL_HOST_H
#define HAL_HOST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file hal_host.h
 * @brief Host platform HAL for testing and development
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the host HAL
 * 
 * @return int 0 on success, negative error code on failure
 */
int HAL_HostInit(void);

/**
 * @brief Deinitialize the host HAL
 * 
 * @return int 0 on success, negative error code on failure
 */
int HAL_HostDeinit(void);

/**
 * @brief Sleep for a specified number of milliseconds
 * 
 * @param ms Milliseconds to sleep
 */
void HAL_HostSleep(uint32_t ms);

/**
 * @brief Get system time in milliseconds
 * 
 * @return uint32_t System time in milliseconds
 */
uint32_t HAL_HostGetTimeMs(void);

/**
 * @brief Get free heap memory size
 * 
 * @return uint32_t Free heap memory in bytes
 */
uint32_t HAL_HostGetFreeHeap(void);

/**
 * @brief Print debug message
 * 
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void HAL_HostDebugPrint(const char* format, ...);

/**
 * @brief Read data from a file
 * 
 * @param filename File path to read
 * @param buffer Buffer to store read data
 * @param size Maximum number of bytes to read
 * @return int Number of bytes read or negative error code
 */
int HAL_HostFileRead(const char* filename, void* buffer, size_t size);

/**
 * @brief Write data to a file
 * 
 * @param filename File path to write
 * @param data Data to write
 * @param size Number of bytes to write
 * @return int Number of bytes written or negative error code
 */
int HAL_HostFileWrite(const char* filename, const void* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* HAL_HOST_H */