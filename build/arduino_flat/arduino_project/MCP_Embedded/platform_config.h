/**
 * @file platform_config.h
 * @brief Platform-specific configuration definitions
 * 
 * This file defines platform-specific macros to ensure consistent
 * configuration management across all platforms.
 */
#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#include "mcp_config.h"

/**
 * @brief Define platform macros based on build environment
 */
#if defined(PLATFORM_RPI) || defined(RASPBERRY_PI)
#define MCP_PLATFORM_RPI
#elif defined(PLATFORM_ESP32) || defined(ESP32)
#define MCP_PLATFORM_ESP32
#elif defined(PLATFORM_ARDUINO) || defined(ARDUINO)
#define MCP_PLATFORM_ARDUINO
#elif defined(PLATFORM_MBED) || defined(MBED)
#define MCP_PLATFORM_MBED
#endif

/**
 * @brief Map platform-specific configuration structures to common configuration
 */

#if defined(MCP_PLATFORM_RPI)
#include "mcp_rpi.h"

#elif defined(MCP_PLATFORM_ESP32)
#include "mcp_esp32.h"

#elif defined(MCP_PLATFORM_ARDUINO)
#include "mcp_arduino.h"

#elif defined(MCP_PLATFORM_MBED)
#include "mcp_mbed.h"

#endif

/**
 * @brief Helper function to convert platform-specific config to common config
 * 
 * @param platform_config Platform-specific configuration
 * @param common_config Common configuration structure
 * @return int 0 on success, negative error code on failure
 */
int PlatformConfigToCommonConfig(void* platform_config, MCP_CommonConfig* common_config);

/**
 * @brief Helper function to fill platform-specific config from common config
 * 
 * @param common_config Common configuration structure
 * @param platform_config Platform-specific configuration
 * @return int 0 on success, negative error code on failure
 */
int CommonConfigToPlatformConfig(MCP_CommonConfig* common_config, void* platform_config);

#endif /* PLATFORM_CONFIG_H */