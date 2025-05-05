#ifndef MCP_ARDUINO_H
#define MCP_ARDUINO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Arduino platform configuration
 */
typedef struct {
    const char* deviceName;     // Device name
    const char* version;        // Firmware version
    bool enableDebug;           // Enable debug output
    bool enablePersistence;     // Enable persistent storage
    uint32_t heapSize;          // Memory heap size
    const char* configFile;     // Configuration file path
    bool enableServer;          // Enable MCP server
    bool enableWifi;            // Enable WiFi (for ESP8266/ESP32)
    const char* ssid;           // WiFi SSID (if enableWifi)
    const char* password;       // WiFi password (if enableWifi)
    uint16_t serverPort;        // Server port (for TCP)
    bool autoStartServer;       // Automatically start server
} MCP_ArduinoConfig;

/**
 * @brief Initialize the MCP system for Arduino platform
 * 
 * @param config Platform configuration (can be NULL for defaults)
 * @return int 0 on success, negative error code on failure
 */
int MCP_SystemInit(const MCP_ArduinoConfig* config);

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

/**
 * @brief Connect to WiFi network (ESP8266/ESP32 only)
 * 
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @param timeout Connection timeout in milliseconds
 * @return int 0 on success, negative error code on failure
 */
int MCP_WiFiConnect(const char* ssid, const char* password, uint32_t timeout);

/**
 * @brief Disconnect from WiFi network (ESP8266/ESP32 only)
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_WiFiDisconnect(void);

/**
 * @brief Get WiFi status (ESP8266/ESP32 only)
 * 
 * @return int WiFi status code
 */
int MCP_WiFiGetStatus(void);

/**
 * @brief Get WiFi IP address as string (ESP8266/ESP32 only)
 * 
 * @param buffer Buffer to store IP address string
 * @param bufferSize Size of buffer
 * @return int Length of IP address string or negative error code
 */
int MCP_WiFiGetIP(char* buffer, size_t bufferSize);

#endif /* MCP_ARDUINO_H */