#ifndef MCP_AUTH_MANAGER_H
#define MCP_AUTH_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Authentication method types
 */
typedef enum {
    AUTH_METHOD_NONE = 0,    // No authentication required (open access)
    AUTH_METHOD_BEARER,      // Bearer token authentication
    AUTH_METHOD_API_KEY,     // API key authentication
    AUTH_METHOD_BASIC,       // Basic authentication (username/password)
    AUTH_METHOD_OAUTH,       // OAuth token authentication
    AUTH_METHOD_CUSTOM       // Custom authentication method
} MCP_AuthMethod;

/**
 * @brief Authentication configuration
 */
typedef struct {
    MCP_AuthMethod method;   // Authentication method
    char* token;             // Authentication token/key
    bool persistent;         // Whether to persist authentication settings
    bool initialized;        // Whether authentication has been initialized
} MCP_AuthConfig;

/**
 * @brief Initialize the authentication manager
 * 
 * @param initialOpen Whether to start with open authentication (no auth required)
 * @return int 0 on success, negative error code on failure
 */
int MCP_AuthManagerInit(bool initialOpen);

/**
 * @brief Set the authentication method and credentials
 * 
 * @param method Authentication method
 * @param token Authentication token/key
 * @param persistent Whether to persist authentication settings
 * @return int 0 on success, negative error code on failure
 */
int MCP_AuthManagerSetAuth(MCP_AuthMethod method, const char* token, bool persistent);

/**
 * @brief Clear authentication (set to open/no authentication)
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_AuthManagerClearAuth(void);

/**
 * @brief Validate authentication credentials
 * 
 * @param method Authentication method
 * @param token Authentication token to validate
 * @return bool True if authentication is valid, false otherwise
 */
bool MCP_AuthManagerValidate(MCP_AuthMethod method, const char* token);

/**
 * @brief Check if authentication is required
 * 
 * @return bool True if authentication is required, false if open access
 */
bool MCP_AuthManagerIsRequired(void);

/**
 * @brief Save authentication settings to persistent storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_AuthManagerSave(void);

/**
 * @brief Load authentication settings from persistent storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_AuthManagerLoad(void);

#endif /* MCP_AUTH_MANAGER_H */