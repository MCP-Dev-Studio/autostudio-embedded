#ifndef MCP_HOST_H
#define MCP_HOST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file mcp_host.h
 * @brief Host platform MCP interface
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Host platform configuration
 */
typedef struct MCP_HostConfig {
    const char* deviceName;     // Maps to common.device_name
    const char* version;        // Maps to common.firmware_version
    bool enableDebug;           // Maps to common.debug_enabled
    bool enablePersistence;     // Maps to common.enable_persistence
    uint32_t heapSize;          // Maps to common.heap_size
    const char* configFile;     // Maps to common.config_file_path
    bool enableServer;          // Maps to common.server_enabled
    uint16_t serverPort;        // Maps to common.server_port
    bool autoStartServer;       // Maps to common.auto_start_server
    bool enableTestMode;        // Maps to platform.host.enable_test_mode
    const char* testOutputDir;  // Maps to platform.host.test_output_dir
} MCP_HostConfig;

/**
 * @brief Initialize the MCP system for Host platform
 * 
 * @param config Platform configuration (can be NULL for defaults)
 * @return int 0 on success, negative error code on failure
 */
int MCP_SystemInit(const MCP_HostConfig* config);

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
 * @return int Number of tasks processed or negative error code
 */
int MCP_SystemProcess(void);

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

#ifdef __cplusplus
}
#endif

#endif /* MCP_HOST_H */