/**
 * @file mcp_config_test.c
 * @brief Modified implementation of MCP configuration system for testing
 */
#include "../src/core/mcp/config/mcp_config.h"
#include "../src/json/json_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Simplified logging for tests
void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[INFO] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[WARN] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[ERROR] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

// Simple stubs for persistent storage functions
int persistent_storage_write(const char* key, const void* data, size_t size) {
    FILE* fp = fopen(key, "wb");
    if (fp == NULL) {
        return -1;
    }
    
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    
    return (written == size) ? 0 : -2;
}

int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize) {
    FILE* fp = fopen(key, "rb");
    if (fp == NULL) {
        *actualSize = 0;
        return -1;
    }
    
    *actualSize = fread(data, 1, maxSize, fp);
    fclose(fp);
    
    return 0;
}

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
    strncpy(config->common.config_file_path, s_default_config_path, 
           sizeof(config->common.config_file_path) - 1);
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
        
        // Create the directory structure using system call (for testing only)
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
    
    printf("Parsing JSON: %.100s...\n", json_string);
    
    // For this test implementation, we'll just do direct string matching
    // In a real implementation, we would use a proper JSON parser
    
    // We'll create a simplified JSON parser for testing
    const char* find_device_section = strstr(json_string, "\"device\"");
    if (find_device_section) {
        printf("Found device section\n");
        const char* name_field = strstr(find_device_section, "\"name\"");
        if (name_field) {
            // Find the value between quotes
            const char* name_start = strstr(name_field, "\":");
            if (name_start) {
                name_start = strchr(name_start + 2, '"');
                if (name_start) {
                    name_start++; // Skip the quote
                    const char* name_end = strchr(name_start, '"');
                    if (name_end) {
                        int name_length = name_end - name_start;
                        char* name = (char*)malloc(name_length + 1);
                        if (name) {
                            strncpy(name, name_start, name_length);
                            name[name_length] = '\0';
                            printf("Found device name: %s\n", name);
                            updateStringField(config->common.device_name, sizeof(config->common.device_name), name);
                            free(name);
                        }
                    }
                }
            }
        }
        
        // Parse version field
        const char* version_field = strstr(find_device_section, "\"firmware_version\"");
        if (version_field) {
            // Find the value between quotes
            const char* version_start = strstr(version_field, "\":");
            if (version_start) {
                version_start = strchr(version_start + 2, '"');
                if (version_start) {
                    version_start++; // Skip the quote
                    const char* version_end = strchr(version_start, '"');
                    if (version_end) {
                        int version_length = version_end - version_start;
                        char* version = (char*)malloc(version_length + 1);
                        if (version) {
                            strncpy(version, version_start, version_length);
                            version[version_length] = '\0';
                            printf("Found firmware version: %s\n", version);
                            updateStringField(config->common.firmware_version, sizeof(config->common.firmware_version), version);
                            free(version);
                        }
                    }
                }
            }
        }
        
        // Parse debug field
        const char* debug_field = strstr(find_device_section, "\"debug_enabled\"");
        if (debug_field) {
            // Find the value (true or false)
            const char* debug_value = debug_field + strlen("\"debug_enabled\"");
            // Skip whitespace and colon
            while (*debug_value && (*debug_value == ' ' || *debug_value == ':')) {
                debug_value++;
            }
            if (strncmp(debug_value, "true", 4) == 0) {
                config->common.debug_enabled = true;
                printf("Found debug_enabled: true\n");
            } else if (strncmp(debug_value, "false", 5) == 0) {
                config->common.debug_enabled = false;
                printf("Found debug_enabled: false\n");
            }
        }
    }
    
    // Parse server section
    const char* find_server_section = strstr(json_string, "\"server\"");
    if (find_server_section) {
        printf("Found server section\n");
        
        // Parse enabled field
        const char* enabled_field = strstr(find_server_section, "\"enabled\"");
        if (enabled_field) {
            // Find the value (true or false)
            const char* enabled_value = enabled_field + strlen("\"enabled\"");
            // Skip whitespace and colon
            while (*enabled_value && (*enabled_value == ' ' || *enabled_value == ':')) {
                enabled_value++;
            }
            if (strncmp(enabled_value, "true", 4) == 0) {
                config->common.server_enabled = true;
                printf("Found server enabled: true\n");
            } else if (strncmp(enabled_value, "false", 5) == 0) {
                config->common.server_enabled = false;
                printf("Found server enabled: false\n");
            }
        }
        
        // Parse port field
        const char* port_field = strstr(find_server_section, "\"port\"");
        if (port_field) {
            // Find the value (integer)
            const char* port_value = port_field + strlen("\"port\"");
            // Skip whitespace and colon
            while (*port_value && (*port_value == ' ' || *port_value == ':')) {
                port_value++;
            }
            // Parse the integer
            if (*port_value >= '0' && *port_value <= '9') {
                int port = atoi(port_value);
                if (port > 0) {
                    config->common.server_port = port;
                    printf("Found server port: %d\n", port);
                }
            }
        }
        
        // Parse auto_start field
        const char* auto_start_field = strstr(find_server_section, "\"auto_start\"");
        if (auto_start_field) {
            // Find the value (true or false)
            const char* auto_start_value = auto_start_field + strlen("\"auto_start\"");
            // Skip whitespace and colon
            while (*auto_start_value && (*auto_start_value == ' ' || *auto_start_value == ':')) {
                auto_start_value++;
            }
            if (strncmp(auto_start_value, "true", 4) == 0) {
                config->common.auto_start_server = true;
                printf("Found server auto_start: true\n");
            } else if (strncmp(auto_start_value, "false", 5) == 0) {
                config->common.auto_start_server = false;
                printf("Found server auto_start: false\n");
            }
        }
    }
    
    // Parse network section
    const char* find_network_section = strstr(json_string, "\"network\"");
    if (find_network_section) {
        printf("Found network section\n");
        
        // Find wifi subsection
        const char* find_wifi_section = strstr(find_network_section, "\"wifi\"");
        if (find_wifi_section) {
            printf("Found wifi section\n");
            
            // Parse enabled field
            const char* enabled_field = strstr(find_wifi_section, "\"enabled\"");
            if (enabled_field) {
                // Find the value (true or false)
                const char* enabled_value = enabled_field + strlen("\"enabled\"");
                // Skip whitespace and colon
                while (*enabled_value && (*enabled_value == ' ' || *enabled_value == ':')) {
                    enabled_value++;
                }
                if (strncmp(enabled_value, "true", 4) == 0) {
                    config->common.network.enabled = true;
                    printf("Found wifi enabled: true\n");
                } else if (strncmp(enabled_value, "false", 5) == 0) {
                    config->common.network.enabled = false;
                    printf("Found wifi enabled: false\n");
                }
            }
            
            // Parse ssid field
            const char* ssid_field = strstr(find_wifi_section, "\"ssid\"");
            if (ssid_field) {
                // Find the value between quotes
                const char* ssid_start = strstr(ssid_field, "\":");
                if (ssid_start) {
                    ssid_start = strchr(ssid_start + 2, '"');
                    if (ssid_start) {
                        ssid_start++; // Skip the quote
                        const char* ssid_end = strchr(ssid_start, '"');
                        if (ssid_end) {
                            int ssid_length = ssid_end - ssid_start;
                            char* ssid = (char*)malloc(ssid_length + 1);
                            if (ssid) {
                                strncpy(ssid, ssid_start, ssid_length);
                                ssid[ssid_length] = '\0';
                                printf("Found wifi ssid: %s\n", ssid);
                                updateStringField(config->common.network.ssid, sizeof(config->common.network.ssid), ssid);
                                free(ssid);
                            }
                        }
                    }
                }
            }
            
            // Parse password field
            const char* password_field = strstr(find_wifi_section, "\"password\"");
            if (password_field) {
                // Find the value between quotes
                const char* password_start = strstr(password_field, "\":");
                if (password_start) {
                    password_start = strchr(password_start + 2, '"');
                    if (password_start) {
                        password_start++; // Skip the quote
                        const char* password_end = strchr(password_start, '"');
                        if (password_end) {
                            int password_length = password_end - password_start;
                            char* password = (char*)malloc(password_length + 1);
                            if (password) {
                                strncpy(password, password_start, password_length);
                                password[password_length] = '\0';
                                printf("Found wifi password\n");
                                updateStringField(config->common.network.password, sizeof(config->common.network.password), password);
                                free(password);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Parse platform section for Raspberry Pi
#if defined(MCP_PLATFORM_RPI)
    const char* find_platform_section = strstr(json_string, "\"platform\"");
    if (find_platform_section) {
        printf("Found platform section\n");
        
        // Find rpi subsection
        const char* find_rpi_section = strstr(find_platform_section, "\"rpi\"");
        if (find_rpi_section) {
            printf("Found rpi section\n");
            
            // Parse enable_camera field
            const char* camera_field = strstr(find_rpi_section, "\"enable_camera\"");
            if (camera_field) {
                // Find the value (true or false)
                const char* camera_value = camera_field + strlen("\"enable_camera\"");
                // Skip whitespace and colon
                while (*camera_value && (*camera_value == ' ' || *camera_value == ':')) {
                    camera_value++;
                }
                if (strncmp(camera_value, "true", 4) == 0) {
                    config->enable_camera = true;
                    printf("Found enable_camera: true\n");
                } else if (strncmp(camera_value, "false", 5) == 0) {
                    config->enable_camera = false;
                    printf("Found enable_camera: false\n");
                }
            }
            
            // Parse camera_resolution field
            const char* resolution_field = strstr(find_rpi_section, "\"camera_resolution\"");
            if (resolution_field) {
                // Find the value (integer)
                const char* resolution_value = resolution_field + strlen("\"camera_resolution\"");
                // Skip whitespace and colon
                while (*resolution_value && (*resolution_value == ' ' || *resolution_value == ':')) {
                    resolution_value++;
                }
                // Parse the integer
                if (*resolution_value >= '0' && *resolution_value <= '9') {
                    int resolution = atoi(resolution_value);
                    if (resolution > 0) {
                        config->camera_resolution = resolution;
                        printf("Found camera_resolution: %d\n", resolution);
                    }
                }
            }
        }
    }
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
#endif
    
    pos += snprintf(buffer + pos, size - pos, "\n}\n");
    
    return pos;
}

/**
 * @brief Get the default configuration file path
 */
const char* MCP_ConfigGetDefaultPath(void) {
    return s_default_config_path;
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