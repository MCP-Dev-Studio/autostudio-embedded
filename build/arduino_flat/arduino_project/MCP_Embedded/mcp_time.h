/**
 * @file mcp_time.h
 * @brief Time utilities for MCP system
 */
#ifndef MCP_TIME_H
#define MCP_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get current time in milliseconds
 * 
 * @return uint64_t Current time in milliseconds
 */
uint64_t MCP_GetCurrentTimeMs(void);

#ifdef __cplusplus
}
#endif

#endif /* MCP_TIME_H */