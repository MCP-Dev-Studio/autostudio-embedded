#ifndef MOCK_TOOL_REGISTRY_H
#define MOCK_TOOL_REGISTRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Result status codes
typedef enum {
    MCP_TOOL_RESULT_SUCCESS = 0,
    MCP_TOOL_RESULT_ERROR = 1,
    MCP_TOOL_RESULT_INVALID_PARAMS = 2,
    MCP_TOOL_RESULT_NOT_FOUND = 3,
    MCP_TOOL_RESULT_EXECUTION_ERROR = 4,
    MCP_TOOL_RESULT_PERMISSION_DENIED = 5,
    MCP_TOOL_RESULT_TIMEOUT = 6,
    MCP_TOOL_RESULT_NOT_IMPLEMENTED = 7
} MCP_ToolResultStatus;

// Tool result structure
typedef struct {
    MCP_ToolResultStatus status;
    const char* resultJson;
    const void* resultData;
    size_t resultDataSize;
} MCP_ToolResult;

// Tool handler function pointer
typedef MCP_ToolResult (*MCP_ToolHandler)(const char* json, size_t length);

// Implementation types
typedef enum {
    MCP_TOOL_TYPE_NATIVE = 0,
    MCP_TOOL_TYPE_COMPOSITE = 1,
    MCP_TOOL_TYPE_SCRIPT = 2,
    MCP_TOOL_TYPE_BYTECODE = 3,
    MCP_TOOL_TYPE_PROXY = 4
} MCP_ToolType;

// Tool definition structure
typedef struct {
    char name[64];
    MCP_ToolHandler handler;
    MCP_ToolType type;
    char* schema;
    void* implementation;
    bool isDynamic;
    bool persistent;
} MCP_ToolDefinition;

// Mock declarations for tool registry functions
int MCP_ToolRegistryInit(uint32_t maxTools);
int MCP_ToolRegister(const char* name, MCP_ToolHandler handler, const char* schema);
int MCP_ToolRegisterDynamic(const char* json, size_t length);
MCP_ToolResult MCP_ToolExecute(const char* json, size_t length);
const MCP_ToolDefinition* MCP_ToolGetDefinition(const char* name);
int MCP_ToolGetList(char* buffer, size_t bufferSize);
int MCP_ToolSaveDynamic(const char* name);

// Mock implementations for creating tool results
MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult);
MCP_ToolResult MCP_ToolCreateErrorResult(MCP_ToolResultStatus status, const char* errorMessage);

#endif // MOCK_TOOL_REGISTRY_H
