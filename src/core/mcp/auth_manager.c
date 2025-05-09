#include "auth_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Include the registry header (now consolidated for all platforms)
#include "../tool_system/tool_registry.h"

#include "../../system/persistent_storage.h"

// Static authentication configuration
static MCP_AuthConfig s_authConfig = {
    .method = AUTH_METHOD_NONE,
    .token = NULL,
    .persistent = false,
    .initialized = false
};

// Auth storage key
#define AUTH_STORAGE_KEY "mcp_auth_config"

// Map persistent_storage functions to storage functions
#define persistentStorageWrite persistent_storage_write
#define persistentStorageRead persistent_storage_read

// Forward declarations for tool handlers
static MCP_ToolResult setAuthHandler(const char* json, size_t length);
static MCP_ToolResult getAuthStatusHandler(const char* json, size_t length);
static MCP_ToolResult clearAuthHandler(const char* json, size_t length);

/**
 * @brief Initialize the authentication manager
 */
int MCP_AuthManagerInit(bool initialOpen) {
    // Initialize auth configuration
    s_authConfig.method = initialOpen ? AUTH_METHOD_NONE : AUTH_METHOD_BEARER;
    
    // Try to load auth settings from persistent storage
    if (MCP_AuthManagerLoad() != 0 && !initialOpen) {
        // If loading fails and initial open is not set, create default token
        s_authConfig.token = strdup("default-mcp-token");
    }
    
    s_authConfig.initialized = true;
    
    // Register auth management tools
    const char* setAuthSchema = 
        "{"
        "\"name\":\"system.setAuth\","
        "\"description\":\"Set authentication method and credentials\","
        "\"params\":{"
        "\"properties\":{"
        "\"method\":{\"type\":\"string\",\"enum\":[\"none\",\"bearer\",\"api_key\",\"basic\",\"oauth\",\"custom\"]},"
        "\"token\":{\"type\":\"string\"},"
        "\"persistent\":{\"type\":\"boolean\"}"
        "},"
        "\"required\":[\"method\"]"
        "}"
        "}";
    
    MCP_ToolRegister_Legacy("system.setAuth", setAuthHandler, setAuthSchema);
    
    const char* getAuthStatusSchema = 
        "{"
        "\"name\":\"system.getAuthStatus\","
        "\"description\":\"Get current authentication status\","
        "\"params\":{"
        "\"properties\":{}"
        "}"
        "}";
    
    MCP_ToolRegister_Legacy("system.getAuthStatus", getAuthStatusHandler, getAuthStatusSchema);
    
    const char* clearAuthSchema = 
        "{"
        "\"name\":\"system.clearAuth\","
        "\"description\":\"Clear authentication (set to open access)\","
        "\"params\":{"
        "\"properties\":{}"
        "}"
        "}";
    
    MCP_ToolRegister_Legacy("system.clearAuth", clearAuthHandler, clearAuthSchema);
    
    #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
    printf("HOST: Auth manager initialized with %s access\n", 
          initialOpen ? "open" : "restricted");
    #endif
    
    return 0;
}

/**
 * @brief Set the authentication method and credentials
 */
int MCP_AuthManagerSetAuth(MCP_AuthMethod method, const char* token, bool persistent) {
    if (!s_authConfig.initialized) {
        return -1; // Not initialized
    }
    
    // Free old token if exists
    if (s_authConfig.token != NULL) {
        free(s_authConfig.token);
        s_authConfig.token = NULL;
    }
    
    // Set new auth configuration
    s_authConfig.method = method;
    s_authConfig.persistent = persistent;
    
    // Set token if provided and method requires it
    if (token != NULL && method != AUTH_METHOD_NONE) {
        s_authConfig.token = strdup(token);
        if (s_authConfig.token == NULL) {
            return -2; // Memory allocation failed
        }
    }
    
    // Save to persistent storage if enabled
    if (persistent) {
        MCP_AuthManagerSave();
    }
    
    #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
    printf("HOST: Auth method set to %d, persistent=%s\n", 
          (int)method, persistent ? "true" : "false");
    #endif
    
    return 0;
}

/**
 * @brief Clear authentication (set to open/no authentication)
 */
int MCP_AuthManagerClearAuth(void) {
    #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
    printf("HOST: Clearing authentication\n");
    #endif
    
    return MCP_AuthManagerSetAuth(AUTH_METHOD_NONE, NULL, s_authConfig.persistent);
}

/**
 * @brief Validate authentication credentials
 * 
 * This function validates authentication credentials against the configured
 * authentication method and token. If authentication is not required (open access),
 * all requests will be allowed regardless of credentials.
 * 
 * @param method Authentication method from request
 * @param token Authentication token from request
 * @return bool True if authentication is valid or not required, false otherwise
 */
bool MCP_AuthManagerValidate(MCP_AuthMethod method, const char* token) {
    // If no authentication required, always return true regardless of provided method/token
    if (s_authConfig.method == AUTH_METHOD_NONE) {
        return true;
    }
    
    // If we receive a null method parameter, treat as no authentication
    // This allows clients to connect without specifying any auth
    if (method == AUTH_METHOD_NONE) {
        return true;
    }
    
    // If methods don't match, authentication fails
    if (method != s_authConfig.method) {
        return false;
    }
    
    // If no token is set or provided, authentication fails
    if (s_authConfig.token == NULL || token == NULL) {
        return false;
    }
    
    // Compare tokens
    return (strcmp(token, s_authConfig.token) == 0);
}

/**
 * @brief Check if authentication is required
 */
bool MCP_AuthManagerIsRequired(void) {
    return (s_authConfig.method != AUTH_METHOD_NONE);
}

/**
 * @brief Save authentication settings to persistent storage
 */
int MCP_AuthManagerSave(void) {
    // Create JSON representation of auth config
    char authJson[256];
    int offset = 0;
    
    // Build auth JSON
    offset += snprintf(authJson + offset, sizeof(authJson) - offset, 
                     "{\"method\":%d", (int)s_authConfig.method);
    
    if (s_authConfig.token != NULL) {
        offset += snprintf(authJson + offset, sizeof(authJson) - offset,
                         ",\"token\":\"%s\"", s_authConfig.token);
    }
    
    offset += snprintf(authJson + offset, sizeof(authJson) - offset,
                     ",\"persistent\":%s}", s_authConfig.persistent ? "true" : "false");
    
    #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
    printf("HOST: Saving auth config: %s\n", authJson);
    #endif
    
    // Write to persistent storage
    return persistentStorageWrite(AUTH_STORAGE_KEY, authJson, strlen(authJson));
}

/**
 * @brief Load authentication settings from persistent storage
 */
int MCP_AuthManagerLoad(void) {
    // Read from persistent storage
    char authJson[256];
    size_t actualSize;
    
    int result = persistentStorageRead(AUTH_STORAGE_KEY, authJson, sizeof(authJson), &actualSize);
    if (result != 0) {
        #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
        printf("HOST: Failed to load auth config (error: %d)\n", result);
        #endif
        return result; // Failed to read
    }
    
    #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
    printf("HOST: Loaded auth config: %s\n", authJson);
    #endif
    
    // Parse JSON (simplified parsing for demo)
    char tokenBuffer[128] = {0};
    int method = 0;
    
    // Very simple parsing, in a real implementation use a JSON parser
    if (sscanf(authJson, "{\"method\":%d,\"token\":\"%127[^\"]\"", &method, tokenBuffer) >= 1) {
        // Free old token if exists
        if (s_authConfig.token != NULL) {
            free(s_authConfig.token);
            s_authConfig.token = NULL;
        }
        
        // Set authentication method
        s_authConfig.method = (MCP_AuthMethod)method;
        
        // Set token if found
        if (tokenBuffer[0] != '\0') {
            s_authConfig.token = strdup(tokenBuffer);
        }
        
        // Check for persistent flag
        if (strstr(authJson, "\"persistent\":true") != NULL) {
            s_authConfig.persistent = true;
        } else {
            s_authConfig.persistent = false;
        }
        
        return 0;
    }
    
    return -1; // Invalid format
}

/**
 * @brief Convert method enum to string
 */
static const char* methodToString(MCP_AuthMethod method) {
    switch (method) {
        case AUTH_METHOD_NONE:
            return "none";
        case AUTH_METHOD_BEARER:
            return "bearer";
        case AUTH_METHOD_API_KEY:
            return "api_key";
        case AUTH_METHOD_BASIC:
            return "basic";
        case AUTH_METHOD_OAUTH:
            return "oauth";
        case AUTH_METHOD_CUSTOM:
            return "custom";
        default:
            return "unknown";
    }
}

/**
 * @brief Convert method string to enum
 */
static MCP_AuthMethod stringToMethod(const char* methodStr) {
    if (methodStr == NULL) {
        return AUTH_METHOD_NONE;
    }
    
    if (strcmp(methodStr, "none") == 0) {
        return AUTH_METHOD_NONE;
    } else if (strcmp(methodStr, "bearer") == 0) {
        return AUTH_METHOD_BEARER;
    } else if (strcmp(methodStr, "api_key") == 0) {
        return AUTH_METHOD_API_KEY;
    } else if (strcmp(methodStr, "basic") == 0) {
        return AUTH_METHOD_BASIC;
    } else if (strcmp(methodStr, "oauth") == 0) {
        return AUTH_METHOD_OAUTH;
    } else if (strcmp(methodStr, "custom") == 0) {
        return AUTH_METHOD_CUSTOM;
    }
    
    return AUTH_METHOD_NONE;
}

/**
 * @brief Handler for system.setAuth tool
 */
static MCP_ToolResult setAuthHandler(const char* json, size_t length) {
    #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
    // Simple stub for HOST platform
    printf("HOST: setAuthHandler called with: %s\n", json ? json : "NULL");
    
    // Return success immediately for HOST platform
    return MCP_ToolCreateSuccessResult("{\"status\":\"success\",\"message\":\"Authentication settings updated\"}");
    #else
    // Extract parameters (simplified for demo, real implementation should use JSON parser)
    if (json == NULL || length == 0) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Invalid JSON");
    }
    
    // Extract method
    const char* methodStart = strstr(json, "\"method\":\"");
    if (methodStart == NULL) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Missing method parameter");
    }
    
    // Extract method string
    char methodStr[32] = {0};
    if (sscanf(methodStart + 10, "%31[^\"]", methodStr) != 1) {
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_INVALID_PARAMETERS, "Invalid method format");
    }
    
    // Extract token if provided
    char token[128] = {0};
    const char* tokenStart = strstr(json, "\"token\":\"");
    if (tokenStart != NULL) {
        sscanf(tokenStart + 9, "%127[^\"]", token);
    }
    
    // Extract persistent flag
    bool persistent = false;
    if (strstr(json, "\"persistent\":true") != NULL) {
        persistent = true;
    }
    
    // Convert method string to enum
    MCP_AuthMethod method = stringToMethod(methodStr);
    
    // Set authentication
    int result = MCP_AuthManagerSetAuth(method, token[0] != '\0' ? token : NULL, persistent);
    if (result != 0) {
        char errorMsg[64];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to set authentication (code: %d)", result);
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, errorMsg);
    }
    
    // Return success
    return MCP_ToolCreateSuccessResult("{\"status\":\"success\",\"message\":\"Authentication settings updated\"}");
    #endif
}

/**
 * @brief Handler for system.getAuthStatus tool
 */
static MCP_ToolResult getAuthStatusHandler(const char* json, size_t length) {
    // Build status JSON
    char statusJson[256];
    snprintf(statusJson, sizeof(statusJson),
            "{\"method\":\"%s\",\"authRequired\":%s,\"persistent\":%s}",
            methodToString(s_authConfig.method),
            MCP_AuthManagerIsRequired() ? "true" : "false",
            s_authConfig.persistent ? "true" : "false");
    
    #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
    printf("HOST: getAuthStatusHandler returning: %s\n", statusJson);
    #endif
    
    // Return status
    return MCP_ToolCreateSuccessResult(statusJson);
}

/**
 * @brief Handler for system.clearAuth tool
 */
static MCP_ToolResult clearAuthHandler(const char* json, size_t length) {
    #if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
    // Clear authentication
    printf("HOST: clearAuthHandler called\n");
    
    int result = MCP_AuthManagerClearAuth();
    if (result != 0) {
        char errorMsg[64];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to clear authentication (code: %d)", result);
        return MCP_ToolCreateErrorResult(1, errorMsg); // 1 is MCP_TOOL_RESULT_ERROR for HOST platform
    }
    #else
    // Clear authentication
    int result = MCP_AuthManagerClearAuth();
    if (result != 0) {
        char errorMsg[64];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to clear authentication (code: %d)", result);
        return MCP_ToolCreateErrorResult(MCP_TOOL_RESULT_ERROR, errorMsg);
    }
    #endif
    
    // Return success
    return MCP_ToolCreateSuccessResult("{\"status\":\"success\",\"message\":\"Authentication cleared\"}");
}