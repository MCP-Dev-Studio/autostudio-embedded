/**
 * @file platform_config.c
 * @brief Implementation of platform-specific configuration mappings
 */
#include "platform_config.h"
#include <string.h>

/**
 * @brief Convert platform-specific config to common config
 */
int PlatformConfigToCommonConfig(void* platform_config, MCP_CommonConfig* common_config) {
    if (platform_config == NULL || common_config == NULL) {
        return -1;
    }
    
    memset(common_config, 0, sizeof(MCP_CommonConfig));
    
#if defined(MCP_PLATFORM_RPI)
    MCP_RPiConfig* rpi_config = (MCP_RPiConfig*)platform_config;
    
    // Device information
    if (rpi_config->deviceName != NULL) {
        strncpy(common_config->device_name, rpi_config->deviceName, sizeof(common_config->device_name) - 1);
    }
    if (rpi_config->version != NULL) {
        strncpy(common_config->firmware_version, rpi_config->version, sizeof(common_config->firmware_version) - 1);
    }
    common_config->debug_enabled = rpi_config->enableDebug;
    
    // Server configuration
    common_config->server_enabled = rpi_config->enableServer;
    common_config->server_port = rpi_config->serverPort;
    common_config->auto_start_server = rpi_config->autoStartServer;
    
    // Network - WiFi configuration
    common_config->network.enabled = rpi_config->enableWifi;
    if (rpi_config->ssid != NULL) {
        strncpy(common_config->network.ssid, rpi_config->ssid, sizeof(common_config->network.ssid) - 1);
    }
    if (rpi_config->password != NULL) {
        strncpy(common_config->network.password, rpi_config->password, sizeof(common_config->network.password) - 1);
    }
    common_config->network.auto_connect = rpi_config->enableWifi; // Default to true if WiFi enabled
    
    // Network - WiFi AP configuration
    common_config->network.ap.enabled = rpi_config->enableHotspot;
    if (rpi_config->hotspotSsid != NULL) {
        strncpy(common_config->network.ap.ssid, rpi_config->hotspotSsid, sizeof(common_config->network.ap.ssid) - 1);
    }
    if (rpi_config->hotspotPassword != NULL) {
        strncpy(common_config->network.ap.password, rpi_config->hotspotPassword, sizeof(common_config->network.ap.password) - 1);
    }
    common_config->network.ap.channel = rpi_config->hotspotChannel;
    
    // Network - BLE configuration
    common_config->network.ble.enabled = rpi_config->enableBLE;
    if (rpi_config->deviceName != NULL) {
        strncpy(common_config->network.ble.device_name, rpi_config->deviceName, sizeof(common_config->network.ble.device_name) - 1);
    }
    
    // Network - Ethernet configuration
    common_config->network.ethernet.enabled = rpi_config->enableEthernet;
    if (rpi_config->ethernetInterface != NULL) {
        strncpy(common_config->network.ethernet.interface, rpi_config->ethernetInterface, sizeof(common_config->network.ethernet.interface) - 1);
    }
    common_config->network.ethernet.dhcp = true; // Default to DHCP
    
    // Interfaces
    common_config->interfaces.i2c.enabled = rpi_config->enableI2C;
    common_config->interfaces.i2c.bus_number = rpi_config->i2cBusNumber;
    common_config->interfaces.spi.enabled = rpi_config->enableSPI;
    common_config->interfaces.spi.bus_number = rpi_config->spiBusNumber;
    common_config->interfaces.uart.enabled = rpi_config->enableUART;
    common_config->interfaces.uart.number = rpi_config->uartNumber;
    common_config->interfaces.uart.baud_rate = rpi_config->uartBaudRate;
    common_config->interfaces.gpio.enabled = rpi_config->enableGPIO;
    
    // System
    common_config->heap_size = rpi_config->heapSize;
    if (rpi_config->configFile != NULL) {
        strncpy(common_config->config_file_path, rpi_config->configFile, sizeof(common_config->config_file_path) - 1);
    }
    common_config->enable_persistence = rpi_config->enablePersistence;
    
#elif defined(MCP_PLATFORM_ESP32)
    MCP_ESP32Config* esp32_config = (MCP_ESP32Config*)platform_config;
    
    // Device information
    if (esp32_config->deviceName != NULL) {
        strncpy(common_config->device_name, esp32_config->deviceName, sizeof(common_config->device_name) - 1);
    }
    if (esp32_config->version != NULL) {
        strncpy(common_config->firmware_version, esp32_config->version, sizeof(common_config->firmware_version) - 1);
    }
    common_config->debug_enabled = esp32_config->enableDebug;
    
    // Server configuration
    common_config->server_enabled = esp32_config->enableServer;
    common_config->server_port = esp32_config->serverPort;
    common_config->auto_start_server = esp32_config->autoStartServer;
    
    // Network - WiFi configuration
    common_config->network.enabled = esp32_config->enableWifi;
    if (esp32_config->ssid != NULL) {
        strncpy(common_config->network.ssid, esp32_config->ssid, sizeof(common_config->network.ssid) - 1);
    }
    if (esp32_config->password != NULL) {
        strncpy(common_config->network.password, esp32_config->password, sizeof(common_config->network.password) - 1);
    }
    common_config->network.auto_connect = esp32_config->enableWifi; // Default to true if WiFi enabled
    
    // Network - BLE configuration
    common_config->network.ble.enabled = esp32_config->enableBLE;
    if (esp32_config->deviceName != NULL) {
        strncpy(common_config->network.ble.device_name, esp32_config->deviceName, sizeof(common_config->network.ble.device_name) - 1);
    }
    
    // System
    common_config->heap_size = esp32_config->heapSize;
    if (esp32_config->configFile != NULL) {
        strncpy(common_config->config_file_path, esp32_config->configFile, sizeof(common_config->config_file_path) - 1);
    }
    common_config->enable_persistence = esp32_config->enablePersistence;
    
#elif defined(MCP_PLATFORM_ARDUINO)
    MCP_ArduinoConfig* arduino_config = (MCP_ArduinoConfig*)platform_config;
    
    // Device information
    if (arduino_config->deviceName != NULL) {
        strncpy(common_config->device_name, arduino_config->deviceName, sizeof(common_config->device_name) - 1);
    }
    if (arduino_config->version != NULL) {
        strncpy(common_config->firmware_version, arduino_config->version, sizeof(common_config->firmware_version) - 1);
    }
    common_config->debug_enabled = arduino_config->enableDebug;
    
    // Server configuration
    common_config->server_enabled = arduino_config->enableServer;
    common_config->server_port = arduino_config->serverPort;
    common_config->auto_start_server = arduino_config->autoStartServer;
    
    // Network - WiFi configuration (for ESP8266/ESP32 based Arduino)
    common_config->network.enabled = arduino_config->enableWifi;
    if (arduino_config->ssid != NULL) {
        strncpy(common_config->network.ssid, arduino_config->ssid, sizeof(common_config->network.ssid) - 1);
    }
    if (arduino_config->password != NULL) {
        strncpy(common_config->network.password, arduino_config->password, sizeof(common_config->network.password) - 1);
    }
    common_config->network.auto_connect = arduino_config->enableWifi; // Default to true if WiFi enabled
    
    // System
    common_config->heap_size = arduino_config->heapSize;
    if (arduino_config->configFile != NULL) {
        strncpy(common_config->config_file_path, arduino_config->configFile, sizeof(common_config->config_file_path) - 1);
    }
    common_config->enable_persistence = arduino_config->enablePersistence;
    
#elif defined(MCP_PLATFORM_MBED)
    MCP_MbedConfig* mbed_config = (MCP_MbedConfig*)platform_config;
    
    // Device information
    if (mbed_config->deviceName != NULL) {
        strncpy(common_config->device_name, mbed_config->deviceName, sizeof(common_config->device_name) - 1);
    }
    if (mbed_config->version != NULL) {
        strncpy(common_config->firmware_version, mbed_config->version, sizeof(common_config->firmware_version) - 1);
    }
    common_config->debug_enabled = mbed_config->enableDebug;
    
    // Server configuration
    common_config->server_enabled = mbed_config->enableServer;
    common_config->server_port = mbed_config->serverPort;
    common_config->auto_start_server = mbed_config->autoStartServer;
    
    // System
    common_config->heap_size = mbed_config->heapSize;
    if (mbed_config->configFile != NULL) {
        strncpy(common_config->config_file_path, mbed_config->configFile, sizeof(common_config->config_file_path) - 1);
    }
    common_config->enable_persistence = mbed_config->enablePersistence;
#endif
    
    return 0;
}

/**
 * @brief Fill platform-specific config from common config
 */
int CommonConfigToPlatformConfig(MCP_CommonConfig* common_config, void* platform_config) {
    if (platform_config == NULL || common_config == NULL) {
        return -1;
    }
    
#if defined(MCP_PLATFORM_RPI)
    MCP_RPiConfig* rpi_config = (MCP_RPiConfig*)platform_config;
    
    // Device information
    rpi_config->deviceName = common_config->device_name;
    rpi_config->version = common_config->firmware_version;
    rpi_config->enableDebug = common_config->debug_enabled;
    
    // Server configuration
    rpi_config->enableServer = common_config->server_enabled;
    rpi_config->serverPort = common_config->server_port;
    rpi_config->autoStartServer = common_config->auto_start_server;
    
    // Network - WiFi configuration
    rpi_config->enableWifi = common_config->network.enabled;
    rpi_config->ssid = common_config->network.ssid;
    rpi_config->password = common_config->network.password;
    
    // Network - WiFi AP configuration
    rpi_config->enableHotspot = common_config->network.ap.enabled;
    rpi_config->hotspotSsid = common_config->network.ap.ssid;
    rpi_config->hotspotPassword = common_config->network.ap.password;
    rpi_config->hotspotChannel = common_config->network.ap.channel;
    
    // Network - BLE configuration
    rpi_config->enableBLE = common_config->network.ble.enabled;
    
    // Network - Ethernet configuration
    rpi_config->enableEthernet = common_config->network.ethernet.enabled;
    rpi_config->ethernetInterface = common_config->network.ethernet.interface;
    
    // Interfaces
    rpi_config->enableI2C = common_config->interfaces.i2c.enabled;
    rpi_config->i2cBusNumber = common_config->interfaces.i2c.bus_number;
    rpi_config->enableSPI = common_config->interfaces.spi.enabled;
    rpi_config->spiBusNumber = common_config->interfaces.spi.bus_number;
    rpi_config->enableUART = common_config->interfaces.uart.enabled;
    rpi_config->uartNumber = common_config->interfaces.uart.number;
    rpi_config->uartBaudRate = common_config->interfaces.uart.baud_rate;
    rpi_config->enableGPIO = common_config->interfaces.gpio.enabled;
    
    // System
    rpi_config->heapSize = common_config->heap_size;
    rpi_config->configFile = common_config->config_file_path;
    rpi_config->enablePersistence = common_config->enable_persistence;
    
#elif defined(MCP_PLATFORM_ESP32)
    MCP_ESP32Config* esp32_config = (MCP_ESP32Config*)platform_config;
    
    // Device information
    esp32_config->deviceName = common_config->device_name;
    esp32_config->version = common_config->firmware_version;
    esp32_config->enableDebug = common_config->debug_enabled;
    
    // Server configuration
    esp32_config->enableServer = common_config->server_enabled;
    esp32_config->serverPort = common_config->server_port;
    esp32_config->autoStartServer = common_config->auto_start_server;
    
    // Network - WiFi configuration
    esp32_config->enableWifi = common_config->network.enabled;
    esp32_config->ssid = common_config->network.ssid;
    esp32_config->password = common_config->network.password;
    
    // Network - BLE configuration
    esp32_config->enableBLE = common_config->network.ble.enabled;
    
    // System
    esp32_config->heapSize = common_config->heap_size;
    esp32_config->configFile = common_config->config_file_path;
    esp32_config->enablePersistence = common_config->enable_persistence;
    
#elif defined(MCP_PLATFORM_ARDUINO)
    MCP_ArduinoConfig* arduino_config = (MCP_ArduinoConfig*)platform_config;
    
    // Device information
    arduino_config->deviceName = common_config->device_name;
    arduino_config->version = common_config->firmware_version;
    arduino_config->enableDebug = common_config->debug_enabled;
    
    // Server configuration
    arduino_config->enableServer = common_config->server_enabled;
    arduino_config->serverPort = common_config->server_port;
    arduino_config->autoStartServer = common_config->auto_start_server;
    
    // Network - WiFi configuration (for ESP8266/ESP32 based Arduino)
    arduino_config->enableWifi = common_config->network.enabled;
    arduino_config->ssid = common_config->network.ssid;
    arduino_config->password = common_config->network.password;
    
    // System
    arduino_config->heapSize = common_config->heap_size;
    arduino_config->configFile = common_config->config_file_path;
    arduino_config->enablePersistence = common_config->enable_persistence;
    
#elif defined(MCP_PLATFORM_MBED)
    MCP_MbedConfig* mbed_config = (MCP_MbedConfig*)platform_config;
    
    // Device information
    mbed_config->deviceName = common_config->device_name;
    mbed_config->version = common_config->firmware_version;
    mbed_config->enableDebug = common_config->debug_enabled;
    
    // Server configuration
    mbed_config->enableServer = common_config->server_enabled;
    mbed_config->serverPort = common_config->server_port;
    mbed_config->autoStartServer = common_config->auto_start_server;
    
    // System
    mbed_config->heapSize = common_config->heap_size;
    mbed_config->configFile = common_config->config_file_path;
    mbed_config->enablePersistence = common_config->enable_persistence;
#endif
    
    return 0;
}