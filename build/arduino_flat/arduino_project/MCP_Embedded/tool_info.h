#ifndef MCP_TOOL_INFO_H
#define MCP_TOOL_INFO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Include platform_compatibility.h for common type definitions
#include "platform_compatibility.h"

// Include content.h for MCP_Content definition
#include "content.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function pointer type for tool invoke function
 * This definition is used across all platforms consistently
 */
typedef int (*MCP_ToolInvokeFunc)(const char* sessionId, const char* operationId, const MCP_Content* params);

/**
 * @brief Tool information structure used for registration
 * Unified definition across all platforms
 */
typedef struct {
    const char* name;                /**< Tool name */
    const char* description;         /**< Tool description */
    const char* schemaJson;          /**< JSON schema for tool parameters */
    int (*init)(void);               /**< Initialization function */
    int (*deinit)(void);             /**< Deinitialization function */
    MCP_ToolInvokeFunc invoke;       /**< Tool invocation handler */
} MCP_ToolInfo;

/**
 * @brief Tool result status codes
 * Consistent across all platforms
 */
typedef enum {
    MCP_TOOL_RESULT_SUCCESS = 0,
    MCP_TOOL_RESULT_ERROR = 1,
    MCP_TOOL_RESULT_NOT_FOUND = 2,
    MCP_TOOL_RESULT_INVALID_PARAMETERS = 3,
    MCP_TOOL_RESULT_EXECUTION_ERROR = 4,
    MCP_TOOL_RESULT_TIMEOUT = 5,
    MCP_TOOL_RESULT_ACCESS_DENIED = 6
} MCP_ToolResultStatus;

/**
 * @brief Tool result structure 
 * Note: This structure is now defined in platform_compatibility.h
 * for consistent cross-platform usage.
 */
// MCP_ToolResult is defined in platform_compatibility.h

/**
 * @brief Tool type enumeration
 * Consistent across all platforms
 */
typedef enum {
    MCP_TOOL_TYPE_NATIVE = 0,
    MCP_TOOL_TYPE_COMPOSITE = 1,
    MCP_TOOL_TYPE_SCRIPT = 2,
    MCP_TOOL_TYPE_BYTECODE = 3
} MCP_ToolType;

/**
 * @brief Composite tool step structure
 * Consistent across all platforms
 */
typedef struct MCP_ToolStep {
    const char* toolName;           /**< Tool to invoke */
    const char* paramsTemplate;     /**< Parameters template with variables */
    const char* resultStore;        /**< Variable name to store result */
    struct MCP_ToolStep* next;      /**< Next step in sequence */
} MCP_ToolStep;

/**
 * @brief Tool definition structure
 * Consistent across all platforms
 */
typedef struct {
    const char* name;               /**< Tool name */
    const char* description;        /**< Tool description */
    const char* schema;             /**< JSON schema for parameters */
    MCP_ToolType type;              /**< Tool type */
    
    /* Implementation based on type */
    union {
        /* Native tool handler */
        #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
        void* nativeHandler;        /**< Handler function (void* for HOST compatibility) */
        #else
        MCP_ToolResult (*nativeHandler)(const char* json, size_t length); /**< Native handler function */
        #endif
        
        /* Composite tool steps */
        struct {
            MCP_ToolStep* steps;     /**< First step in sequence */
            uint32_t stepCount;      /**< Number of steps */
        } composite;
        
        /* Script-based tool */
        struct {
            const char* script;      /**< Script content */
            const char* language;    /**< Script language */
        } script;
        
        /* Bytecode tool */
        struct {
            const uint8_t* program;  /**< Bytecode program */
            size_t size;             /**< Bytecode size */
        } bytecode;
    } implementation;
    
    bool isDynamic;                 /**< Whether this is a dynamic tool */
    uint32_t creationTime;          /**< Tool creation time */
    bool persistent;                /**< Whether to persist this tool */
} MCP_ToolDefinition;

// MCP_ToolCreateSuccessResult 및 MCP_ToolCreateErrorResult 함수들은
// 전체 플랫폼에서 일관된 동작을 위해 tool_registry.h에서 공통으로 정의됩니다.

#ifdef __cplusplus
}
#endif

#endif /* MCP_TOOL_INFO_H */