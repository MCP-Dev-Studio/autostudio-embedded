#ifdef MCP_OS_MBED
#include "mbed.h"
#endif

#include "system/mcp_os_core.h"

#ifdef MCP_OS_ARDUINO
#include <Arduino.h>

// Define Arduino platform
#define MCP_PLATFORM_ARDUINO
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
int main(void)
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
        MCP_SystemProcess();
        
        // Short delay - replaced ESP32 specific code with generic delay
        // vTaskDelay(pdMS_TO_TICKS(10));
        // Use a platform-independent delay approach
        for (volatile int i = 0; i < 1000000; i++) {}
    }
    
    return 0; // Never reached, but needed for compiler
}
#elif defined(MCP_OS_RPI)
int main(void)
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
    
    LOG_INFO("Main", "MCP OS v%s starting on Raspberry Pi...", MCP_OS_VERSION_STR);
    
    // Configure system
    MCP_RPiConfig sysConfig;
    sysConfig.deviceName = "MCP OS Raspberry Pi";
    sysConfig.version = MCP_OS_VERSION_STR;
    sysConfig.enableDebug = true;
    sysConfig.enablePersistence = true;
    sysConfig.heapSize = MCP_OS_MEMORY_SIZE;
    sysConfig.configFile = "/etc/mcp/mcp_config.json";
    sysConfig.enableServer = true;
    sysConfig.serverPort = 5555;
    sysConfig.autoStartServer = true;
    sysConfig.enableI2C = true;
    sysConfig.i2cBusNumber = 1;  // Default I2C bus on most Raspberry Pi models
    sysConfig.enableSPI = true;
    sysConfig.spiBusNumber = 0;  // Default SPI bus
    sysConfig.enableUART = true;
    sysConfig.uartNumber = 0;    // Primary UART
    sysConfig.uartBaudRate = 115200;
    sysConfig.enableGPIO = true;
    sysConfig.enableWifi = true;
    sysConfig.ssid = "YourWiFiSSID";          // Can be changed later via MCP_WiFiConnect
    sysConfig.password = "YourWiFiPassword";  // Can be changed later via MCP_WiFiConnect
    sysConfig.enableBLE = false;              // Set to true to enable Bluetooth
    sysConfig.enableEthernet = true;
    sysConfig.ethernetInterface = "eth0";     // Default Ethernet interface
    sysConfig.enableHotspot = false;          // Set to true to enable WiFi hotspot mode
    sysConfig.hotspotSsid = "MCPHotspot";     // Default hotspot SSID
    sysConfig.hotspotPassword = "mcppassword"; // Default hotspot password
    sysConfig.hotspotChannel = 6;             // Default hotspot channel
    
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
        usleep(10000); // 10 ms delay
    }
    
    return 0; // Never reached, but needed for compiler
}
#elif defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Forward declarations with explicit C linkage
extern "C" {
    int MCP_LoadPersistentState(void);
    int MCP_ServerStart(void);
    int MCP_SystemProcess(uint32_t timeout);
    int MCP_LoggingToolRegister(void);
}

// Global flag to control main loop
static volatile bool g_running = true;

// Signal handler for graceful shutdown
static void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    g_running = false;
}

int main(void)
{
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("MCP OS v%s starting on HOST platform...\n", MCP_OS_VERSION_STR);
    
    // Initialize logging for HOST platform
    printf("Initializing logging system\n");
    
    // Configure HOST platform
    printf("Configuring HOST platform\n");
    MCP_HostConfig sysConfig;
    sysConfig.port = 5555;
    strncpy(sysConfig.host, "localhost", sizeof(sysConfig.host)-1);
    sysConfig.baudRate = 115200;
    strncpy(sysConfig.logFile, "mcp_host.log", sizeof(sysConfig.logFile)-1);
    sysConfig.logLevel = 3; // INFO
    sysConfig.useTCP = true;
    
    // Print configuration
    printf("HOST Configuration:\n");
    printf("- Port: %d\n", sysConfig.port);
    printf("- Host: %s\n", sysConfig.host);
    printf("- Log Level: %d\n", sysConfig.logLevel);
    printf("- Transport: %s\n", sysConfig.useTCP ? "TCP" : "Serial");
    
    // Load persistent state
    printf("Loading persistent state\n");
    int result = MCP_LoadPersistentState();
    if (result != 0) {
        printf("Warning: Failed to load persistent state: %d\n", result);
    }
    
    // Start server
    printf("Starting MCP server\n");
    result = MCP_ServerStart();
    if (result != 0) {
        printf("Error: Server start failed: %d\n", result);
        return result;
    }
    
    printf("HOST system initialized, entering main loop\n");
    printf("Press Ctrl+C to exit\n");
    
    // Register logging tool with the tool registry
    extern int MCP_LoggingToolRegister(void);
    MCP_LoggingToolRegister();
    
    // Main loop
    while (g_running) {
        // Process system tasks
        MCP_SystemProcess(100);
        
        // Short delay
        usleep(100000); // 100 ms delay
    }
    
    printf("HOST system shutting down cleanly\n");
    return 0;
}
#else
int main() {
    // Platform not defined
    return -1;
}
#endif