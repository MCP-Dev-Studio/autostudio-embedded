/**
 * @file mcp_esp32_config.c
 * @brief ESP32 implementation of MCP configuration API
 */
#include "mcp_esp32.h"
#include "../../core/mcp/config/mcp_config.h"
#include "../../core/mcp/config/platform_config.h"
#include <stdio.h>
#include <string.h>

// Configuration state
static MCP_PLATFORM_CONFIG s_config;
static bool s_config_initialized = false;

/**
 * @brief Load persistent state from storage
 */
int MCP_LoadPersistentState(void) {
    if (!s_config_initialized) {
        // Initialize configuration with defaults
        MCP_ConfigInit(&s_config);
        s_config_initialized = true;
    }
    
    // Load configuration from file
    return MCP_ConfigLoad(&s_config, NULL);
}

/**
 * @brief Save persistent state to storage
 */
int MCP_SavePersistentState(void) {
    if (!s_config_initialized) {
        // Initialize configuration with defaults if not already initialized
        MCP_ConfigInit(&s_config);
        s_config_initialized = true;
    }
    
    // Save configuration to file
    return MCP_ConfigSave(&s_config, NULL);
}

/**
 * @brief Initialize the MCP system for ESP32 platform
 */
int MCP_SystemInit(const MCP_ESP32Config* config) {
    // Initialize configuration if not already initialized
    if (!s_config_initialized) {
        MCP_ConfigInit(&s_config);
        s_config_initialized = true;
    }
    
    // Load persistent configuration
    MCP_LoadPersistentState();
    
    // Override with provided configuration if not NULL
    if (config != NULL) {
        // Convert platform-specific config to common config
        PlatformConfigToCommonConfig((void*)config, &s_config.common);
        
        // Save the updated configuration
        MCP_SavePersistentState();
    }
    
    printf("MCP System initialized for ESP32 platform\n");
    printf("Device name: %s\n", s_config.common.device_name);
    printf("Firmware version: %s\n", s_config.common.firmware_version);
    
    return 0;
}

/**
 * @brief Set a configuration parameter via JSON
 */
int MCP_SetConfiguration(const char* json_config) {
    if (json_config == NULL) {
        return -1;
    }
    
    // Initialize configuration if not already initialized
    if (!s_config_initialized) {
        MCP_ConfigInit(&s_config);
        s_config_initialized = true;
    }
    
    // Parse JSON and update configuration
    int result = MCP_ConfigUpdateFromJSON(&s_config, json_config);
    if (result != 0) {
        return result;
    }
    
    // Save the updated configuration
    return MCP_SavePersistentState();
}

/**
 * @brief Get the current configuration as JSON
 */
int MCP_GetConfiguration(char* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return -1;
    }
    
    // Initialize configuration if not already initialized
    if (!s_config_initialized) {
        MCP_ConfigInit(&s_config);
        s_config_initialized = true;
    }
    
    // Serialize configuration to JSON
    return MCP_ConfigSerializeToJSON(&s_config, buffer, size);
}

/**
 * @brief Get system status as JSON
 */
int MCP_SystemGetStatus(char* buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // Basic system status
    snprintf(buffer, bufferSize,
             "{"
             "\"status\":\"running\","
             "\"device\":\"%s\","
             "\"firmware\":\"%s\","
             "\"wifi_enabled\":%s,"
             "\"wifi_connected\":%s,"
             "\"server_enabled\":%s,"
             "\"server_port\":%u"
             "}",
             s_config.common.device_name,
             s_config.common.firmware_version,
             s_config.common.network.enabled ? "true" : "false",
             MCP_WiFiGetStatus() > 0 ? "true" : "false",
             s_config.common.server_enabled ? "true" : "false",
             s_config.common.server_port);
    
    return strlen(buffer);
}