#include "arduino_compat.h"
/**
 * @file mcp_arduino_config.c
 * @brief Arduino implementation of MCP configuration API
 */
#include "mcp_arduino.h"
#include "mcp_config.h"
#include "platform_config.h"
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
 * @brief Initialize the MCP system for Arduino platform
 */
int MCP_SystemInit(const MCP_ArduinoConfig* config) {
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
#if defined(MCP_PLATFORM_ARDUINO)
        s_config.analog_reference = config->analogReference;
        s_config.enable_watchdog = config->enableWatchdog;
#endif
        
        // Save the updated configuration
        MCP_SavePersistentState();
    }
    
    // Initialize analog reference if specified
#if defined(MCP_PLATFORM_ARDUINO)
    if (s_config.analog_reference >= 0) {
        // analogReference(s_config.analog_reference); // Uncomment in actual Arduino implementation
    }
    
    // Enable watchdog if requested
    if (s_config.enable_watchdog) {
        // wdt_enable(WDTO_2S); // Uncomment in actual Arduino implementation
    }
#endif
    
    MCP_SystemDebugPrint("MCP System initialized for Arduino platform\n");
    MCP_SystemDebugPrint("Device name: %s\n", s_config.common.device_name);
    MCP_SystemDebugPrint("Firmware version: %s\n", s_config.common.firmware_version);
    
    // Connect to WiFi if enabled
    if (s_config.common.network.enabled && s_config.common.network.auto_connect) {
        MCP_WiFiConnect(s_config.common.network.ssid, 
                        s_config.common.network.password, 
                        30000); // 30 second timeout
    }
    
    // Start server if auto-start is enabled
    if (s_config.common.server_enabled && s_config.common.auto_start_server) {
        MCP_ServerStart();
    }
    
    return 0;
}

/**
 * @brief Connect to WiFi network (ESP8266/ESP32 only)
 */
int MCP_WiFiConnect(const char* ssid, const char* password, uint32_t timeout) {
    if (ssid == NULL) {
        return -1;
    }
    
    // Save credentials to configuration
    strncpy(s_config.common.network.ssid, ssid, sizeof(s_config.common.network.ssid) - 1);
    
    if (password != NULL) {
        strncpy(s_config.common.network.password, password, sizeof(s_config.common.network.password) - 1);
    } else {
        s_config.common.network.password[0] = '\0';
    }
    
    s_config.common.network.enabled = true;
    s_config.common.network.auto_connect = true;
    
    // Save configuration changes
    MCP_SavePersistentState();
    
    // Implement actual WiFi connection here for ESP8266/ESP32 Arduino
    // This is a placeholder - the actual implementation depends on the specific Arduino WiFi library
    /*
    WiFi.begin(ssid, password);
    
    uint32_t start_time = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start_time > timeout) {
            return -2; // Connection timeout
        }
        delay(500);
    }
    */
    
    return 0;
}

/**
 * @brief Disconnect from WiFi network (ESP8266/ESP32 only)
 */
int MCP_WiFiDisconnect(void) {
    s_config.common.network.auto_connect = false;
    
    // Implement actual WiFi disconnection here for ESP8266/ESP32 Arduino
    // WiFi.disconnect();
    
    return 0;
}

/**
 * @brief Get WiFi status (ESP8266/ESP32 only)
 */
int MCP_WiFiGetStatus(void) {
    // Implement actual WiFi status check here for ESP8266/ESP32 Arduino
    // return WiFi.status();
    return 0; // Placeholder
}

/**
 * @brief Get WiFi IP address as string (ESP8266/ESP32 only)
 */
int MCP_WiFiGetIP(char* buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // Implement actual IP address retrieval for ESP8266/ESP32 Arduino
    /*
    IPAddress ip = WiFi.localIP();
    snprintf(buffer, bufferSize, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    */
    
    strncpy(buffer, "0.0.0.0", bufferSize - 1); // Placeholder
    return strlen(buffer);
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
#if defined(MCP_PLATFORM_ARDUINO)
    if (s_config.analog_reference >= 0) {
        // analogReference(s_config.analog_reference); // Uncomment in actual Arduino implementation
    }
    
    // Update watchdog settings
    if (s_config.enable_watchdog) {
        // wdt_enable(WDTO_2S); // Uncomment in actual Arduino implementation
    } else {
        // wdt_disable(); // Uncomment in actual Arduino implementation
    }
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
    
    // WiFi status (if applicable)
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"wifi_enabled\":%s,",
            s_config.common.network.enabled ? "true" : "false");
    
    // Only include these fields for ESP8266/ESP32 based Arduino
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"wifi_connected\":%s,",
            MCP_WiFiGetStatus() > 0 ? "true" : "false");
    
    // Server information
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"server_enabled\":%s,"
            "\"server_port\":%u,",
            s_config.common.server_enabled ? "true" : "false",
            s_config.common.server_port);
    
    // Platform-specific information
#if defined(MCP_PLATFORM_ARDUINO)
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"analog_reference\":%d,"
            "\"watchdog_enabled\":%s",
            s_config.analog_reference,
            s_config.enable_watchdog ? "true" : "false");
#endif
    
    pos += snprintf(buffer + pos, bufferSize - pos, "}");
    
    return pos;
}

/**
 * @brief Print debug message (if debug enabled)
 */
void MCP_SystemDebugPrint(const char* format, ...) {
    if (!s_config.common.debug_enabled) {
        return;
    }
    
    // Implement platform-specific debug print here
    // For Arduino, this would typically use Serial.print
    /*
    va_list args;
    va_start(args, format);
    
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial.print(buffer);
    
    va_end(args);
    */
}
