/**
 * @file mcp_config.h
 * @brief MCP configuration system for persistent settings
 * 
 * This file defines a common configuration system for all MCP platforms,
 * allowing settings to be stored, loaded, and modified at runtime.
 */
#ifndef MCP_CONFIG_H
#define MCP_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Common configuration structure for all platforms
 * 
 * This is the base configuration structure that all platform-specific
 * configurations extend. It contains common settings that apply to
 * all platforms.
 */
typedef struct {
    // Device information
    char device_name[64];
    char firmware_version[32];
    bool debug_enabled;
    
    // Server configuration
    bool server_enabled;
    uint16_t server_port;
    bool auto_start_server;
    
    // Network configuration
    struct {
        // WiFi configuration
        bool enabled;
        char ssid[64];
        char password[64];
        bool auto_connect;
        
        // WiFi AP configuration
        struct {
            bool enabled;
            char ssid[64];
            char password[64];
            uint16_t channel;
        } ap;
        
        // Ethernet configuration
        struct {
            bool enabled;
            char interface[16];
            bool dhcp;
            char static_ip[16];
            char gateway[16];
            char subnet[16];
        } ethernet;
        
        // BLE configuration
        struct {
            bool enabled;
            char device_name[64];
            bool auto_advertise;
        } ble;
    } network;
    
    // Interface configurations
    struct {
        // I2C configuration
        struct {
            bool enabled;
            int bus_number;
        } i2c;
        
        // SPI configuration
        struct {
            bool enabled;
            int bus_number;
        } spi;
        
        // UART configuration
        struct {
            bool enabled;
            int number;
            uint32_t baud_rate;
        } uart;
        
        // GPIO configuration
        struct {
            bool enabled;
        } gpio;
    } interfaces;
    
    // System configurations
    uint32_t heap_size;
    char config_file_path[128];
    bool enable_persistence;
} MCP_CommonConfig;

/**
 * @brief Platform-specific configuration extensions
 * 
 * Each platform can define additional configuration fields
 * as needed. The MCP_Config union allows accessing either
 * the common fields or platform-specific fields.
 */
#if defined(MCP_PLATFORM_RPI)
typedef struct {
    MCP_CommonConfig common;
    // Raspberry Pi specific settings
    bool enable_camera;
    int camera_resolution;
} MCP_RPiSpecificConfig;
#define MCP_PLATFORM_CONFIG MCP_RPiSpecificConfig

#elif defined(MCP_PLATFORM_ESP32)
typedef struct {
    MCP_CommonConfig common;
    // ESP32 specific settings
    bool enable_ota;
    bool enable_web_server;
    uint16_t web_server_port;
    bool enable_deep_sleep;
    uint32_t deep_sleep_time_ms;
} MCP_ESP32SpecificConfig;
#define MCP_PLATFORM_CONFIG MCP_ESP32SpecificConfig

#elif defined(MCP_PLATFORM_ARDUINO)
typedef struct {
    MCP_CommonConfig common;
    // Arduino specific settings
    int analog_reference;
    bool enable_watchdog;
} MCP_ArduinoSpecificConfig;
#define MCP_PLATFORM_CONFIG MCP_ArduinoSpecificConfig

#elif defined(MCP_PLATFORM_MBED)
typedef struct {
    MCP_CommonConfig common;
    // Mbed specific settings
    bool enable_rtos;
    uint32_t task_stack_size;
} MCP_MbedSpecificConfig;
#define MCP_PLATFORM_CONFIG MCP_MbedSpecificConfig

#elif defined(MCP_PLATFORM_HOST)
typedef struct {
    MCP_CommonConfig common;
    // Host specific settings for testing
    bool enable_mock_hardware;
    int test_mode;
} MCP_HostSpecificConfig;
#define MCP_PLATFORM_CONFIG MCP_HostSpecificConfig

#else
// Default to common config if platform is not specified
#define MCP_PLATFORM_CONFIG MCP_CommonConfig
#endif

// Configuration API functions

/**
 * @brief Initialize configuration with default values
 * 
 * @param config Pointer to configuration structure
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigInit(MCP_PLATFORM_CONFIG* config);

/**
 * @brief Load configuration from persistent storage
 * 
 * @param config Pointer to configuration structure to fill
 * @param file_path Path to configuration file (NULL for default)
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigLoad(MCP_PLATFORM_CONFIG* config, const char* file_path);

/**
 * @brief Save configuration to persistent storage
 * 
 * @param config Pointer to configuration structure to save
 * @param file_path Path to configuration file (NULL for default)
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigSave(const MCP_PLATFORM_CONFIG* config, const char* file_path);

/**
 * @brief Update configuration from JSON string
 * 
 * @param config Pointer to configuration structure to update
 * @param json_string JSON string containing configuration updates
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigUpdateFromJSON(MCP_PLATFORM_CONFIG* config, const char* json_string);

/**
 * @brief Serialize configuration to JSON string
 * 
 * @param config Pointer to configuration structure to serialize
 * @param buffer Buffer to store JSON string
 * @param size Size of buffer
 * @return int Length of JSON string written, or negative error code 
 */
int MCP_ConfigSerializeToJSON(const MCP_PLATFORM_CONFIG* config, char* buffer, size_t size);

/**
 * @brief Get the default configuration file path
 * 
 * @return const char* Default configuration file path
 */
const char* MCP_ConfigGetDefaultPath(void);

/**
 * @brief Platform-independent API for setting configuration via JSON
 * 
 * @param json_config JSON string containing configuration updates
 * @return int 0 on success, negative error code on failure
 */
int MCP_SetConfiguration(const char* json_config);

/**
 * @brief Platform-independent API for getting configuration as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param size Size of buffer
 * @return int Length of JSON string written, or negative error code
 */
int MCP_GetConfiguration(char* buffer, size_t size);

#endif /* MCP_CONFIG_H */