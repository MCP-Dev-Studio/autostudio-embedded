/**
 * @file mcp_mbed_config.c
 * @brief Mbed implementation of MCP configuration API
 */
#include "mcp_mbed.h"
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
 * @brief Initialize the MCP system for Mbed platform
 */
int MCP_SystemInit(const MCP_MbedConfig* config) {
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
        
        // Platform-specific settings
#if defined(MCP_PLATFORM_MBED)
        s_config.enable_rtos = config->enableRTOS;
        s_config.task_stack_size = config->taskStackSize;
#endif
        
        // Save the updated configuration
        MCP_SavePersistentState();
    }
    
#if defined(MCP_PLATFORM_MBED)
    // Initialize RTOS if enabled
    if (s_config.enable_rtos) {
        // Initialize RTOS components here
        // This is platform-specific code
    }
#endif
    
    MCP_SystemDebugPrint("MCP System initialized for Mbed platform\n");
    MCP_SystemDebugPrint("Device name: %s\n", s_config.common.device_name);
    MCP_SystemDebugPrint("Firmware version: %s\n", s_config.common.firmware_version);
    
    // Start server if auto-start is enabled
    if (s_config.common.server_enabled && s_config.common.auto_start_server) {
        MCP_ServerStart();
    }
    
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
    
    // Apply any immediate configuration changes
#if defined(MCP_PLATFORM_MBED)
    // Handle any immediate configuration changes for Mbed
    // This is platform-specific code
#endif
    
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
 * @brief Set debug output enable state
 */
int MCP_SystemSetDebug(bool enable) {
    bool previous = s_config.common.debug_enabled;
    s_config.common.debug_enabled = enable;
    
    // Save the updated configuration
    MCP_SavePersistentState();
    
    return previous;
}

/**
 * @brief Print debug message (if debug enabled)
 */
void MCP_SystemDebugPrint(const char* format, ...) {
    if (!s_config.common.debug_enabled) {
        return;
    }
    
    // Implement platform-specific debug print here
    // This would typically use printf for Mbed
    /*
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    */
}

/**
 * @brief Sleep for specified milliseconds
 */
void MCP_SystemSleepMs(uint32_t ms) {
    // Implement platform-specific sleep here
    // For Mbed, this would typically use wait_ms or ThisThread::sleep_for
    /*
    ThisThread::sleep_for(std::chrono::milliseconds(ms));
    */
}

/**
 * @brief Process system tasks
 */
int MCP_SystemProcess(uint32_t timeout) {
    // Process any system tasks
    // This implementation is platform-specific
    
    // Sleep for the specified timeout
    if (timeout > 0) {
        MCP_SystemSleepMs(timeout);
    }
    
    return 0;
}

/**
 * @brief Get system status as JSON
 */
int MCP_SystemGetStatus(char* buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // Basic system status
    int pos = 0;
    pos += snprintf(buffer + pos, bufferSize - pos,
            "{"
            "\"status\":\"running\","
            "\"device\":\"%s\","
            "\"firmware\":\"%s\",",
            s_config.common.device_name,
            s_config.common.firmware_version);
    
    // Server information
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"server_enabled\":%s,"
            "\"server_port\":%u,",
            s_config.common.server_enabled ? "true" : "false",
            s_config.common.server_port);
    
    // Platform-specific information
#if defined(MCP_PLATFORM_MBED)
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"rtos_enabled\":%s,"
            "\"task_stack_size\":%u",
            s_config.enable_rtos ? "true" : "false",
            s_config.task_stack_size);
#endif
    
    pos += snprintf(buffer + pos, bufferSize - pos, "}");
    
    return pos;
}