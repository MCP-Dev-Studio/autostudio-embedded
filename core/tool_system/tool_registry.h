/**
 * @file tool_registry.h
 * @brief Tool registry for MCP system
 */
#ifndef MCP_TOOL_REGISTRY_H
#define MCP_TOOL_REGISTRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Tool execution result status
 */
typedef enum {
    MCP_TOOL_RESULT_SUCCESS,
    MCP_TOOL_RESULT_ERROR,
    MCP_TOOL_RESULT_INVALID_PARAMETERS,
    MCP_TOOL_RESULT_NOT_FOUND,
    MCP_TOOL_RESULT_EXECUTION_ERROR,
    MCP_TOOL_RESULT_TIMEOUT,
    MCP_TOOL_RESULT_PERMISSION_DENIED
} MCP_ToolResultStatus;

/**
 * @brief Tool execution result
 */
typedef struct {
    MCP_ToolResultStatus status;
    char* resultJson;          // JSON string result (owned by tool)
    void* resultData;          // Binary result data (owned by tool)
    size_t resultDataSize;     // Size of binary result data
} MCP_ToolResult;

/**
 * @brief Tool handler function type
 *
 * @param json JSON string parameters
 * @param length Length of JSON string
 * @return MCP_ToolResult Result of tool execution
 */
typedef MCP_ToolResult (*MCP_ToolHandler)(const char* json, size_t length);

/**
 * @brief Tool type enumeration
 */
typedef enum {
    MCP_TOOL_TYPE_NATIVE,      // Native tool with handler function
    MCP_TOOL_TYPE_COMPOSITE,   // Composite tool with multiple steps
    MCP_TOOL_TYPE_SCRIPT,      // Script-based tool
    MCP_TOOL_TYPE_BYTECODE     // Bytecode-based tool
} MCP_ToolType;

/**
 * @brief Tool step definition (for composite tools)
 */
typedef struct MCP_ToolStep {
    char* toolName;            // Tool to call
    char* paramsTemplate;      // Parameter template (with variables)
    char* resultStore;         // Variable name to store result
    struct MCP_ToolStep* next; // Next step
} MCP_ToolStep;

/**
 * @brief Dynamic tool definition
 */
typedef struct {
    char* name;                // Tool name
    char* description;         // Tool description
    char* schema;              // Parameter schema (JSON)
    MCP_ToolType type;         // Tool type

    // Tool implementation based on type
    union {
        MCP_ToolHandler nativeHandler;  // Native handler function

        struct {
            MCP_ToolStep* steps;        // Composite tool steps
            int stepCount;              // Step count
        } composite;

        struct {
            char* script;               // Script code
            char* language;             // Script language
        } script;

        struct {
            void* program;              // Bytecode program
            size_t size;                // Program size
        } bytecode;
    } implementation;

    bool isDynamic;            // Dynamically defined flag
    uint32_t creationTime;     // Creation timestamp
    bool persistent;           // Save to persistent storage
} MCP_ToolDefinition;

/**
 * @brief Initialize the tool registry
 *
 * @param maxTools Maximum number of tools to register
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolRegistryInit(int maxTools);

/**
 * @brief Register a tool in the registry
 *
 * @param name Tool name
 * @param handler Tool handler function
 * @param schema JSON schema for tool parameters
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolRegister(const char* name, MCP_ToolHandler handler, const char* schema);

/**
 * @brief Register a dynamic tool from JSON definition
 *
 * @param json JSON string with tool definition
 * @param length Length of JSON string
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolRegisterDynamic(const char* json, size_t length);

/**
 * @brief Find a tool by name
 *
 * @param name Tool name to find
 * @return int Tool index or negative error code if not found
 */
int MCP_ToolFind(const char* name);

/**
 * @brief Get tool definition by name
 *
 * @param name Tool name to find
 * @return MCP_ToolDefinition* Tool definition or NULL if not found
 */
const MCP_ToolDefinition* MCP_ToolGetDefinition(const char* name);

/**
 * @brief Execute a tool with JSON parameters
 *
 * @param json JSON string with format {"tool": "toolName", "params": {...}}
 * @param length Length of JSON string
 * @return MCP_ToolResult Result of tool execution
 */
MCP_ToolResult MCP_ToolExecute(const char* json, size_t length);

/**
 * @brief Get the JSON schema for a tool
 *
 * @param name Tool name
 * @return const char* JSON schema string or NULL if not found
 */
const char* MCP_ToolGetSchema(const char* name);

/**
 * @brief Get the list of registered tools as JSON
 *
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_ToolGetList(char* buffer, size_t bufferSize);

/**
 * @brief Save a dynamic tool to persistent storage
 *
 * @param name Tool name
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolSaveDynamic(const char* name);

/**
 * @brief Load a dynamic tool from persistent storage
 *
 * @param name Tool name
 * @return int 0 on success, negative error code on failure
 */
int MCP_ToolLoadDynamic(const char* name);

/**
 * @brief Load all dynamic tools from persistent storage
 *
 * @return int Number of tools loaded or negative error code
 */
int MCP_ToolLoadAllDynamic(void);

/**
 * @brief Create a default success result
 *
 * @param jsonResult Optional JSON result string (can be NULL)
 * @return MCP_ToolResult Success result
 */
MCP_ToolResult MCP_ToolCreateSuccessResult(const char* jsonResult);

/**
 * @brief Create an error result
 *
 * @param status Error status
 * @param errorMessage Error message
 * @return MCP_ToolResult Error result
 */
MCP_ToolResult MCP_ToolCreateErrorResult(MCP_ToolResultStatus status, const char* errorMessage);

#endif /* MCP_TOOL_REGISTRY_H */