#ifndef MCP_OS_CORE_H
#define MCP_OS_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file mcp_os_core.h
 * @brief Core header file for MCP OS
 * 
 * This header includes all necessary components for MCP OS operation.
 * Applications should include this file to access the full MCP OS API.
 */

/* Include platform detection */
#if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST) || defined(HOST)
    /* For host testing platform */
    #define MCP_OS_HOST 1
    #define MCP_PLATFORM_HOST 1
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <unistd.h>

    /* Host platform configuration */
    typedef struct {
        int port;
        char host[64];
        int baudRate;
        char logFile[256];
        int logLevel;
        bool useTCP;
    } MCP_HostConfig;
    
    /* Forward declarations for host platform functions */
#ifdef __cplusplus
extern "C" {
#endif

    int MCP_LoadPersistentState(void);
    int MCP_ServerStart(void);
    int MCP_SystemProcess(uint32_t timeout);

#ifdef __cplusplus
}
#endif
    
#elif defined(MCP_OS_MBED)
    #include "mcp_mbed.h"
    #include "hal_mbed.h"
    #include "io_mapper.h"
    #include "pin_mapping.h"
#elif defined(MCP_OS_ARDUINO)
    #include "mcp_arduino.h"
    #include "hal_arduino.h"
#elif defined(MCP_OS_ESP32)
    #include "mcp_esp32.h"
    #include "hal_esp32.h"
#elif defined(MCP_OS_RPI)
    #include "mcp_rpi.h"
    #include "hal_rpi.h"
#else
    #error "No platform defined! Define MCP_OS_HOST, MCP_OS_MBED, MCP_OS_ARDUINO, MCP_OS_ESP32, or MCP_OS_RPI"
#endif

/* Core layer includes */
#include "task_scheduler.h"
#include "memory_manager.h"
#include "event_system.h"
#include "config_system.h"

/* Tool system includes */
/* Use consolidated headers for all platforms */
#include "tool_registry.h"
#include "tool_info.h"

#if !defined(MCP_OS_HOST) && !defined(MCP_PLATFORM_HOST)
/* For regular platforms, include additional tool system headers */
#include "tool_handler.h"
#include "automation_engine.h"
#include "rule_interpreter.h"
#include "bytecode_interpreter.h"
#endif

/* Device layer includes */
#include "driver_manager.h"
#include "sensor_manager.h"
#include "actuator_manager.h"
#include "i2c_controller.h"
#include "spi_controller.h"
#include "gpio_controller.h"

/* MCP server includes */
#include "server.h"
#include "session.h"
#include "content.h"
#include "protocol_handler.h"
#include "server_nocode.h"

/* Utility includes */
#include "json_parser.h"
#include "persistent_storage.h"
#include "logging.h"

/**
 * @brief MCP OS version information
 */
#define MCP_OS_VERSION_MAJOR 1
#define MCP_OS_VERSION_MINOR 0
#define MCP_OS_VERSION_PATCH 0
#define MCP_OS_VERSION_STR "1.0.0"

/**
 * @brief MCP OS feature flags
 */
#define MCP_OS_FEATURE_TOOLS 1
#define MCP_OS_FEATURE_RESOURCES 1
#define MCP_OS_FEATURE_EVENTS 1
#define MCP_OS_FEATURE_AUTOMATION 1
#define MCP_OS_FEATURE_BYTECODE 1
#define MCP_OS_FEATURE_JSONSCHEMA 1

/**
 * @brief MCP OS memory region sizes
 */
#ifndef MCP_OS_MEMORY_SIZE
#define MCP_OS_MEMORY_SIZE 32768
#endif

#define MCP_OS_MEMORY_STATIC_SIZE (MCP_OS_MEMORY_SIZE / 5)
#define MCP_OS_MEMORY_DYNAMIC_SIZE (MCP_OS_MEMORY_SIZE / 5)
#define MCP_OS_MEMORY_TOOL_SIZE (MCP_OS_MEMORY_SIZE / 5)
#define MCP_OS_MEMORY_RESOURCE_SIZE (MCP_OS_MEMORY_SIZE / 5)
#define MCP_OS_MEMORY_SYSTEM_SIZE (MCP_OS_MEMORY_SIZE / 5)

#ifdef __cplusplus
}
#endif

#endif /* MCP_OS_CORE_H */
