#include "arduino_compat.h"
#include "mcp_arduino.h"
#include "hal_arduino.h"
#include "mcp_config.h"
#include "platform_config.h"
#include "server.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Arduino MCP system implementation
static MCP_ArduinoConfig s_arduino_config;
static bool s_server_running = false;
static bool s_wifi_connected = false;
static uint32_t s_start_time = 0;

/**
 * @brief Initialize the MCP system for Arduino platform
 */
int MCP_SystemInit(const MCP_ArduinoConfig* config) {
    // Initialize HAL
    HAL_ArduinoInit();
    
    // Initialize Arduino-specific configuration
    if (config != NULL) {
        memcpy(&s_arduino_config, config, sizeof(MCP_ArduinoConfig));
    } else {
        // Set default configuration
        s_arduino_config.deviceName = "Arduino MCP Device";
        s_arduino_config.version = "1.0.0";
        s_arduino_config.enableDebug = true;
        s_arduino_config.enablePersistence = true;
        s_arduino_config.heapSize = 32768;
        s_arduino_config.configFile = "/config.json";
        s_arduino_config.enableServer = true;
        s_arduino_config.enableWifi = false;
        s_arduino_config.ssid = "";
        s_arduino_config.password = "";
        s_arduino_config.serverPort = 8080;
        s_arduino_config.autoStartServer = true;
        s_arduino_config.analogReference = 0;
        s_arduino_config.enableWatchdog = false;
    }
    
    // Load persistent state
    MCP_LoadPersistentState();
    
    // Store start time
    s_start_time = HAL_ArduinoMillis();
    
    // Initialize logging
    LogConfig logConfig = {
        .level = s_arduino_config.enableDebug ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO,
        .outputs = LOG_OUTPUT_SERIAL,
        .includeTimestamp = true,
        .includeLevelName = true
    };
    log_init(&logConfig);
    
    MCP_SystemDebugPrint("MCP system initialized on Arduino platform\n");
    MCP_SystemDebugPrint("Device name: %s\n", s_arduino_config.deviceName);
    MCP_SystemDebugPrint("Firmware version: %s\n", s_arduino_config.version);
    
    // Connect to WiFi if enabled
    if (s_arduino_config.enableWifi) {
        MCP_WiFiConnect(s_arduino_config.ssid, s_arduino_config.password, 30000);
    }
    
    // Auto-start server if enabled
    if (s_arduino_config.enableServer && s_arduino_config.autoStartServer) {
        MCP_ServerStart();
    }
    
    return 0;
}

/**
 * @brief Start the MCP server
 */
int MCP_ServerStart(void) {
    if (s_server_running) {
        return 0; // Already running
    }
    
    MCP_SystemDebugPrint("Starting MCP server on port %u\n", s_arduino_config.serverPort);
    
    // Create server configuration
    MCP_ServerConfig server_config = {0};
    server_config.deviceName = (char*)s_arduino_config.deviceName;
    server_config.version = (char*)s_arduino_config.version;
    server_config.maxSessions = 4;
    server_config.sessionTimeout = 30000;
    server_config.enableWifi = s_arduino_config.enableWifi;
    
    // Initialize the server
    int result = MCP_ServerInit(&server_config);
    if (result == 0) {
        s_server_running = true;
        MCP_SystemDebugPrint("MCP server started successfully\n");
    } else {
        MCP_SystemDebugPrint("Failed to start MCP server: %d\n", result);
    }
    
    return result;
}

/**
 * @brief Process system tasks
 */
int MCP_SystemProcess(void) {
    // Process server events if running
    if (s_server_running) {
        // Process server tasks
        // In a real implementation, this would handle connections, messages, etc.
    }
    
    // Update status
    
    return 0;
}

/**
 * @brief Deinitialize the MCP system
 */
int MCP_SystemDeinit(void) {
    // Save persistent state
    MCP_SavePersistentState();
    
    // Disconnect WiFi if connected
    if (s_wifi_connected) {
        MCP_WiFiDisconnect();
    }
    
    // Deinitialize HAL
    HAL_ArduinoDeinit();
    
    MCP_SystemDebugPrint("MCP system deinitialized\n");
    
    return 0;
}

/**
 * @brief Connect to WiFi network (ESP8266/ESP32 only)
 */
int MCP_WiFiConnect(const char* ssid, const char* password, uint32_t timeout) {
    if (ssid == NULL) {
        return -1;
    }
    
    MCP_SystemDebugPrint("Connecting to WiFi SSID: %s\n", ssid);
    
    // Save credentials for later use
    s_arduino_config.enableWifi = true;
    s_arduino_config.ssid = ssid;
    s_arduino_config.password = password ? password : "";
    
    // Save configuration
    MCP_SavePersistentState();
    
    // In a real implementation, this would connect to WiFi
    // For ESP8266/ESP32 boards
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific WiFi connection code
    /*
    WiFi.begin(ssid, password);
    
    uint32_t start_time = HAL_ArduinoMillis();
    while (WiFi.status() != WL_CONNECTED) {
        if (HAL_ArduinoMillis() - start_time > timeout) {
            return -2; // Connection timeout
        }
        HAL_ArduinoDelay(500);
    }
    
    s_wifi_connected = true;
    MCP_SystemDebugPrint("Connected to WiFi. IP: %s\n", WiFi.localIP().toString().c_str());
    */
#endif
    
    // For testing on non-Arduino platforms
    s_wifi_connected = true;
    MCP_SystemDebugPrint("Connected to WiFi. IP: 192.168.1.100\n");
    
    return 0;
}

/**
 * @brief Disconnect from WiFi network (ESP8266/ESP32 only)
 */
int MCP_WiFiDisconnect(void) {
    if (!s_wifi_connected) {
        return 0; // Already disconnected
    }
    
    MCP_SystemDebugPrint("Disconnecting from WiFi\n");
    
    // In a real implementation, this would disconnect from WiFi
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific WiFi disconnection code
    // WiFi.disconnect();
#endif
    
    s_wifi_connected = false;
    
    return 0;
}

/**
 * @brief Get WiFi status (ESP8266/ESP32 only)
 */
int MCP_WiFiGetStatus(void) {
    // In a real implementation, this would check WiFi status
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific WiFi status check
    // return WiFi.status();
#endif
    
    // For testing
    return s_wifi_connected ? 1 : 0;
}

/**
 * @brief Get WiFi IP address as string (ESP8266/ESP32 only)
 */
int MCP_WiFiGetIP(char* buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // In a real implementation, this would get the IP address
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific WiFi IP retrieval
    /*
    IPAddress ip = WiFi.localIP();
    snprintf(buffer, bufferSize, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    */
#endif
    
    // For testing
    snprintf(buffer, bufferSize, "192.168.1.100");
    
    return strlen(buffer);
}

/**
 * @brief Get system status as JSON
 */
int MCP_SystemGetStatus(char* buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // Get system uptime
    uint32_t uptime = HAL_ArduinoMillis() - s_start_time;
    
    // Format JSON status
    int pos = 0;
    pos += snprintf(buffer + pos, bufferSize - pos,
            "{"
            "\"status\":\"running\","
            "\"device\":\"%s\","
            "\"firmware\":\"%s\","
            "\"uptime_ms\":%u,",
            s_arduino_config.deviceName,
            s_arduino_config.version,
            uptime);
    
    // WiFi status
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"wifi_enabled\":%s,"
            "\"wifi_connected\":%s,",
            s_arduino_config.enableWifi ? "true" : "false",
            s_wifi_connected ? "true" : "false");
    
    // Server status
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"server_enabled\":%s,"
            "\"server_running\":%s,"
            "\"server_port\":%u,",
            s_arduino_config.enableServer ? "true" : "false",
            s_server_running ? "true" : "false",
            s_arduino_config.serverPort);
    
    // Platform-specific information
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"platform\":\"arduino\","
            "\"analog_reference\":%d,"
            "\"watchdog_enabled\":%s,",
            s_arduino_config.analogReference,
            s_arduino_config.enableWatchdog ? "true" : "false");
    
    // Debug status
    pos += snprintf(buffer + pos, bufferSize - pos,
            "\"debug_enabled\":%s"
            "}",
            s_arduino_config.enableDebug ? "true" : "false");
    
    return pos;
}

/**
 * @brief Set debug output enable state
 */
int MCP_SystemSetDebug(bool enable) {
    bool previous = s_arduino_config.enableDebug;
    s_arduino_config.enableDebug = enable;
    
    // Update log level
    log_set_level(enable ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO);
    
    // Save the updated configuration
    MCP_SavePersistentState();
    
    return previous;
}

/**
 * @brief Print debug message (if debug enabled)
 */
void MCP_SystemDebugPrint(const char* format, ...) {
    if (!s_arduino_config.enableDebug) {
        return;
    }
    
    // For Arduino, use Serial if available
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific debug print
    /*
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial.print(buffer);
    va_end(args);
    */
#else
    // For non-Arduino platforms, use standard printf
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}
