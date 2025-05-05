/**
 * @file build_fix.h
 * @brief Declarations of helper functions to resolve duplicate symbol issues
 */
#ifndef BUILD_FIX_H
#define BUILD_FIX_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/tool_system/tool_registry.h"

// Function declarations (implementations in build_fix.c)
MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult);
MCP_ToolResult MCP_ToolCreateErrorResult(MCP_ToolResultStatus status, const char* errorMessage);

// Add a forward declaration of MCP_DeviceInfoInit
int MCP_DeviceInfoInit(void);

#endif // BUILD_FIX_H
