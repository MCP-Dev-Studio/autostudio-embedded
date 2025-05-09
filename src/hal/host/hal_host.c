#include "hal_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

// Flag to track initialization state
static bool s_initialized = false;

/**
 * @brief Initialize the host HAL
 */
int HAL_HostInit(void) {
    if (s_initialized) {
        return 0;  // Already initialized
    }
    
    printf("Host HAL initialized\n");
    s_initialized = true;
    return 0;
}

/**
 * @brief Deinitialize the host HAL
 */
int HAL_HostDeinit(void) {
    if (!s_initialized) {
        return 0;  // Already deinitialized
    }
    
    printf("Host HAL deinitialized\n");
    s_initialized = false;
    return 0;
}

/**
 * @brief Sleep for a specified number of milliseconds
 */
void HAL_HostSleep(uint32_t ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

/**
 * @brief Get system time in milliseconds
 */
uint32_t HAL_HostGetTimeMs(void) {
#ifdef _WIN32
    return (uint32_t)GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

/**
 * @brief Get free heap memory size
 * Note: This is a simplified implementation for host platform
 */
uint32_t HAL_HostGetFreeHeap(void) {
    // This is just a stub for host platform
    // In a real embedded system, this would return actual free heap memory
    return 1024 * 1024;  // Return 1MB as a placeholder
}

/**
 * @brief Print debug message
 */
void HAL_HostDebugPrint(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

/**
 * @brief Read data from a file
 */
int HAL_HostFileRead(const char* filename, void* buffer, size_t size) {
    if (filename == NULL || buffer == NULL || size == 0) {
        return -1;
    }
    
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return -2;
    }
    
    size_t bytesRead = fread(buffer, 1, size, file);
    fclose(file);
    
    return (int)bytesRead;
}

/**
 * @brief Write data to a file
 */
int HAL_HostFileWrite(const char* filename, const void* data, size_t size) {
    if (filename == NULL || data == NULL || size == 0) {
        return -1;
    }
    
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        return -2;
    }
    
    size_t bytesWritten = fwrite(data, 1, size, file);
    fclose(file);
    
    return (int)bytesWritten;
}