/**
 * @file mcp_config.c
 * @brief Implementation of MCP configuration system
 */
#include "mcp_config.h"
#include "../../../json/json_helpers.h"
#include "../../../system/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Include the default config JSON as a string constant for Arduino
#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
#include "config_json.h"
#endif

// Define simplified logging functions for the host platform
// These will be used when the actual logging system isn't fully initialized
#if defined(MCP_PLATFORM_HOST)
#define CONFIG_MODULE "CONFIG"

#elif defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
#define CONFIG_MODULE "CONFIG"
// Arduino config path definition
static const char* s_default_config_path = "/config.json";

// Function to get default config JSON for Arduino
const char* MCP_GetDefaultConfigJson(void) {
    return DEFAULT_CONFIG_JSON;
}
void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[INFO][%s] ", CONFIG_MODULE);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[WARN][%s] ", CONFIG_MODULE);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[ERROR][%s] ", CONFIG_MODULE);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
#else
// For non-host platforms, use the proper logging macros
#define log_info(format, ...) LOG_INFO(CONFIG_MODULE, format, ##__VA_ARGS__)
#define log_warn(format, ...) LOG_WARN(CONFIG_MODULE, format, ##__VA_ARGS__)
#define log_error(format, ...) LOG_ERROR(CONFIG_MODULE, format, ##__VA_ARGS__)
#define CONFIG_MODULE "CONFIG"
#endif

// Global configuration instance for the current platform
static MCP_PLATFORM_CONFIG s_config;
static bool s_config_initialized = false;
static char s_default_config_path[256] = "/etc/mcp/config.json";

/**
 * @brief Initialize configuration with default values
 */
int MCP_ConfigInit(MCP_PLATFORM_CONFIG* config) {
    if (config == NULL) {
        return -1;
    }
    
    // Initialize common config with defaults
    memset(config, 0, sizeof(MCP_PLATFORM_CONFIG));
    
    // Device information
    strncpy(config->common.device_name, "MCP Device", sizeof(config->common.device_name) - 1);
    strncpy(config->common.firmware_version, "1.0.0", sizeof(config->common.firmware_version) - 1);
    config->common.debug_enabled = false;
    
    // Server configuration
    config->common.server_enabled = true;
    config->common.server_port = 8080;
    config->common.auto_start_server = true;
    
    // Network - WiFi configuration
    config->common.network.enabled = false;
    config->common.network.auto_connect = false;
    
    // Network - WiFi AP configuration
    config->common.network.ap.enabled = false;
    config->common.network.ap.channel = 6;
    
    // Network - BLE configuration
    config->common.network.ble.enabled = false;
    strncpy(config->common.network.ble.device_name, "MCP Device", 
           sizeof(config->common.network.ble.device_name) - 1);
    config->common.network.ble.auto_advertise = false;
    
    // Network - Ethernet configuration
    config->common.network.ethernet.enabled = true;
    strncpy(config->common.network.ethernet.interface, "eth0", 
           sizeof(config->common.network.ethernet.interface) - 1);
    config->common.network.ethernet.dhcp = true;
    
    // Interfaces - I2C configuration
    config->common.interfaces.i2c.enabled = false;
    config->common.interfaces.i2c.bus_number = 1;
    
    // Interfaces - SPI configuration
    config->common.interfaces.spi.enabled = false;
    config->common.interfaces.spi.bus_number = 0;
    
    // Interfaces - UART configuration
    config->common.interfaces.uart.enabled = false;
    config->common.interfaces.uart.number = 0;
    config->common.interfaces.uart.baud_rate = 115200;
    
    // Interfaces - GPIO configuration
    config->common.interfaces.gpio.enabled = true;
    
    // System configurations
    config->common.heap_size = 1024 * 1024; // 1MB heap by default
    
    // Set config file path based on platform
    #if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
    printf("Using embedded default configuration for Arduino\n");
    strncpy(config->common.config_file_path, "embedded", 
           sizeof(config->common.config_file_path) - 1);
    #else
    strncpy(config->common.config_file_path, s_default_config_path, 
           sizeof(config->common.config_file_path) - 1);
    #endif
    
    config->common.enable_persistence = true;
    
    // Platform-specific configurations
#if defined(MCP_PLATFORM_RPI)
    config->enable_camera = false;
    config->camera_resolution = 720;
    
#elif defined(MCP_PLATFORM_ESP32)
    config->enable_ota = true;
    config->enable_web_server = true;
    config->web_server_port = 80;
    config->enable_deep_sleep = false;
    config->deep_sleep_time_ms = 0;
    
#elif defined(MCP_PLATFORM_ARDUINO)
    config->analog_reference = 0; // DEFAULT
    config->enable_watchdog = false;
    
#elif defined(MCP_PLATFORM_MBED)
    config->enable_rtos = true;
    config->task_stack_size = 4096;

#elif defined(MCP_PLATFORM_HOST)
    config->enable_mock_hardware = true;
    config->test_mode = 0;
#endif
    
    s_config_initialized = true;
    return 0;
}

/**
 * @brief Load configuration from persistent storage
 */
int MCP_ConfigLoad(MCP_PLATFORM_CONFIG* config, const char* file_path) {
    if (config == NULL) {
        return -1;
    }
    
    // Use default path if not specified
    if (file_path == NULL) {
        file_path = config->common.config_file_path;
    }
    
    log_info("Loading configuration from %s", file_path);
    
    // Initialize with defaults first
    MCP_ConfigInit(config);
    
    // Open config file
    FILE* fp = fopen(file_path, "r");
    if (fp == NULL) {
        log_warn("Configuration file not found, using defaults");
        return 0; // Not an error, just use defaults
    }
    
    // Read file content
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0) {
        log_warn("Empty configuration file, using defaults");
        fclose(fp);
        return 0;
    }
    
    char* json_buffer = (char*)malloc(file_size + 1);
    if (json_buffer == NULL) {
        log_error("Failed to allocate memory for configuration file");
        fclose(fp);
        return -2;
    }
    
    size_t bytes_read = fread(json_buffer, 1, file_size, fp);
    fclose(fp);
    
    if (bytes_read != (size_t)file_size) {
        log_error("Failed to read configuration file");
        free(json_buffer);
        return -3;
    }
    
    // Null-terminate the string
    json_buffer[file_size] = '\0';
    
    // Parse JSON and update config
    int result = MCP_ConfigUpdateFromJSON(config, json_buffer);
    free(json_buffer);
    
    if (result != 0) {
        log_error("Failed to parse configuration JSON: %d", result);
        return -4;
    }
    
    log_info("Configuration loaded successfully");
    return 0;
}

/**
 * @brief Save configuration to persistent storage
 */
int MCP_ConfigSave(const MCP_PLATFORM_CONFIG* config, const char* file_path) {
    if (config == NULL) {
        return -1;
    }
    
    if (!config->common.enable_persistence) {
        log_info("Persistence disabled, not saving configuration");
        return 0;
    }
    
    // Use default path if not specified
    if (file_path == NULL) {
        file_path = config->common.config_file_path;
    }
    
    log_info("Saving configuration to %s", file_path);
    
    // Serialize to JSON
    char* json_buffer = (char*)malloc(16384); // 16KB buffer for config
    if (json_buffer == NULL) {
        log_error("Failed to allocate memory for configuration JSON");
        return -2;
    }
    
    int json_length = MCP_ConfigSerializeToJSON(config, json_buffer, 16384);
    if (json_length <= 0) {
        log_error("Failed to serialize configuration to JSON: %d", json_length);
        free(json_buffer);
        return -3;
    }
    
    // Create directory if it doesn't exist
    char dir_path[256];
    strncpy(dir_path, file_path, sizeof(dir_path) - 1);
    
    // Find the last slash in the path
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0'; // Truncate the string at the last slash
        
        // Create the directory structure
        // This is a simplified version - a real implementation would create
        // each directory in the path if it doesn't exist
        #if defined(_WIN32)
        system("mkdir -p");
        #else
        char mkdir_cmd[300];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", dir_path);
        system(mkdir_cmd);
        #endif
    }
    
    // Open and write file
    FILE* fp = fopen(file_path, "w");
    if (fp == NULL) {
        log_error("Failed to open configuration file for writing");
        free(json_buffer);
        return -4;
    }
    
    size_t bytes_written = fwrite(json_buffer, 1, json_length, fp);
    fclose(fp);
    
    free(json_buffer);
    
    if (bytes_written != (size_t)json_length) {
        log_error("Failed to write configuration file: %zu of %d bytes written", 
                  bytes_written, json_length);
        return -5;
    }
    
    log_info("Configuration saved successfully");
    return 0;
}

/**
 * @brief Update a string field in the configuration
 */
static void updateStringField(char* dest, size_t dest_size, const char* src) {
    if (src != NULL) {
        memset(dest, 0, dest_size);
        strncpy(dest, src, dest_size - 1);
    }
}

/**
 * @brief Update configuration from JSON string
 */
int MCP_ConfigUpdateFromJSON(MCP_PLATFORM_CONFIG* config, const char* json_string) {
    if (config == NULL || json_string == NULL) {
        return -1;
    }
    
    // Device information
    char* device_name = json_get_string_field(json_string, "device.name");
    if (device_name != NULL) {
        updateStringField(config->common.device_name, sizeof(config->common.device_name), device_name);
        free(device_name);
    }
    
    char* firmware_version = json_get_string_field(json_string, "device.firmware_version");
    if (firmware_version != NULL) {
        updateStringField(config->common.firmware_version, sizeof(config->common.firmware_version), firmware_version);
        free(firmware_version);
    }
    
    config->common.debug_enabled = json_get_bool_field(json_string, "device.debug_enabled", config->common.debug_enabled);
    
    // Server configuration
    config->common.server_enabled = json_get_bool_field(json_string, "server.enabled", config->common.server_enabled);
    config->common.server_port = json_get_int_field(json_string, "server.port", config->common.server_port);
    config->common.auto_start_server = json_get_bool_field(json_string, "server.auto_start", config->common.auto_start_server);
    
    // Network - WiFi configuration
    config->common.network.enabled = json_get_bool_field(json_string, "network.wifi.enabled", config->common.network.enabled);
    
    char* wifi_ssid = json_get_string_field(json_string, "network.wifi.ssid");
    if (wifi_ssid != NULL) {
        updateStringField(config->common.network.ssid, sizeof(config->common.network.ssid), wifi_ssid);
        free(wifi_ssid);
    }
    
    char* wifi_password = json_get_string_field(json_string, "network.wifi.password");
    if (wifi_password != NULL) {
        updateStringField(config->common.network.password, sizeof(config->common.network.password), wifi_password);
        free(wifi_password);
    }
    
    config->common.network.auto_connect = json_get_bool_field(json_string, "network.wifi.auto_connect", config->common.network.auto_connect);
    
    // Network - WiFi AP configuration
    config->common.network.ap.enabled = json_get_bool_field(json_string, "network.wifi_ap.enabled", config->common.network.ap.enabled);
    
    char* ap_ssid = json_get_string_field(json_string, "network.wifi_ap.ssid");
    if (ap_ssid != NULL) {
        updateStringField(config->common.network.ap.ssid, sizeof(config->common.network.ap.ssid), ap_ssid);
        free(ap_ssid);
    }
    
    char* ap_password = json_get_string_field(json_string, "network.wifi_ap.password");
    if (ap_password != NULL) {
        updateStringField(config->common.network.ap.password, sizeof(config->common.network.ap.password), ap_password);
        free(ap_password);
    }
    
    config->common.network.ap.channel = json_get_int_field(json_string, "network.wifi_ap.channel", config->common.network.ap.channel);
    
    // Network - BLE configuration
    config->common.network.ble.enabled = json_get_bool_field(json_string, "network.ble.enabled", config->common.network.ble.enabled);
    
    char* ble_device_name = json_get_string_field(json_string, "network.ble.device_name");
    if (ble_device_name != NULL) {
        updateStringField(config->common.network.ble.device_name, sizeof(config->common.network.ble.device_name), ble_device_name);
        free(ble_device_name);
    }
    
    config->common.network.ble.auto_advertise = json_get_bool_field(json_string, "network.ble.auto_advertise", config->common.network.ble.auto_advertise);
    
    // Network - Ethernet configuration
    config->common.network.ethernet.enabled = json_get_bool_field(json_string, "network.ethernet.enabled", config->common.network.ethernet.enabled);
    
    char* ethernet_interface = json_get_string_field(json_string, "network.ethernet.interface");
    if (ethernet_interface != NULL) {
        updateStringField(config->common.network.ethernet.interface, sizeof(config->common.network.ethernet.interface), ethernet_interface);
        free(ethernet_interface);
    }
    
    config->common.network.ethernet.dhcp = json_get_bool_field(json_string, "network.ethernet.dhcp", config->common.network.ethernet.dhcp);
    
    char* ethernet_static_ip = json_get_string_field(json_string, "network.ethernet.static_ip");
    if (ethernet_static_ip != NULL) {
        updateStringField(config->common.network.ethernet.static_ip, sizeof(config->common.network.ethernet.static_ip), ethernet_static_ip);
        free(ethernet_static_ip);
    }
    
    char* ethernet_gateway = json_get_string_field(json_string, "network.ethernet.gateway");
    if (ethernet_gateway != NULL) {
        updateStringField(config->common.network.ethernet.gateway, sizeof(config->common.network.ethernet.gateway), ethernet_gateway);
        free(ethernet_gateway);
    }
    
    char* ethernet_subnet = json_get_string_field(json_string, "network.ethernet.subnet");
    if (ethernet_subnet != NULL) {
        updateStringField(config->common.network.ethernet.subnet, sizeof(config->common.network.ethernet.subnet), ethernet_subnet);
        free(ethernet_subnet);
    }
    
    // Interfaces - I2C configuration
    config->common.interfaces.i2c.enabled = json_get_bool_field(json_string, "interfaces.i2c.enabled", config->common.interfaces.i2c.enabled);
    config->common.interfaces.i2c.bus_number = json_get_int_field(json_string, "interfaces.i2c.bus_number", config->common.interfaces.i2c.bus_number);
    
    // Interfaces - SPI configuration
    config->common.interfaces.spi.enabled = json_get_bool_field(json_string, "interfaces.spi.enabled", config->common.interfaces.spi.enabled);
    config->common.interfaces.spi.bus_number = json_get_int_field(json_string, "interfaces.spi.bus_number", config->common.interfaces.spi.bus_number);
    
    // Interfaces - UART configuration
    config->common.interfaces.uart.enabled = json_get_bool_field(json_string, "interfaces.uart.enabled", config->common.interfaces.uart.enabled);
    config->common.interfaces.uart.number = json_get_int_field(json_string, "interfaces.uart.number", config->common.interfaces.uart.number);
    config->common.interfaces.uart.baud_rate = json_get_int_field(json_string, "interfaces.uart.baud_rate", config->common.interfaces.uart.baud_rate);
    
    // Interfaces - GPIO configuration
    config->common.interfaces.gpio.enabled = json_get_bool_field(json_string, "interfaces.gpio.enabled", config->common.interfaces.gpio.enabled);
    
    // System configurations
    config->common.heap_size = json_get_int_field(json_string, "system.heap_size", config->common.heap_size);
    
    char* config_file_path = json_get_string_field(json_string, "system.config_file_path");
    if (config_file_path != NULL) {
        updateStringField(config->common.config_file_path, sizeof(config->common.config_file_path), config_file_path);
        free(config_file_path);
    }
    
    config->common.enable_persistence = json_get_bool_field(json_string, "system.enable_persistence", config->common.enable_persistence);
    
    // Platform-specific configurations
#if defined(MCP_PLATFORM_RPI)
    config->enable_camera = json_get_bool_field(json_string, "platform.rpi.enable_camera", config->enable_camera);
    config->camera_resolution = json_get_int_field(json_string, "platform.rpi.camera_resolution", config->camera_resolution);
    
#elif defined(MCP_PLATFORM_ESP32)
    config->enable_ota = json_get_bool_field(json_string, "platform.esp32.enable_ota", config->enable_ota);
    config->enable_web_server = json_get_bool_field(json_string, "platform.esp32.enable_web_server", config->enable_web_server);
    config->web_server_port = json_get_int_field(json_string, "platform.esp32.web_server_port", config->web_server_port);
    config->enable_deep_sleep = json_get_bool_field(json_string, "platform.esp32.enable_deep_sleep", config->enable_deep_sleep);
    config->deep_sleep_time_ms = json_get_int_field(json_string, "platform.esp32.deep_sleep_time_ms", config->deep_sleep_time_ms);
    
#elif defined(MCP_PLATFORM_ARDUINO)
    config->analog_reference = json_get_int_field(json_string, "platform.arduino.analog_reference", config->analog_reference);
    config->enable_watchdog = json_get_bool_field(json_string, "platform.arduino.enable_watchdog", config->enable_watchdog);
    
#elif defined(MCP_PLATFORM_MBED)
    config->enable_rtos = json_get_bool_field(json_string, "platform.mbed.enable_rtos", config->enable_rtos);
    config->task_stack_size = json_get_int_field(json_string, "platform.mbed.task_stack_size", config->task_stack_size);

#elif defined(MCP_PLATFORM_HOST)
    config->enable_mock_hardware = json_get_bool_field(json_string, "platform.host.enable_mock_hardware", config->enable_mock_hardware);
    config->test_mode = json_get_int_field(json_string, "platform.host.test_mode", config->test_mode);
#endif
    
    return 0;
}

/**
 * @brief Serialize configuration to JSON string
 */
int MCP_ConfigSerializeToJSON(const MCP_PLATFORM_CONFIG* config, char* buffer, size_t size) {
    if (config == NULL || buffer == NULL || size == 0) {
        return -1;
    }
    
    int pos = 0;
    pos += snprintf(buffer + pos, size - pos, "{\n");
    
    // Device information
    pos += snprintf(buffer + pos, size - pos,
        "  \"device\": {\n"
        "    \"name\": \"%s\",\n"
        "    \"firmware_version\": \"%s\",\n"
        "    \"debug_enabled\": %s\n"
        "  },\n",
        config->common.device_name,
        config->common.firmware_version,
        config->common.debug_enabled ? "true" : "false"
    );
    
    // Server configuration
    pos += snprintf(buffer + pos, size - pos,
        "  \"server\": {\n"
        "    \"enabled\": %s,\n"
        "    \"port\": %u,\n"
        "    \"auto_start\": %s\n"
        "  },\n",
        config->common.server_enabled ? "true" : "false",
        config->common.server_port,
        config->common.auto_start_server ? "true" : "false"
    );
    
    // Network configuration
    pos += snprintf(buffer + pos, size - pos, "  \"network\": {\n");
    
    // WiFi configuration
    pos += snprintf(buffer + pos, size - pos,
        "    \"wifi\": {\n"
        "      \"enabled\": %s,\n"
        "      \"ssid\": \"%s\",\n"
        "      \"password\": \"%s\",\n"
        "      \"auto_connect\": %s\n"
        "    },\n",
        config->common.network.enabled ? "true" : "false",
        config->common.network.ssid,
        config->common.network.password,
        config->common.network.auto_connect ? "true" : "false"
    );
    
    // WiFi AP configuration
    pos += snprintf(buffer + pos, size - pos,
        "    \"wifi_ap\": {\n"
        "      \"enabled\": %s,\n"
        "      \"ssid\": \"%s\",\n"
        "      \"password\": \"%s\",\n"
        "      \"channel\": %u\n"
        "    },\n",
        config->common.network.ap.enabled ? "true" : "false",
        config->common.network.ap.ssid,
        config->common.network.ap.password,
        config->common.network.ap.channel
    );
    
    // BLE configuration
    pos += snprintf(buffer + pos, size - pos,
        "    \"ble\": {\n"
        "      \"enabled\": %s,\n"
        "      \"device_name\": \"%s\",\n"
        "      \"auto_advertise\": %s\n"
        "    },\n",
        config->common.network.ble.enabled ? "true" : "false",
        config->common.network.ble.device_name,
        config->common.network.ble.auto_advertise ? "true" : "false"
    );
    
    // Ethernet configuration
    pos += snprintf(buffer + pos, size - pos,
        "    \"ethernet\": {\n"
        "      \"enabled\": %s,\n"
        "      \"interface\": \"%s\",\n"
        "      \"dhcp\": %s,\n"
        "      \"static_ip\": \"%s\",\n"
        "      \"gateway\": \"%s\",\n"
        "      \"subnet\": \"%s\"\n"
        "    }\n"
        "  },\n",
        config->common.network.ethernet.enabled ? "true" : "false",
        config->common.network.ethernet.interface,
        config->common.network.ethernet.dhcp ? "true" : "false",
        config->common.network.ethernet.static_ip,
        config->common.network.ethernet.gateway,
        config->common.network.ethernet.subnet
    );
    
    // Interface configurations
    pos += snprintf(buffer + pos, size - pos, "  \"interfaces\": {\n");
    
    // I2C configuration
    pos += snprintf(buffer + pos, size - pos,
        "    \"i2c\": {\n"
        "      \"enabled\": %s,\n"
        "      \"bus_number\": %d\n"
        "    },\n",
        config->common.interfaces.i2c.enabled ? "true" : "false",
        config->common.interfaces.i2c.bus_number
    );
    
    // SPI configuration
    pos += snprintf(buffer + pos, size - pos,
        "    \"spi\": {\n"
        "      \"enabled\": %s,\n"
        "      \"bus_number\": %d\n"
        "    },\n",
        config->common.interfaces.spi.enabled ? "true" : "false",
        config->common.interfaces.spi.bus_number
    );
    
    // UART configuration
    pos += snprintf(buffer + pos, size - pos,
        "    \"uart\": {\n"
        "      \"enabled\": %s,\n"
        "      \"number\": %d,\n"
        "      \"baud_rate\": %u\n"
        "    },\n",
        config->common.interfaces.uart.enabled ? "true" : "false",
        config->common.interfaces.uart.number,
        config->common.interfaces.uart.baud_rate
    );
    
    // GPIO configuration
    pos += snprintf(buffer + pos, size - pos,
        "    \"gpio\": {\n"
        "      \"enabled\": %s\n"
        "    }\n"
        "  },\n",
        config->common.interfaces.gpio.enabled ? "true" : "false"
    );
    
    // System configurations
    pos += snprintf(buffer + pos, size - pos,
        "  \"system\": {\n"
        "    \"heap_size\": %u,\n"
        "    \"config_file_path\": \"%s\",\n"
        "    \"enable_persistence\": %s\n"
        "  }",
        config->common.heap_size,
        config->common.config_file_path,
        config->common.enable_persistence ? "true" : "false"
    );
    
    // Platform-specific configurations
#if defined(MCP_PLATFORM_RPI)
    pos += snprintf(buffer + pos, size - pos,
        ",\n"
        "  \"platform\": {\n"
        "    \"rpi\": {\n"
        "      \"enable_camera\": %s,\n"
        "      \"camera_resolution\": %d\n"
        "    }\n"
        "  }",
        config->enable_camera ? "true" : "false",
        config->camera_resolution
    );
    
#elif defined(MCP_PLATFORM_ESP32)
    pos += snprintf(buffer + pos, size - pos,
        ",\n"
        "  \"platform\": {\n"
        "    \"esp32\": {\n"
        "      \"enable_ota\": %s,\n"
        "      \"enable_web_server\": %s,\n"
        "      \"web_server_port\": %u,\n"
        "      \"enable_deep_sleep\": %s,\n"
        "      \"deep_sleep_time_ms\": %u\n"
        "    }\n"
        "  }",
        config->enable_ota ? "true" : "false",
        config->enable_web_server ? "true" : "false",
        config->web_server_port,
        config->enable_deep_sleep ? "true" : "false",
        config->deep_sleep_time_ms
    );
    
#elif defined(MCP_PLATFORM_ARDUINO)
    pos += snprintf(buffer + pos, size - pos,
        ",\n"
        "  \"platform\": {\n"
        "    \"arduino\": {\n"
        "      \"analog_reference\": %d,\n"
        "      \"enable_watchdog\": %s\n"
        "    }\n"
        "  }",
        config->analog_reference,
        config->enable_watchdog ? "true" : "false"
    );
    
#elif defined(MCP_PLATFORM_MBED)
    pos += snprintf(buffer + pos, size - pos,
        ",\n"
        "  \"platform\": {\n"
        "    \"mbed\": {\n"
        "      \"enable_rtos\": %s,\n"
        "      \"task_stack_size\": %u\n"
        "    }\n"
        "  }",
        config->enable_rtos ? "true" : "false",
        config->task_stack_size
    );

#elif defined(MCP_PLATFORM_HOST)
    pos += snprintf(buffer + pos, size - pos,
        ",\n"
        "  \"platform\": {\n"
        "    \"host\": {\n"
        "      \"enable_mock_hardware\": %s,\n"
        "      \"test_mode\": %d\n"
        "    }\n"
        "  }",
        config->enable_mock_hardware ? "true" : "false",
        config->test_mode
    );
#endif
    
    pos += snprintf(buffer + pos, size - pos, "\n}\n");
    
    return pos;
}

/**
 * @brief Get the default configuration file path
 */
const char* MCP_ConfigGetDefaultPath(void) {
    #if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
    return "embedded";
    #else
    return s_default_config_path;
    #endif
}

/**
 * @brief Platform-independent API for setting configuration via JSON
 */
int MCP_SetConfiguration(const char* json_config) {
    if (json_config == NULL) {
        return -1;
    }
    
    if (!s_config_initialized) {
        MCP_ConfigInit(&s_config);
    }
    
    int result = MCP_ConfigUpdateFromJSON(&s_config, json_config);
    if (result != 0) {
        return result;
    }
    
    // Save the updated configuration if persistence is enabled
    if (s_config.common.enable_persistence) {
        result = MCP_ConfigSave(&s_config, NULL);
        if (result != 0) {
            return result;
        }
    }
    
    return 0;
}

/**
 * @brief Platform-independent API for getting configuration as JSON
 */
int MCP_GetConfiguration(char* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return -1;
    }
    
    if (!s_config_initialized) {
        MCP_ConfigInit(&s_config);
    }
    
    return MCP_ConfigSerializeToJSON(&s_config, buffer, size);
}