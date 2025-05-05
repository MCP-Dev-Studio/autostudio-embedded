/**
 * @file persistent_storage.c
 * @brief Persistent storage implementation for embedded systems
 */
#include "persistent_storage.h"
#include "logging.h"
#include <string.h>
#include <stdlib.h>

#ifdef ESP32
#include "nvs.h"
#include "nvs_flash.h"
#endif

#ifdef ARDUINO
#include <EEPROM.h>
#endif

#ifdef MBED
#include "FlashIAP.h"
#endif

// Storage configuration
static StorageConfig s_config;
static bool s_initialized = false;

// Storage context
typedef struct {
    StorageType type;
    void* handle;
    bool transaction_active;
    bool compression_enabled;
} StorageContext;

static StorageContext s_context = {0};

// Key directory structure (for EEPROM/Flash storage)
#define MAX_KEYS 32
#define MAX_KEY_LENGTH 32

typedef struct {
    char key[MAX_KEY_LENGTH];
    uint32_t address;
    uint32_t size;
} KeyEntry;

typedef struct {
    uint32_t magic;         // Magic number to identify valid directory
    uint32_t version;       // Directory format version
    uint32_t keyCount;      // Number of keys in directory
    KeyEntry entries[MAX_KEYS]; // Key entries
} KeyDirectory;

#define DIRECTORY_MAGIC 0x5073746F // "Psto" in ASCII
#define DIRECTORY_VERSION 1
#define DIRECTORY_SIZE sizeof(KeyDirectory)
#define DIRECTORY_ADDRESS 0

// Memory-backed storage (for testing)
#define MEM_STORAGE_SIZE (64 * 1024) // 64 KB
static uint8_t* s_memStorage = NULL;

// Forward declarations for platform-specific implementations
static int storage_init_eeprom(const StorageConfig* config);
static int storage_init_flash(const StorageConfig* config);
static int storage_init_sd_card(const StorageConfig* config);
static int storage_init_file_system(const StorageConfig* config);
static int storage_init_nvs(const StorageConfig* config);

static int storage_write_eeprom(const char* key, const void* data, size_t size);
static int storage_write_flash(const char* key, const void* data, size_t size);
static int storage_write_sd_card(const char* key, const void* data, size_t size);
static int storage_write_file_system(const char* key, const void* data, size_t size);
static int storage_write_nvs(const char* key, const void* data, size_t size);

static int storage_read_eeprom(const char* key, void* data, size_t maxSize, size_t* actualSize);
static int storage_read_flash(const char* key, void* data, size_t maxSize, size_t* actualSize);
static int storage_read_sd_card(const char* key, void* data, size_t maxSize, size_t* actualSize);
static int storage_read_file_system(const char* key, void* data, size_t maxSize, size_t* actualSize);
static int storage_read_nvs(const char* key, void* data, size_t maxSize, size_t* actualSize);

static bool storage_exists_eeprom(const char* key);
static bool storage_exists_flash(const char* key);
static bool storage_exists_sd_card(const char* key);
static bool storage_exists_file_system(const char* key);
static bool storage_exists_nvs(const char* key);

static int storage_delete_eeprom(const char* key);
static int storage_delete_flash(const char* key);
static int storage_delete_sd_card(const char* key);
static int storage_delete_file_system(const char* key);
static int storage_delete_nvs(const char* key);

static int storage_get_keys_eeprom(char** keys, size_t maxKeys);
static int storage_get_keys_flash(char** keys, size_t maxKeys);
static int storage_get_keys_sd_card(char** keys, size_t maxKeys);
static int storage_get_keys_file_system(char** keys, size_t maxKeys);
static int storage_get_keys_nvs(char** keys, size_t maxKeys);

static int storage_get_size_eeprom(const char* key);
static int storage_get_size_flash(const char* key);
static int storage_get_size_sd_card(const char* key);
static int storage_get_size_file_system(const char* key);
static int storage_get_size_nvs(const char* key);

// Directory management for EEPROM/Flash
static KeyDirectory s_directory = {0};
static bool s_directoryLoaded = false;

// Helper functions
static int load_directory(void);
static int save_directory(void);
static int find_key_entry(const char* key);
static int add_key_entry(const char* key, uint32_t address, uint32_t size);
static int remove_key_entry(const char* key);
static uint32_t allocate_storage(size_t size);

// Compression helpers (simple RLE implementation)
static size_t compress_data(const void* data, size_t size, void* compressedData, size_t maxCompressedSize);
static size_t decompress_data(const void* compressedData, size_t compressedSize, void* data, size_t maxSize);

/**
 * @brief Initialize persistent storage
 */
int persistent_storage_init(const StorageConfig* config) {
    if (config == NULL) {
        return -1;
    }

    if (s_initialized) {
        return -2; // Already initialized
    }

    // Store configuration
    memcpy(&s_config, config, sizeof(StorageConfig));
    
    // Initialize context
    s_context.type = config->type;
    s_context.transaction_active = false;
    s_context.compression_enabled = false;
    
    // Initialize storage based on type
    int result = 0;
    
    switch (config->type) {
        case STORAGE_TYPE_EEPROM:
            result = storage_init_eeprom(config);
            break;
            
        case STORAGE_TYPE_FLASH:
            result = storage_init_flash(config);
            break;
            
        case STORAGE_TYPE_SD_CARD:
            result = storage_init_sd_card(config);
            break;
            
        case STORAGE_TYPE_FILE_SYSTEM:
            result = storage_init_file_system(config);
            break;
            
        case STORAGE_TYPE_NVS:
            result = storage_init_nvs(config);
            break;
            
        default:
            return -3; // Unknown storage type
    }
    
    if (result == 0) {
        s_initialized = true;
    }
    
    return result;
}

/**
 * @brief Deinitialize persistent storage
 */
int persistent_storage_deinit(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // Commit any pending changes
    if (s_context.transaction_active) {
        persistent_storage_end_transaction();
    }
    
    // Clean up based on storage type
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            // Nothing to do for EEPROM
            break;
            
        case STORAGE_TYPE_FLASH:
            // Nothing to do for flash
            break;
            
        case STORAGE_TYPE_SD_CARD:
            // Unmount SD card if needed
            break;
            
        case STORAGE_TYPE_FILE_SYSTEM:
            // Unmount file system if needed
            break;
            
        case STORAGE_TYPE_NVS:
#ifdef ESP32
            // Close NVS handle
            if (s_context.handle != NULL) {
                nvs_close((nvs_handle_t)s_context.handle);
                s_context.handle = NULL;
            }
#endif
            break;
            
        default:
            break;
    }
    
    // Free memory-backed storage if allocated
    if (s_memStorage != NULL) {
        free(s_memStorage);
        s_memStorage = NULL;
    }
    
    s_initialized = false;
    return 0;
}

/**
 * @brief Write data to persistent storage
 */
int persistent_storage_write(const char* key, const void* data, size_t size) {
    if (!s_initialized || key == NULL || data == NULL) {
        return -1;
    }
    
    // Check if read-only mode is enabled
    if (s_config.readOnly) {
        return -2;
    }
    
    // Compress data if enabled
    void* dataToWrite = (void*)data;
    size_t sizeToWrite = size;
    
    if (s_context.compression_enabled && size > 16) { // Only compress if data is large enough
        void* compressedData = malloc(size); // Allocate buffer for compressed data
        if (compressedData == NULL) {
            return -3; // Memory allocation failed
        }
        
        size_t compressedSize = compress_data(data, size, compressedData, size);
        if (compressedSize > 0 && compressedSize < size) {
            // Use compressed data
            dataToWrite = compressedData;
            sizeToWrite = compressedSize;
        }
    }
    
    // Write data based on storage type
    int result = 0;
    
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            result = storage_write_eeprom(key, dataToWrite, sizeToWrite);
            break;
            
        case STORAGE_TYPE_FLASH:
            result = storage_write_flash(key, dataToWrite, sizeToWrite);
            break;
            
        case STORAGE_TYPE_SD_CARD:
            result = storage_write_sd_card(key, dataToWrite, sizeToWrite);
            break;
            
        case STORAGE_TYPE_FILE_SYSTEM:
            result = storage_write_file_system(key, dataToWrite, sizeToWrite);
            break;
            
        case STORAGE_TYPE_NVS:
            result = storage_write_nvs(key, dataToWrite, sizeToWrite);
            break;
            
        default:
            result = -4; // Unknown storage type
    }
    
    // Free compressed data if allocated
    if (dataToWrite != data) {
        free(dataToWrite);
    }
    
    // Auto-commit if not in transaction
    if (result == 0 && !s_context.transaction_active) {
        persistent_storage_commit();
    }
    
    return result;
}

/**
 * @brief Read data from persistent storage
 */
int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize) {
    if (!s_initialized || key == NULL || data == NULL || maxSize == 0) {
        return -1;
    }
    
    // Check if key exists
    if (!persistent_storage_exists(key)) {
        return -2; // Key not found
    }
    
    // Read data based on storage type
    int result = 0;
    
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            result = storage_read_eeprom(key, data, maxSize, actualSize);
            break;
            
        case STORAGE_TYPE_FLASH:
            result = storage_read_flash(key, data, maxSize, actualSize);
            break;
            
        case STORAGE_TYPE_SD_CARD:
            result = storage_read_sd_card(key, data, maxSize, actualSize);
            break;
            
        case STORAGE_TYPE_FILE_SYSTEM:
            result = storage_read_file_system(key, data, maxSize, actualSize);
            break;
            
        case STORAGE_TYPE_NVS:
            result = storage_read_nvs(key, data, maxSize, actualSize);
            break;
            
        default:
            result = -3; // Unknown storage type
    }
    
    // Decompress data if needed
    if (result == 0 && s_context.compression_enabled) {
        // Check for compression marker (simplified approach)
        uint8_t* byteData = (uint8_t*)data;
        if (byteData[0] == 0xAB && byteData[1] == 0xCD) { // Compression marker
            // This is compressed data
            void* decompressedData = malloc(maxSize);
            if (decompressedData == NULL) {
                return -4; // Memory allocation failed
            }
            
            size_t decompressedSize = decompress_data(data, *actualSize, decompressedData, maxSize);
            if (decompressedSize > 0) {
                // Copy decompressed data back to buffer
                if (decompressedSize <= maxSize) {
                    memcpy(data, decompressedData, decompressedSize);
                    *actualSize = decompressedSize;
                } else {
                    // Buffer too small for decompressed data
                    memcpy(data, decompressedData, maxSize);
                    *actualSize = maxSize;
                    result = -5;
                }
            } else {
                result = -6; // Decompression failed
            }
            
            free(decompressedData);
        }
    }
    
    return result;
}

/**
 * @brief Check if key exists in persistent storage
 */
bool persistent_storage_exists(const char* key) {
    if (!s_initialized || key == NULL) {
        return false;
    }
    
    // Check if key exists based on storage type
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            return storage_exists_eeprom(key);
            
        case STORAGE_TYPE_FLASH:
            return storage_exists_flash(key);
            
        case STORAGE_TYPE_SD_CARD:
            return storage_exists_sd_card(key);
            
        case STORAGE_TYPE_FILE_SYSTEM:
            return storage_exists_file_system(key);
            
        case STORAGE_TYPE_NVS:
            return storage_exists_nvs(key);
            
        default:
            return false;
    }
}

/**
 * @brief Delete key from persistent storage
 */
int persistent_storage_delete(const char* key) {
    if (!s_initialized || key == NULL) {
        return -1;
    }
    
    // Check if read-only mode is enabled
    if (s_config.readOnly) {
        return -2;
    }
    
    // Delete key based on storage type
    int result = 0;
    
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            result = storage_delete_eeprom(key);
            break;
            
        case STORAGE_TYPE_FLASH:
            result = storage_delete_flash(key);
            break;
            
        case STORAGE_TYPE_SD_CARD:
            result = storage_delete_sd_card(key);
            break;
            
        case STORAGE_TYPE_FILE_SYSTEM:
            result = storage_delete_file_system(key);
            break;
            
        case STORAGE_TYPE_NVS:
            result = storage_delete_nvs(key);
            break;
            
        default:
            result = -3; // Unknown storage type
    }
    
    // Auto-commit if not in transaction
    if (result == 0 && !s_context.transaction_active) {
        persistent_storage_commit();
    }
    
    return result;
}

/**
 * @brief Get all keys in persistent storage
 */
int persistent_storage_get_keys(char** keys, size_t maxKeys) {
    if (!s_initialized || keys == NULL || maxKeys == 0) {
        return -1;
    }
    
    // Get keys based on storage type
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            return storage_get_keys_eeprom(keys, maxKeys);
            
        case STORAGE_TYPE_FLASH:
            return storage_get_keys_flash(keys, maxKeys);
            
        case STORAGE_TYPE_SD_CARD:
            return storage_get_keys_sd_card(keys, maxKeys);
            
        case STORAGE_TYPE_FILE_SYSTEM:
            return storage_get_keys_file_system(keys, maxKeys);
            
        case STORAGE_TYPE_NVS:
            return storage_get_keys_nvs(keys, maxKeys);
            
        default:
            return -2; // Unknown storage type
    }
}

/**
 * @brief Get size of data stored under key
 */
int persistent_storage_get_size(const char* key) {
    if (!s_initialized || key == NULL) {
        return -1;
    }
    
    // Check if key exists
    if (!persistent_storage_exists(key)) {
        return -2; // Key not found
    }
    
    // Get size based on storage type
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            return storage_get_size_eeprom(key);
            
        case STORAGE_TYPE_FLASH:
            return storage_get_size_flash(key);
            
        case STORAGE_TYPE_SD_CARD:
            return storage_get_size_sd_card(key);
            
        case STORAGE_TYPE_FILE_SYSTEM:
            return storage_get_size_file_system(key);
            
        case STORAGE_TYPE_NVS:
            return storage_get_size_nvs(key);
            
        default:
            return -3; // Unknown storage type
    }
}

/**
 * @brief Commit changes to persistent storage
 */
int persistent_storage_commit(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // Commit changes based on storage type
    int result = 0;
    
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            // Save directory for EEPROM
            result = save_directory();
            break;
            
        case STORAGE_TYPE_FLASH:
            // Save directory for flash
            result = save_directory();
            break;
            
        case STORAGE_TYPE_SD_CARD:
            // Nothing to do for SD card
            break;
            
        case STORAGE_TYPE_FILE_SYSTEM:
            // Nothing to do for file system
            break;
            
        case STORAGE_TYPE_NVS:
#ifdef ESP32
            // Commit NVS changes
            if (s_context.handle != NULL) {
                result = nvs_commit((nvs_handle_t)s_context.handle);
            }
#endif
            break;
            
        default:
            result = -2; // Unknown storage type
    }
    
    return result;
}

/**
 * @brief Clear all data in persistent storage
 */
int persistent_storage_clear(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // Check if read-only mode is enabled
    if (s_config.readOnly) {
        return -2;
    }
    
    // Clear storage based on type
    int result = 0;
    
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            // Clear directory for EEPROM
            memset(&s_directory, 0, sizeof(s_directory));
            s_directory.magic = DIRECTORY_MAGIC;
            s_directory.version = DIRECTORY_VERSION;
            s_directory.keyCount = 0;
            result = save_directory();
            break;
            
        case STORAGE_TYPE_FLASH:
            // Clear directory for flash
            memset(&s_directory, 0, sizeof(s_directory));
            s_directory.magic = DIRECTORY_MAGIC;
            s_directory.version = DIRECTORY_VERSION;
            s_directory.keyCount = 0;
            result = save_directory();
            break;
            
        case STORAGE_TYPE_SD_CARD:
            // Would need to delete all files
            break;
            
        case STORAGE_TYPE_FILE_SYSTEM:
            // Would need to delete all files
            break;
            
        case STORAGE_TYPE_NVS:
#ifdef ESP32
            // Erase NVS partition
            if (s_context.handle != NULL) {
                nvs_close((nvs_handle_t)s_context.handle);
                s_context.handle = NULL;
                
                // Erase partition
                const char* partition = s_config.partition ? s_config.partition : "nvs";
                result = nvs_flash_erase_partition(partition);
                
                // Reinitialize NVS
                if (result == 0) {
                    result = storage_init_nvs(&s_config);
                }
            }
#endif
            break;
            
        default:
            result = -3; // Unknown storage type
    }
    
    return result;
}

/**
 * @brief Get free space in persistent storage
 */
int persistent_storage_get_free_space(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // Calculate free space based on storage type
    int freeSpace = 0;
    
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
            // Calculate free space for EEPROM
            if (load_directory() == 0) {
                uint32_t usedSpace = DIRECTORY_SIZE;
                for (uint32_t i = 0; i < s_directory.keyCount; i++) {
                    usedSpace += s_directory.entries[i].size;
                }
                freeSpace = s_config.size - usedSpace;
                if (freeSpace < 0) {
                    freeSpace = 0;
                }
            }
            break;
            
        case STORAGE_TYPE_FLASH:
            // Calculate free space for flash
            if (load_directory() == 0) {
                uint32_t usedSpace = DIRECTORY_SIZE;
                for (uint32_t i = 0; i < s_directory.keyCount; i++) {
                    usedSpace += s_directory.entries[i].size;
                }
                freeSpace = s_config.size - usedSpace;
                if (freeSpace < 0) {
                    freeSpace = 0;
                }
            }
            break;
            
        case STORAGE_TYPE_SD_CARD:
            // Would need to check SD card free space
            freeSpace = -2; // Not implemented
            break;
            
        case STORAGE_TYPE_FILE_SYSTEM:
            // Would need to check file system free space
            freeSpace = -2; // Not implemented
            break;
            
        case STORAGE_TYPE_NVS:
            // No easy way to get free space in NVS
            freeSpace = -2; // Not implemented
            break;
            
        default:
            freeSpace = -3; // Unknown storage type
    }
    
    return freeSpace;
}

/**
 * @brief Get total space in persistent storage
 */
int persistent_storage_get_total_space(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // Return total space based on storage type
    switch (s_context.type) {
        case STORAGE_TYPE_EEPROM:
        case STORAGE_TYPE_FLASH:
            return s_config.size;
            
        case STORAGE_TYPE_SD_CARD:
        case STORAGE_TYPE_FILE_SYSTEM:
        case STORAGE_TYPE_NVS:
            // Would need to check total space
            return -2; // Not implemented
            
        default:
            return -3; // Unknown storage type
    }
}

/**
 * @brief Begin a transaction
 */
int persistent_storage_begin_transaction(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // Check if already in transaction
    if (s_context.transaction_active) {
        return -2;
    }
    
    s_context.transaction_active = true;
    return 0;
}

/**
 * @brief End a transaction and commit changes
 */
int persistent_storage_end_transaction(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // Check if in transaction
    if (!s_context.transaction_active) {
        return -2;
    }
    
    // Commit changes
    int result = persistent_storage_commit();
    s_context.transaction_active = false;
    
    return result;
}

/**
 * @brief Set storage compression
 */
int persistent_storage_set_compression(bool enable) {
    if (!s_initialized) {
        return -1;
    }
    
    s_context.compression_enabled = enable;
    return 0;
}

// ===== Platform-specific implementations =====

// --- EEPROM storage implementation ---
static int storage_init_eeprom(const StorageConfig* config) {
    // Allocate memory-backed storage for testing
    s_memStorage = (uint8_t*)malloc(config->size);
    if (s_memStorage == NULL) {
        return -1;
    }
    
    // Clear storage
    memset(s_memStorage, 0xFF, config->size);
    
    // Initialize directory
    load_directory();
    
    return 0;
}

static int storage_write_eeprom(const char* key, const void* data, size_t size) {
    // Load directory if needed
    if (!s_directoryLoaded) {
        if (load_directory() != 0) {
            return -1;
        }
    }
    
    // Check if key already exists
    int keyIndex = find_key_entry(key);
    if (keyIndex >= 0) {
        // Key exists, check if we can overwrite in place
        if (s_directory.entries[keyIndex].size >= size) {
            // Can overwrite in place
            uint32_t address = s_directory.entries[keyIndex].address;
            memcpy(s_memStorage + address, data, size);
            s_directory.entries[keyIndex].size = size;
        } else {
            // Need to allocate new space
            storage_delete_eeprom(key);
            // Fall through to allocate new space
            keyIndex = -1;
        }
    }
    
    if (keyIndex < 0) {
        // Allocate space for data
        uint32_t address = allocate_storage(size);
        if (address == 0) {
            return -2; // Out of space
        }
        
        // Write data
        memcpy(s_memStorage + address, data, size);
        
        // Add key to directory
        if (add_key_entry(key, address, size) != 0) {
            return -3; // Directory full
        }
    }
    
    return 0;
}

static int storage_read_eeprom(const char* key, void* data, size_t maxSize, size_t* actualSize) {
    // Load directory if needed
    if (!s_directoryLoaded) {
        if (load_directory() != 0) {
            return -1;
        }
    }
    
    // Find key in directory
    int keyIndex = find_key_entry(key);
    if (keyIndex < 0) {
        return -2; // Key not found
    }
    
    // Read data
    uint32_t address = s_directory.entries[keyIndex].address;
    uint32_t size = s_directory.entries[keyIndex].size;
    
    *actualSize = size <= maxSize ? size : maxSize;
    memcpy(data, s_memStorage + address, *actualSize);
    
    return 0;
}

static bool storage_exists_eeprom(const char* key) {
    // Load directory if needed
    if (!s_directoryLoaded) {
        if (load_directory() != 0) {
            return false;
        }
    }
    
    return find_key_entry(key) >= 0;
}

static int storage_delete_eeprom(const char* key) {
    // Load directory if needed
    if (!s_directoryLoaded) {
        if (load_directory() != 0) {
            return -1;
        }
    }
    
    // Find key in directory
    int keyIndex = find_key_entry(key);
    if (keyIndex < 0) {
        return -2; // Key not found
    }
    
    // Remove key from directory
    return remove_key_entry(key);
}

static int storage_get_keys_eeprom(char** keys, size_t maxKeys) {
    // Load directory if needed
    if (!s_directoryLoaded) {
        if (load_directory() != 0) {
            return -1;
        }
    }
    
    // Copy keys
    size_t count = 0;
    for (uint32_t i = 0; i < s_directory.keyCount && count < maxKeys; i++) {
        keys[count] = strdup(s_directory.entries[i].key);
        if (keys[count] != NULL) {
            count++;
        }
    }
    
    return count;
}

static int storage_get_size_eeprom(const char* key) {
    // Load directory if needed
    if (!s_directoryLoaded) {
        if (load_directory() != 0) {
            return -1;
        }
    }
    
    // Find key in directory
    int keyIndex = find_key_entry(key);
    if (keyIndex < 0) {
        return -2; // Key not found
    }
    
    return s_directory.entries[keyIndex].size;
}

// --- Flash storage implementation ---
// For the sake of this implementation, we'll reuse the EEPROM implementation
// In a real implementation, these would use platform-specific flash APIs
static int storage_init_flash(const StorageConfig* config) {
    return storage_init_eeprom(config);
}

static int storage_write_flash(const char* key, const void* data, size_t size) {
    return storage_write_eeprom(key, data, size);
}

static int storage_read_flash(const char* key, void* data, size_t maxSize, size_t* actualSize) {
    return storage_read_eeprom(key, data, maxSize, actualSize);
}

static bool storage_exists_flash(const char* key) {
    return storage_exists_eeprom(key);
}

static int storage_delete_flash(const char* key) {
    return storage_delete_eeprom(key);
}

static int storage_get_keys_flash(char** keys, size_t maxKeys) {
    return storage_get_keys_eeprom(keys, maxKeys);
}

static int storage_get_size_flash(const char* key) {
    return storage_get_size_eeprom(key);
}

// --- SD Card storage implementation ---
static int storage_init_sd_card(const StorageConfig* config) {
    // In a real implementation, this would initialize SD card
    // For now, reuse memory-backed storage
    return storage_init_eeprom(config);
}

static int storage_write_sd_card(const char* key, const void* data, size_t size) {
    // In a real implementation, this would write to a file on SD card
    return storage_write_eeprom(key, data, size);
}

static int storage_read_sd_card(const char* key, void* data, size_t maxSize, size_t* actualSize) {
    // In a real implementation, this would read from a file on SD card
    return storage_read_eeprom(key, data, maxSize, actualSize);
}

static bool storage_exists_sd_card(const char* key) {
    // In a real implementation, this would check if file exists on SD card
    return storage_exists_eeprom(key);
}

static int storage_delete_sd_card(const char* key) {
    // In a real implementation, this would delete a file on SD card
    return storage_delete_eeprom(key);
}

static int storage_get_keys_sd_card(char** keys, size_t maxKeys) {
    // In a real implementation, this would list files on SD card
    return storage_get_keys_eeprom(keys, maxKeys);
}

static int storage_get_size_sd_card(const char* key) {
    // In a real implementation, this would get file size on SD card
    return storage_get_size_eeprom(key);
}

// --- File system storage implementation ---
static int storage_init_file_system(const StorageConfig* config) {
    // In a real implementation, this would initialize file system
    // For now, reuse memory-backed storage
    return storage_init_eeprom(config);
}

static int storage_write_file_system(const char* key, const void* data, size_t size) {
    // In a real implementation, this would write to a file
    return storage_write_eeprom(key, data, size);
}

static int storage_read_file_system(const char* key, void* data, size_t maxSize, size_t* actualSize) {
    // In a real implementation, this would read from a file
    return storage_read_eeprom(key, data, maxSize, actualSize);
}

static bool storage_exists_file_system(const char* key) {
    // In a real implementation, this would check if file exists
    return storage_exists_eeprom(key);
}

static int storage_delete_file_system(const char* key) {
    // In a real implementation, this would delete a file
    return storage_delete_eeprom(key);
}

static int storage_get_keys_file_system(char** keys, size_t maxKeys) {
    // In a real implementation, this would list files
    return storage_get_keys_eeprom(keys, maxKeys);
}

static int storage_get_size_file_system(const char* key) {
    // In a real implementation, this would get file size
    return storage_get_size_eeprom(key);
}

// --- NVS storage implementation ---
static int storage_init_nvs(const StorageConfig* config) {
#ifdef ESP32
    // Initialize NVS
    const char* partition = config->partition ? config->partition : "nvs";
    esp_err_t err = nvs_flash_init_partition(partition);
    
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        if (config->formatIfMounted) {
            // Erase partition
            err = nvs_flash_erase_partition(partition);
            if (err != ESP_OK) {
                return -1;
            }
            
            // Try again
            err = nvs_flash_init_partition(partition);
            if (err != ESP_OK) {
                return -2;
            }
        } else {
            return -3;
        }
    } else if (err != ESP_OK) {
        return -4;
    }
    
    // Open NVS handle
    nvs_handle_t handle;
    err = nvs_open_from_partition(partition, "storage", 
                                  config->readOnly ? NVS_READONLY : NVS_READWRITE, 
                                  &handle);
    if (err != ESP_OK) {
        return -5;
    }
    
    s_context.handle = (void*)handle;
    return 0;
#else
    // ESP32 not supported, fall back to memory-backed storage
    return storage_init_eeprom(config);
#endif
}

static int storage_write_nvs(const char* key, const void* data, size_t size) {
#ifdef ESP32
    if (s_context.handle == NULL) {
        return -1;
    }
    
    // Write blob to NVS
    esp_err_t err = nvs_set_blob((nvs_handle_t)s_context.handle, key, data, size);
    if (err != ESP_OK) {
        return -2;
    }
    
    return 0;
#else
    // ESP32 not supported, fall back to memory-backed storage
    return storage_write_eeprom(key, data, size);
#endif
}

static int storage_read_nvs(const char* key, void* data, size_t maxSize, size_t* actualSize) {
#ifdef ESP32
    if (s_context.handle == NULL) {
        return -1;
    }
    
    // Get blob size
    size_t size = 0;
    esp_err_t err = nvs_get_blob((nvs_handle_t)s_context.handle, key, NULL, &size);
    if (err != ESP_OK) {
        return -2;
    }
    
    // Check if buffer is large enough
    if (size > maxSize) {
        // Buffer too small, still read what we can
        *actualSize = maxSize;
    } else {
        *actualSize = size;
    }
    
    // Read blob
    err = nvs_get_blob((nvs_handle_t)s_context.handle, key, data, actualSize);
    if (err != ESP_OK) {
        return -3;
    }
    
    return 0;
#else
    // ESP32 not supported, fall back to memory-backed storage
    return storage_read_eeprom(key, data, maxSize, actualSize);
#endif
}

static bool storage_exists_nvs(const char* key) {
#ifdef ESP32
    if (s_context.handle == NULL) {
        return false;
    }
    
    // Check if key exists
    size_t size = 0;
    esp_err_t err = nvs_get_blob((nvs_handle_t)s_context.handle, key, NULL, &size);
    
    return err == ESP_OK;
#else
    // ESP32 not supported, fall back to memory-backed storage
    return storage_exists_eeprom(key);
#endif
}

static int storage_delete_nvs(const char* key) {
#ifdef ESP32
    if (s_context.handle == NULL) {
        return -1;
    }
    
    // Erase key
    esp_err_t err = nvs_erase_key((nvs_handle_t)s_context.handle, key);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        return -2;
    }
    
    return 0;
#else
    // ESP32 not supported, fall back to memory-backed storage
    return storage_delete_eeprom(key);
#endif
}

static int storage_get_keys_nvs(char** keys, size_t maxKeys) {
#ifdef ESP32
    // NVS doesn't provide a way to list keys
    // As a workaround, we could store a special key with a list of all keys
    // For now, return not implemented
    return -1;
#else
    // ESP32 not supported, fall back to memory-backed storage
    return storage_get_keys_eeprom(keys, maxKeys);
#endif
}

static int storage_get_size_nvs(const char* key) {
#ifdef ESP32
    if (s_context.handle == NULL) {
        return -1;
    }
    
    // Get blob size
    size_t size = 0;
    esp_err_t err = nvs_get_blob((nvs_handle_t)s_context.handle, key, NULL, &size);
    if (err != ESP_OK) {
        return -2;
    }
    
    return size;
#else
    // ESP32 not supported, fall back to memory-backed storage
    return storage_get_size_eeprom(key);
#endif
}

// ===== Helper functions =====

/**
 * @brief Load key directory from storage
 */
static int load_directory(void) {
    if (s_memStorage == NULL) {
        return -1;
    }
    
    // Read directory
    memcpy(&s_directory, s_memStorage + DIRECTORY_ADDRESS, DIRECTORY_SIZE);
    
    // Check magic number
    if (s_directory.magic != DIRECTORY_MAGIC) {
        // Initialize directory
        memset(&s_directory, 0, sizeof(s_directory));
        s_directory.magic = DIRECTORY_MAGIC;
        s_directory.version = DIRECTORY_VERSION;
        s_directory.keyCount = 0;
        
        // Write directory
        if (save_directory() != 0) {
            return -2;
        }
    }
    
    s_directoryLoaded = true;
    return 0;
}

/**
 * @brief Save key directory to storage
 */
static int save_directory(void) {
    if (s_memStorage == NULL) {
        return -1;
    }
    
    // Write directory
    memcpy(s_memStorage + DIRECTORY_ADDRESS, &s_directory, DIRECTORY_SIZE);
    
    return 0;
}

/**
 * @brief Find key entry in directory
 */
static int find_key_entry(const char* key) {
    for (uint32_t i = 0; i < s_directory.keyCount; i++) {
        if (strcmp(s_directory.entries[i].key, key) == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * @brief Add key entry to directory
 */
static int add_key_entry(const char* key, uint32_t address, uint32_t size) {
    // Check if directory is full
    if (s_directory.keyCount >= MAX_KEYS) {
        return -1;
    }
    
    // Check if key already exists
    int keyIndex = find_key_entry(key);
    if (keyIndex >= 0) {
        // Update existing entry
        s_directory.entries[keyIndex].address = address;
        s_directory.entries[keyIndex].size = size;
    } else {
        // Add new entry
        keyIndex = s_directory.keyCount;
        strncpy(s_directory.entries[keyIndex].key, key, MAX_KEY_LENGTH - 1);
        s_directory.entries[keyIndex].key[MAX_KEY_LENGTH - 1] = '\0';
        s_directory.entries[keyIndex].address = address;
        s_directory.entries[keyIndex].size = size;
        s_directory.keyCount++;
    }
    
    return 0;
}

/**
 * @brief Remove key entry from directory
 */
static int remove_key_entry(const char* key) {
    // Find key
    int keyIndex = find_key_entry(key);
    if (keyIndex < 0) {
        return -1;
    }
    
    // Remove entry by shifting all entries after it
    for (uint32_t i = keyIndex; i < s_directory.keyCount - 1; i++) {
        memcpy(&s_directory.entries[i], &s_directory.entries[i + 1], sizeof(KeyEntry));
    }
    
    s_directory.keyCount--;
    
    return 0;
}

/**
 * @brief Allocate storage space
 */
static uint32_t allocate_storage(size_t size) {
    if (s_memStorage == NULL) {
        return 0;
    }
    
    // Start after directory
    uint32_t address = DIRECTORY_SIZE;
    
    // Find a free block
    for (uint32_t i = 0; i < s_directory.keyCount; i++) {
        // Check if there's enough space before this entry
        if (s_directory.entries[i].address > address && 
            s_directory.entries[i].address - address >= size) {
            // Found a free block
            return address;
        }
        
        // Move past this entry
        address = s_directory.entries[i].address + s_directory.entries[i].size;
    }
    
    // Check if there's enough space at the end
    if (address + size <= s_config.size) {
        return address;
    }
    
    // No space found
    return 0;
}

/**
 * @brief Compress data using RLE
 */
static size_t compress_data(const void* data, size_t size, void* compressedData, size_t maxCompressedSize) {
    const uint8_t* src = (const uint8_t*)data;
    uint8_t* dst = (uint8_t*)compressedData;
    
    // Add compression marker
    if (maxCompressedSize < 2) {
        return 0;
    }
    
    dst[0] = 0xAB;
    dst[1] = 0xCD;
    
    size_t srcPos = 0;
    size_t dstPos = 2;
    
    while (srcPos < size) {
        // Check if we have enough space for worst case (2 bytes)
        if (dstPos + 2 > maxCompressedSize) {
            return 0; // Not enough space
        }
        
        uint8_t currentByte = src[srcPos];
        uint8_t count = 1;
        
        // Count repeating bytes
        while (srcPos + count < size && 
               src[srcPos + count] == currentByte && 
               count < 255) {
            count++;
        }
        
        if (count >= 3) {
            // Use RLE for 3 or more repeating bytes
            dst[dstPos++] = 0; // Control byte
            dst[dstPos++] = count;
            dst[dstPos++] = currentByte;
            srcPos += count;
        } else {
            // Use literal for 1 or 2 bytes
            uint8_t literalCount = 0;
            size_t literalStart = srcPos;
            
            // Count non-repeating bytes
            while (srcPos < size && literalCount < 255) {
                currentByte = src[srcPos];
                
                // Look ahead for repeating bytes
                uint8_t repeatCount = 0;
                while (srcPos + repeatCount + 1 < size && 
                       src[srcPos + repeatCount + 1] == currentByte && 
                       repeatCount < 255) {
                    repeatCount++;
                }
                
                if (repeatCount >= 2) {
                    // Found a repeating sequence, stop literal
                    break;
                }
                
                srcPos++;
                literalCount++;
            }
            
            // Check if we have enough space
            if (dstPos + 2 + literalCount > maxCompressedSize) {
                return 0; // Not enough space
            }
            
            // Write literal
            dst[dstPos++] = 1; // Control byte
            dst[dstPos++] = literalCount;
            memcpy(dst + dstPos, src + literalStart, literalCount);
            dstPos += literalCount;
        }
    }
    
    return dstPos;
}

/**
 * @brief Decompress data using RLE
 */
static size_t decompress_data(const void* compressedData, size_t compressedSize, void* data, size_t maxSize) {
    const uint8_t* src = (const uint8_t*)compressedData;
    uint8_t* dst = (uint8_t*)data;
    
    // Check compression marker
    if (compressedSize < 2 || src[0] != 0xAB || src[1] != 0xCD) {
        return 0;
    }
    
    size_t srcPos = 2;
    size_t dstPos = 0;
    
    while (srcPos < compressedSize) {
        uint8_t controlByte = src[srcPos++];
        
        if (controlByte == 0) {
            // RLE block
            if (srcPos + 1 >= compressedSize) {
                return 0; // Invalid compressed data
            }
            
            uint8_t count = src[srcPos++];
            uint8_t value = src[srcPos++];
            
            // Check if we have enough space
            if (dstPos + count > maxSize) {
                return 0; // Not enough space
            }
            
            // Repeat byte
            for (uint8_t i = 0; i < count; i++) {
                dst[dstPos++] = value;
            }
        } else if (controlByte == 1) {
            // Literal block
            if (srcPos >= compressedSize) {
                return 0; // Invalid compressed data
            }
            
            uint8_t count = src[srcPos++];
            
            // Check if we have enough space
            if (dstPos + count > maxSize || srcPos + count > compressedSize) {
                return 0; // Not enough space
            }
            
            // Copy literal bytes
            memcpy(dst + dstPos, src + srcPos, count);
            dstPos += count;
            srcPos += count;
        } else {
            return 0; // Invalid control byte
        }
    }
    
    return dstPos;
}