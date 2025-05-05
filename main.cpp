#ifdef MCP_OS_MBED
#include "mbed.h"
#endif

#include "mcp_os_core.h"

#ifdef MCP_OS_ARDUINO
#include <Arduino.h>
#endif

/**
 * Main application entry point for MCP OS
 */
#if defined(MCP_OS_MBED)
int main() {
    // Initialize system logging
    LogConfig logConfig;
    logConfig.level = LOG_LEVEL_INFO;
    logConfig.outputs = LOG_OUTPUT_SERIAL;
    logConfig.logFileName = NULL;
    logConfig.maxFileSize = 0;
    logConfig.maxMemoryEntries = 100;
    logConfig.includeTimestamp = true;
    logConfig.includeLevelName = true;
    logConfig.includeModuleName = true;
    logConfig.colorOutput = true;
    logConfig.customLogCallback = NULL;
    
    log_init(&logConfig);
    
    LOG_INFO("Main", "MCP OS v%s starting...", MCP_OS_VERSION_STR);
    
    // Configure system
    MCP_MbedConfig sysConfig;
    sysConfig.deviceName = "MCP OS Device";
    sysConfig.version = MCP_OS_VERSION_STR;
    sysConfig.enableDebug = true;
    sysConfig.enablePersistence = true;
    sysConfig.heapSize = MCP_OS_MEMORY_SIZE;
    sysConfig.configFile = "mcp_config.json";
    sysConfig.enableServer = true;
    sysConfig.serverPort = 5555;
    sysConfig.autoStartServer = true;
    
    // System initialization
    int result = MCP_SystemInit(&sysConfig);
    if (result != 0) {
        LOG_ERROR("Main", "System initialization failed: %d", result);
        return result;
    }
    
    // Load persistent state
    result = MCP_LoadPersistentState();
    if (result != 0) {
        LOG_WARN("Main", "Failed to load persistent state: %d", result);
    }
    
    // Start server
    result = MCP_ServerStart();
    if (result != 0) {
        LOG_ERROR("Main", "Server start failed: %d", result);
        return result;
    }
    
    LOG_INFO("Main", "System initialized, entering main loop");
    
    // Main loop
    while (true) {
        // Process system tasks
        MCP_SystemProcess(10);
        
        // Short delay
        ThisThread::sleep_for(10ms);
    }
}
#elif defined(MCP_OS_ARDUINO)
void setup() {
    // Initialize serial for debug output
    Serial.begin(115200);
    while (!Serial && millis() < 5000);
    
    // Initialize system logging
    LogConfig logConfig;
    logConfig.level = LOG_LEVEL_INFO;
    logConfig.outputs = LOG_OUTPUT_SERIAL;
    logConfig.logFileName = NULL;
    logConfig.maxFileSize = 0;
    logConfig.maxMemoryEntries = 50;
    logConfig.includeTimestamp = true;
    logConfig.includeLevelName = true;
    logConfig.includeModuleName = true;
    logConfig.colorOutput = true;
    logConfig.customLogCallback = NULL;
    
    log_init(&logConfig);
    
    LOG_INFO("Main", "MCP OS v%s starting...", MCP_OS_VERSION_STR);
    
    // Configure system
    MCP_ArduinoConfig sysConfig;
    sysConfig.deviceName = "MCP OS Arduino";
    sysConfig.version = MCP_OS_VERSION_STR;
    sysConfig.enableDebug = true;
    sysConfig.enablePersistence = true;
    sysConfig.heapSize = MCP_OS_MEMORY_SIZE;
    sysConfig.configFile = "/mcp_config.json";
    sysConfig.enableServer = true;
    
    #if defined(ESP8266) || defined(ESP32)
    sysConfig.enableWifi = true;
    sysConfig.ssid = "YourWiFiSSID";
    sysConfig.password = "YourWiFiPassword";
    #else
    sysConfig.enableWifi = false;
    #endif
    
    sysConfig.serverPort = 5555;
    sysConfig.autoStartServer = true;
    
    // System initialization
    int result = MCP_SystemInit(&sysConfig);
    if (result != 0) {
        LOG_ERROR("Main", "System initialization failed: %d", result);
        return;
    }
    
    // Load persistent state
    result = MCP_LoadPersistentState();
    if (result != 0) {
        LOG_WARN("Main", "Failed to load persistent state: %d", result);
    }
    
    // Start server
    result = MCP_ServerStart();
    if (result != 0) {
        LOG_ERROR("Main", "Server start failed: %d", result);
        return;
    }
    
    LOG_INFO("Main", "System initialized");
}

void loop() {
    // Process system tasks
    MCP_SystemProcess();
    
    // Small delay for stability
    delay(10);
}
#elif defined(MCP_OS_ESP32)
extern "C" void app_main(void)
{
    // Initialize system logging
    LogConfig logConfig;
    logConfig.level = LOG_LEVEL_INFO;
    logConfig.outputs = LOG_OUTPUT_SERIAL;
    logConfig.logFileName = NULL;
    logConfig.maxFileSize = 0;
    logConfig.maxMemoryEntries = 100;
    logConfig.includeTimestamp = true;
    logConfig.includeLevelName = true;
    logConfig.includeModuleName = true;
    logConfig.colorOutput = true;
    logConfig.customLogCallback = NULL;
    
    log_init(&logConfig);
    
    LOG_INFO("Main", "MCP OS v%s starting...", MCP_OS_VERSION_STR);
    
    // Configure system
    MCP_ESP32Config sysConfig;
    sysConfig.deviceName = "MCP OS ESP32";
    sysConfig.version = MCP_OS_VERSION_STR;
    sysConfig.enableDebug = true;
    sysConfig.enablePersistence = true;
    sysConfig.heapSize = MCP_OS_MEMORY_SIZE;
    sysConfig.configFile = "/spiffs/mcp_config.json";
    sysConfig.enableServer = true;
    sysConfig.enableWifi = true;
    sysConfig.ssid = "YourWiFiSSID";
    sysConfig.password = "YourWiFiPassword";
    sysConfig.enableBLE = false;
    sysConfig.serverPort = 5555;
    sysConfig.autoStartServer = true;
    sysConfig.enableOTA = true;
    sysConfig.enableWebServer = true;
    sysConfig.webServerPort = 80;
    
    // System initialization
    int result = MCP_SystemInit(&sysConfig);
    if (result != 0) {
        LOG_ERROR("Main", "System initialization failed: %d", result);
        return;
    }
    
    // Load persistent state
    result = MCP_LoadPersistentState();
    if (result != 0) {
        LOG_WARN("Main", "Failed to load persistent state: %d", result);
    }
    
    // Start server
    result = MCP_ServerStart();
    if (result != 0) {
        LOG_ERROR("Main", "Server start failed: %d", result);
        return;
    }
    
    LOG_INFO("Main", "System initialized, entering main loop");
    
    // Main loop
    while (true) {
        // Process system tasks
        MCP_SystemProcess();
        
        // Short delay
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#else
int main() {
    // Platform not defined
    return -1;
}
#endif