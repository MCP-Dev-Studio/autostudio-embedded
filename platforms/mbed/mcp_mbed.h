#ifndef MCP_MBED_H
#define MCP_MBED_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief mbed platform configuration
 */
typedef struct {
    const char* deviceName;     // Device name
    const char* version;        // Firmware version
    bool enableDebug;           // Enable debug output
    bool enablePersistence;     // Enable persistent storage
    uint32_t heapSize;          // Memory heap size
    const char* configFile;     // Configuration file path
    bool enableServer;          // Enable MCP server
    uint16_t serverPort;        // Server port (for TCP)
    bool autoStartServer;       // Automatically start server
} MCP_MbedConfig;

/**
 * @brief Initialize the MCP system for mbed platform
 * 
 * @param config Platform configuration (can be NULL for defaults)
 * @return int 0 on success, negative error code on failure
 */
int MCP_SystemInit(const MCP_MbedConfig* config);

/**
 * @brief Start the MCP server
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_ServerStart(void);

/**
 * @brief Load persistent state from storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoadPersistentState(void);

/**
 * @brief Save persistent state to storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_SavePersistentState(void);

/**
 * @brief Process system tasks
 * 
 * @param timeout Maximum time to block in milliseconds
 * @return int Number of tasks processed or negative error code
 */
int MCP_SystemProcess(uint32_t timeout);

/**
 * @brief Deinitialize the MCP system
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_SystemDeinit(void);

/**
 * @brief Get system status as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_SystemGetStatus(char* buffer, size_t bufferSize);

/**
 * @brief Set debug output enable state
 * 
 * @param enable Enable debug output
 * @return int Previous enable state
 */
int MCP_SystemSetDebug(bool enable);

/**
 * @brief Print debug message (if debug enabled)
 * 
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void MCP_SystemDebugPrint(const char* format, ...);

/**
 * @brief Get system time in milliseconds
 * 
 * @return uint32_t System time in milliseconds
 */
uint32_t MCP_SystemGetTimeMs(void);

/**
 * @brief Sleep for specified milliseconds
 * 
 * @param ms Milliseconds to sleep
 */
void MCP_SystemSleepMs(uint32_t ms);

#endif /* MCP_MBED_H */