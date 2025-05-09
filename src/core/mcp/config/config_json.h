/**
 * @file config_json.h
 * @brief Default configuration JSON as a string constant
 * 
 * This file provides the default configuration JSON as a string constant,
 * which allows it to be included in the code rather than read from a file.
 * This is particularly useful for platforms like Arduino where file system
 * access may be limited.
 */
#ifndef CONFIG_JSON_H
#define CONFIG_JSON_H

/**
 * Default configuration as a string constant
 */
static const char* DEFAULT_CONFIG_JSON = 
"{\n"
"  \"device\": {\n"
"    \"name\": \"MCP Example Device\",\n"
"    \"firmware_version\": \"1.2.0\",\n"
"    \"debug_enabled\": true\n"
"  },\n"
"  \"server\": {\n"
"    \"enabled\": true,\n"
"    \"port\": 8080,\n"
"    \"auto_start\": true\n"
"  },\n"
"  \"network\": {\n"
"    \"wifi\": {\n"
"      \"enabled\": true,\n"
"      \"ssid\": \"YourNetworkSSID\",\n"
"      \"password\": \"YourNetworkPassword\",\n"
"      \"auto_connect\": true\n"
"    },\n"
"    \"wifi_ap\": {\n"
"      \"enabled\": false,\n"
"      \"ssid\": \"MCP-Device-AP\",\n"
"      \"password\": \"mcp-access\",\n"
"      \"channel\": 6\n"
"    },\n"
"    \"ble\": {\n"
"      \"enabled\": false,\n"
"      \"device_name\": \"MCP-BLE-Device\",\n"
"      \"auto_advertise\": false\n"
"    },\n"
"    \"ethernet\": {\n"
"      \"enabled\": true,\n"
"      \"interface\": \"eth0\",\n"
"      \"dhcp\": true,\n"
"      \"static_ip\": \"192.168.1.100\",\n"
"      \"gateway\": \"192.168.1.1\",\n"
"      \"subnet\": \"255.255.255.0\"\n"
"    }\n"
"  },\n"
"  \"interfaces\": {\n"
"    \"i2c\": {\n"
"      \"enabled\": true,\n"
"      \"bus_number\": 1\n"
"    },\n"
"    \"spi\": {\n"
"      \"enabled\": true,\n"
"      \"bus_number\": 0\n"
"    },\n"
"    \"uart\": {\n"
"      \"enabled\": true,\n"
"      \"number\": 0,\n"
"      \"baud_rate\": 115200\n"
"    },\n"
"    \"gpio\": {\n"
"      \"enabled\": true\n"
"    }\n"
"  },\n"
"  \"system\": {\n"
"    \"heap_size\": 1048576,\n"
"    \"config_file_path\": \"\",\n"
"    \"enable_persistence\": true\n"
"  },\n"
"  \"platform\": {\n"
"    \"rpi\": {\n"
"      \"enable_camera\": true,\n"
"      \"camera_resolution\": 1080\n"
"    },\n"
"    \"esp32\": {\n"
"      \"enable_ota\": true,\n"
"      \"enable_web_server\": true,\n"
"      \"web_server_port\": 80,\n"
"      \"enable_deep_sleep\": false,\n"
"      \"deep_sleep_time_ms\": 0\n"
"    },\n"
"    \"arduino\": {\n"
"      \"analog_reference\": 0,\n"
"      \"enable_watchdog\": true\n"
"    },\n"
"    \"mbed\": {\n"
"      \"enable_rtos\": true,\n"
"      \"task_stack_size\": 4096\n"
"    }\n"
"  }\n"
"}";

#endif /* CONFIG_JSON_H */