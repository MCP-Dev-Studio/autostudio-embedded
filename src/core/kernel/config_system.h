#ifndef MCP_CONFIG_SYSTEM_H
#define MCP_CONFIG_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Configuration value types
 */
typedef enum {
    MCP_CONFIG_TYPE_BOOL,
    MCP_CONFIG_TYPE_INT,
    MCP_CONFIG_TYPE_FLOAT,
    MCP_CONFIG_TYPE_STRING,
    MCP_CONFIG_TYPE_OBJECT
} MCP_ConfigType;

/**
 * @brief Configuration value storage
 */
typedef union {
    bool boolValue;
    int32_t intValue;
    float floatValue;
    char* stringValue;
    void* objectValue;
} MCP_ConfigValue;

/**
 * @brief Configuration entry
 */
typedef struct {
    char* key;
    MCP_ConfigType type;
    MCP_ConfigValue value;
    bool persistent;    // Should be saved to persistent storage
} MCP_ConfigEntry;

/**
 * @brief Initialize the configuration system
 * 
 * @param maxEntries Maximum number of configuration entries
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigInit(uint16_t maxEntries);

/**
 * @brief Set a boolean configuration value
 * 
 * @param key Configuration key
 * @param value Value to set
 * @param persistent Whether to save to persistent storage
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigSetBool(const char* key, bool value, bool persistent);

/**
 * @brief Set an integer configuration value
 * 
 * @param key Configuration key
 * @param value Value to set
 * @param persistent Whether to save to persistent storage
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigSetInt(const char* key, int32_t value, bool persistent);

/**
 * @brief Set a float configuration value
 * 
 * @param key Configuration key
 * @param value Value to set
 * @param persistent Whether to save to persistent storage
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigSetFloat(const char* key, float value, bool persistent);

/**
 * @brief Set a string configuration value
 * 
 * @param key Configuration key
 * @param value Value to set
 * @param persistent Whether to save to persistent storage
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigSetString(const char* key, const char* value, bool persistent);

/**
 * @brief Set an object configuration value
 * 
 * @param key Configuration key
 * @param value Value to set
 * @param persistent Whether to save to persistent storage
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigSetObject(const char* key, void* value, bool persistent);

/**
 * @brief Get a boolean configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value to return if key not found
 * @return bool Configuration value or default value
 */
bool MCP_ConfigGetBool(const char* key, bool defaultValue);

/**
 * @brief Get an integer configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value to return if key not found
 * @return int32_t Configuration value or default value
 */
int32_t MCP_ConfigGetInt(const char* key, int32_t defaultValue);

/**
 * @brief Get a float configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value to return if key not found
 * @return float Configuration value or default value
 */
float MCP_ConfigGetFloat(const char* key, float defaultValue);

/**
 * @brief Get a string configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value to return if key not found
 * @return const char* Configuration value or default value
 */
const char* MCP_ConfigGetString(const char* key, const char* defaultValue);

/**
 * @brief Get an object configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value to return if key not found
 * @return void* Configuration value or default value
 */
void* MCP_ConfigGetObject(const char* key, void* defaultValue);

/**
 * @brief Remove a configuration entry
 * 
 * @param key Configuration key to remove
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigRemove(const char* key);

/**
 * @brief Save all persistent configuration to storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigSave(void);

/**
 * @brief Load configuration from storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigLoad(void);

/**
 * @brief Export configuration as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_ConfigExportJson(char* buffer, size_t bufferSize);

/**
 * @brief Import configuration from JSON
 * 
 * @param json JSON string to parse
 * @param jsonLength Length of JSON string
 * @return int 0 on success, negative error code on failure
 */
int MCP_ConfigImportJson(const char* json, size_t jsonLength);

#endif /* MCP_CONFIG_SYSTEM_H */