#ifndef PERSISTENT_STORAGE_H
#define PERSISTENT_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Storage types
 */
typedef enum {
    STORAGE_TYPE_EEPROM,      // EEPROM storage (small, byte-addressable)
    STORAGE_TYPE_FLASH,       // Flash memory storage (larger, block-oriented)
    STORAGE_TYPE_SD_CARD,     // SD card storage
    STORAGE_TYPE_FILE_SYSTEM, // File system (for platforms with OS)
    STORAGE_TYPE_NVS          // Non-volatile storage (ESP32)
} StorageType;

/**
 * @brief Storage initialization configuration
 */
typedef struct {
    StorageType type;          // Storage type
    const char* mountPoint;    // Mount point (for file system)
    const char* basePath;      // Base path (for file system)
    uint32_t baseAddress;      // Base address (for EEPROM/flash)
    uint32_t size;             // Storage size in bytes
    bool formatIfMounted;      // Format if mount fails (for file system)
    const char* partition;     // Partition name (for ESP32 NVS)
    bool readOnly;             // Read-only mode
} StorageConfig;

/**
 * @brief Initialize persistent storage
 * 
 * @param config Storage configuration
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_init(const StorageConfig* config);

/**
 * @brief Deinitialize persistent storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_deinit(void);

/**
 * @brief Write data to persistent storage
 * 
 * @param key Storage key
 * @param data Data to write
 * @param size Data size in bytes
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_write(const char* key, const void* data, size_t size);

/**
 * @brief Read data from persistent storage
 * 
 * @param key Storage key
 * @param data Buffer to store read data
 * @param maxSize Maximum data size to read
 * @param actualSize Pointer to store actual size read
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize);

/**
 * @brief Check if key exists in persistent storage
 * 
 * @param key Storage key
 * @return bool True if key exists, false otherwise
 */
bool persistent_storage_exists(const char* key);

/**
 * @brief Delete key from persistent storage
 * 
 * @param key Storage key
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_delete(const char* key);

/**
 * @brief Get all keys in persistent storage
 * 
 * @param keys Array to store keys (must be pre-allocated)
 * @param maxKeys Maximum number of keys to return
 * @return int Number of keys found or negative error code
 */
int persistent_storage_get_keys(char** keys, size_t maxKeys);

/**
 * @brief Get size of data stored under key
 * 
 * @param key Storage key
 * @return int Size in bytes or negative error code
 */
int persistent_storage_get_size(const char* key);

/**
 * @brief Commit changes to persistent storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_commit(void);

/**
 * @brief Clear all data in persistent storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_clear(void);

/**
 * @brief Get free space in persistent storage
 * 
 * @return int Free space in bytes or negative error code
 */
int persistent_storage_get_free_space(void);

/**
 * @brief Get total space in persistent storage
 * 
 * @return int Total space in bytes or negative error code
 */
int persistent_storage_get_total_space(void);

/**
 * @brief Begin a transaction (for multi-operation consistency)
 * 
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_begin_transaction(void);

/**
 * @brief End a transaction and commit changes
 * 
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_end_transaction(void);

/**
 * @brief Set storage compression (if supported)
 * 
 * @param enable Enable compression
 * @return int 0 on success, negative error code on failure
 */
int persistent_storage_set_compression(bool enable);

#endif /* PERSISTENT_STORAGE_H */