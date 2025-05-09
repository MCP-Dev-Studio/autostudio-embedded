#include "mcp_host.h"
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
 * @brief Initialize the MCP system for Host platform
 */
int MCP_HostSystemInit(const MCP_HostConfig* config) {
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
        // Maps platform-specific names to common config structure fields
        if (config->deviceName != NULL) {
            strncpy(s_config.common.device_name, config->deviceName, sizeof(s_config.common.device_name) - 1);
        }
        if (config->version != NULL) {
            strncpy(s_config.common.firmware_version, config->version, sizeof(s_config.common.firmware_version) - 1);
        }
        s_config.common.debug_enabled = config->enableDebug;
        s_config.common.enable_persistence = config->enablePersistence;
        s_config.common.heap_size = config->heapSize;
        if (config->configFile != NULL) {
            strncpy(s_config.common.config_file_path, config->configFile, sizeof(s_config.common.config_file_path) - 1);
        }
        s_config.common.server_enabled = config->enableServer;
        s_config.common.server_port = config->serverPort;
        s_config.common.auto_start_server = config->autoStartServer;
        
        // Platform-specific fields
        s_config.test_mode_enabled = config->enableTestMode;
        if (config->testOutputDir != NULL) {
            strncpy(s_config.test_output_dir, config->testOutputDir, sizeof(s_config.test_output_dir) - 1);
        }
        
        // Save the updated configuration
        MCP_SavePersistentState();
    }
    
    printf("MCP Host System initialized\n");
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