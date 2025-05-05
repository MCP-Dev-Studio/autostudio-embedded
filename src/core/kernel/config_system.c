#include "config_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declaration of persistent storage interface
extern int persistent_storage_write(const char* key, const void* data, size_t size);
extern int persistent_storage_read(const char* key, void* data, size_t maxSize, size_t* actualSize);

// Internal state
static MCP_ConfigEntry* s_entries = NULL;
static uint16_t s_maxEntries = 0;
static uint16_t s_entryCount = 0;
static bool s_initialized = false;

int MCP_ConfigInit(uint16_t maxEntries) {
    if (s_initialized) {
        return -1;  // Already initialized
    }
    
    // Allocate entry array
    s_entries = (MCP_ConfigEntry*)calloc(maxEntries, sizeof(MCP_ConfigEntry));
    if (s_entries == NULL) {
        return -2;  // Memory allocation failed
    }
    
    s_maxEntries = maxEntries;
    s_entryCount = 0;
    s_initialized = true;
    
    return 0;
}

static MCP_ConfigEntry* findEntry(const char* key) {
    if (!s_initialized || key == NULL) {
        return NULL;
    }
    
    for (uint16_t i = 0; i < s_maxEntries; i++) {
        if (s_entries[i].key != NULL && strcmp(s_entries[i].key, key) == 0) {
            return &s_entries[i];
        }
    }
    
    return NULL;
}

static MCP_ConfigEntry* allocateEntry(const char* key) {
    if (!s_initialized || key == NULL) {
        return NULL;
    }
    
    // Check if entry already exists
    MCP_ConfigEntry* entry = findEntry(key);
    if (entry != NULL) {
        return entry;
    }
    
    // Check if we have space for a new entry
    if (s_entryCount >= s_maxEntries) {
        return NULL;  // No space for new entry
    }
    
    // Find free slot
    uint16_t i;
    for (i = 0; i < s_maxEntries; i++) {
        if (s_entries[i].key == NULL) {
            break;
        }
    }
    
    if (i >= s_maxEntries) {
        return NULL;  // No free slot found
    }
    
    // Allocate and copy key
    s_entries[i].key = strdup(key);
    if (s_entries[i].key == NULL) {
        return NULL;  // Memory allocation failed
    }
    
    s_entryCount++;
    
    return &s_entries[i];
}

static void freeEntryValue(MCP_ConfigEntry* entry) {
    if (entry == NULL) {
        return;
    }
    
    // Free string values
    if (entry->type == MCP_CONFIG_TYPE_STRING && entry->value.stringValue != NULL) {
        free(entry->value.stringValue);
        entry->value.stringValue = NULL;
    }
}

static void freeEntry(MCP_ConfigEntry* entry) {
    if (entry == NULL) {
        return;
    }
    
    // Free key and value
    if (entry->key != NULL) {
        free(entry->key);
        entry->key = NULL;
    }
    
    freeEntryValue(entry);
}

int MCP_ConfigSetBool(const char* key, bool value, bool persistent) {
    MCP_ConfigEntry* entry = allocateEntry(key);
    if (entry == NULL) {
        return -1;
    }
    
    // Free previous value if needed
    freeEntryValue(entry);
    
    // Set new value
    entry->type = MCP_CONFIG_TYPE_BOOL;
    entry->value.boolValue = value;
    entry->persistent = persistent;
    
    return 0;
}

int MCP_ConfigSetInt(const char* key, int32_t value, bool persistent) {
    MCP_ConfigEntry* entry = allocateEntry(key);
    if (entry == NULL) {
        return -1;
    }
    
    // Free previous value if needed
    freeEntryValue(entry);
    
    // Set new value
    entry->type = MCP_CONFIG_TYPE_INT;
    entry->value.intValue = value;
    entry->persistent = persistent;
    
    return 0;
}

int MCP_ConfigSetFloat(const char* key, float value, bool persistent) {
    MCP_ConfigEntry* entry = allocateEntry(key);
    if (entry == NULL) {
        return -1;
    }
    
    // Free previous value if needed
    freeEntryValue(entry);
    
    // Set new value
    entry->type = MCP_CONFIG_TYPE_FLOAT;
    entry->value.floatValue = value;
    entry->persistent = persistent;
    
    return 0;
}

int MCP_ConfigSetString(const char* key, const char* value, bool persistent) {
    if (value == NULL) {
        return -2;
    }
    
    MCP_ConfigEntry* entry = allocateEntry(key);
    if (entry == NULL) {
        return -1;
    }
    
    // Free previous value if needed
    freeEntryValue(entry);
    
    // Allocate and copy string value
    entry->type = MCP_CONFIG_TYPE_STRING;
    entry->value.stringValue = strdup(value);
    if (entry->value.stringValue == NULL) {
        return -3;  // Memory allocation failed
    }
    
    entry->persistent = persistent;
    
    return 0;
}

int MCP_ConfigSetObject(const char* key, void* value, bool persistent) {
    MCP_ConfigEntry* entry = allocateEntry(key);
    if (entry == NULL) {
        return -1;
    }
    
    // Free previous value if needed
    freeEntryValue(entry);
    
    // Set new value
    entry->type = MCP_CONFIG_TYPE_OBJECT;
    entry->value.objectValue = value;
    entry->persistent = persistent;
    
    return 0;
}

bool MCP_ConfigGetBool(const char* key, bool defaultValue) {
    MCP_ConfigEntry* entry = findEntry(key);
    if (entry == NULL || entry->type != MCP_CONFIG_TYPE_BOOL) {
        return defaultValue;
    }
    
    return entry->value.boolValue;
}

int32_t MCP_ConfigGetInt(const char* key, int32_t defaultValue) {
    MCP_ConfigEntry* entry = findEntry(key);
    if (entry == NULL || entry->type != MCP_CONFIG_TYPE_INT) {
        return defaultValue;
    }
    
    return entry->value.intValue;
}

float MCP_ConfigGetFloat(const char* key, float defaultValue) {
    MCP_ConfigEntry* entry = findEntry(key);
    if (entry == NULL || entry->type != MCP_CONFIG_TYPE_FLOAT) {
        return defaultValue;
    }
    
    return entry->value.floatValue;
}

const char* MCP_ConfigGetString(const char* key, const char* defaultValue) {
    MCP_ConfigEntry* entry = findEntry(key);
    if (entry == NULL || entry->type != MCP_CONFIG_TYPE_STRING || entry->value.stringValue == NULL) {
        return defaultValue;
    }
    
    return entry->value.stringValue;
}

void* MCP_ConfigGetObject(const char* key, void* defaultValue) {
    MCP_ConfigEntry* entry = findEntry(key);
    if (entry == NULL || entry->type != MCP_CONFIG_TYPE_OBJECT) {
        return defaultValue;
    }
    
    return entry->value.objectValue;
}

int MCP_ConfigRemove(const char* key) {
    MCP_ConfigEntry* entry = findEntry(key);
    if (entry == NULL) {
        return -1;  // Entry not found
    }
    
    // Free entry
    freeEntry(entry);
    s_entryCount--;
    
    return 0;
}

// Structure for persistent storage
typedef struct {
    char key[64];
    MCP_ConfigType type;
    union {
        bool boolValue;
        int32_t intValue;
        float floatValue;
        char stringValue[256];  // Fixed size for simplicity
    } value;
} StoredConfigEntry;

int MCP_ConfigSave(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // For each persistent entry, save to storage
    for (uint16_t i = 0; i < s_maxEntries; i++) {
        if (s_entries[i].key != NULL && s_entries[i].persistent) {
            StoredConfigEntry storedEntry;
            memset(&storedEntry, 0, sizeof(StoredConfigEntry));
            
            // Copy key and type
            strncpy(storedEntry.key, s_entries[i].key, sizeof(storedEntry.key) - 1);
            storedEntry.type = s_entries[i].type;
            
            // Copy value based on type
            switch (s_entries[i].type) {
                case MCP_CONFIG_TYPE_BOOL:
                    storedEntry.value.boolValue = s_entries[i].value.boolValue;
                    break;
                    
                case MCP_CONFIG_TYPE_INT:
                    storedEntry.value.intValue = s_entries[i].value.intValue;
                    break;
                    
                case MCP_CONFIG_TYPE_FLOAT:
                    storedEntry.value.floatValue = s_entries[i].value.floatValue;
                    break;
                    
                case MCP_CONFIG_TYPE_STRING:
                    if (s_entries[i].value.stringValue != NULL) {
                        strncpy(storedEntry.value.stringValue, s_entries[i].value.stringValue, 
                               sizeof(storedEntry.value.stringValue) - 1);
                    }
                    break;
                    
                case MCP_CONFIG_TYPE_OBJECT:
                    // Not saving objects for simplicity
                    break;
            }
            
            // Write to storage
            persistent_storage_write(s_entries[i].key, &storedEntry, sizeof(StoredConfigEntry));
        }
    }
    
    return 0;
}

int MCP_ConfigLoad(void) {
    if (!s_initialized) {
        return -1;
    }
    
    // Read all keys from persistent storage
    // This is a simplified version, in practice you'd need to know all keys
    // or have a directory of keys in the storage
    
    // For demonstration, we'll assume we have a list of keys to load
    const char* keysToLoad[] = {
        "deviceName",
        "version",
        "capabilities.tools",
        "capabilities.resources",
        "capabilities.events",
        "capabilities.automation",
        NULL
    };
    
    for (int i = 0; keysToLoad[i] != NULL; i++) {
        StoredConfigEntry storedEntry;
        size_t actualSize;
        
        int result = persistent_storage_read(keysToLoad[i], &storedEntry, sizeof(StoredConfigEntry), &actualSize);
        
        if (result == 0 && actualSize == sizeof(StoredConfigEntry)) {
            // Process based on type
            switch (storedEntry.type) {
                case MCP_CONFIG_TYPE_BOOL:
                    MCP_ConfigSetBool(storedEntry.key, storedEntry.value.boolValue, true);
                    break;
                    
                case MCP_CONFIG_TYPE_INT:
                    MCP_ConfigSetInt(storedEntry.key, storedEntry.value.intValue, true);
                    break;
                    
                case MCP_CONFIG_TYPE_FLOAT:
                    MCP_ConfigSetFloat(storedEntry.key, storedEntry.value.floatValue, true);
                    break;
                    
                case MCP_CONFIG_TYPE_STRING:
                    MCP_ConfigSetString(storedEntry.key, storedEntry.value.stringValue, true);
                    break;
                    
                case MCP_CONFIG_TYPE_OBJECT:
                    // Not loading objects for simplicity
                    break;
            }
        }
    }
    
    return 0;
}

// This is a minimal JSON implementation for simplicity
int MCP_ConfigExportJson(char* buffer, size_t bufferSize) {
    if (!s_initialized || buffer == NULL || bufferSize == 0) {
        return -1;
    }
    
    // Start JSON object
    int offset = 0;
    offset += snprintf(buffer + offset, bufferSize - offset, "{");
    
    // Add entries
    bool first = true;
    for (uint16_t i = 0; i < s_maxEntries; i++) {
        if (s_entries[i].key != NULL) {
            // Add comma if not first entry
            if (!first) {
                offset += snprintf(buffer + offset, bufferSize - offset, ",");
            }
            first = false;
            
            // Add key
            offset += snprintf(buffer + offset, bufferSize - offset, "\"%s\":", s_entries[i].key);
            
            // Add value based on type
            switch (s_entries[i].type) {
                case MCP_CONFIG_TYPE_BOOL:
                    offset += snprintf(buffer + offset, bufferSize - offset, 
                                    "%s", s_entries[i].value.boolValue ? "true" : "false");
                    break;
                    
                case MCP_CONFIG_TYPE_INT:
                    offset += snprintf(buffer + offset, bufferSize - offset, 
                                    "%d", s_entries[i].value.intValue);
                    break;
                    
                case MCP_CONFIG_TYPE_FLOAT:
                    offset += snprintf(buffer + offset, bufferSize - offset, 
                                    "%f", s_entries[i].value.floatValue);
                    break;
                    
                case MCP_CONFIG_TYPE_STRING:
                    if (s_entries[i].value.stringValue != NULL) {
                        offset += snprintf(buffer + offset, bufferSize - offset, 
                                        "\"%s\"", s_entries[i].value.stringValue);
                    } else {
                        offset += snprintf(buffer + offset, bufferSize - offset, "null");
                    }
                    break;
                    
                case MCP_CONFIG_TYPE_OBJECT:
                    offset += snprintf(buffer + offset, bufferSize - offset, "{}");
                    break;
            }
            
            // Check if we're about to overflow
            if ((size_t)offset >= bufferSize - 2) {
                return -2;  // Buffer too small
            }
        }
    }
    
    // End JSON object
    offset += snprintf(buffer + offset, bufferSize - offset, "}");
    
    return offset;
}

// This is a simplified version. In practice, you'd use a proper JSON parser
int MCP_ConfigImportJson(const char* json, size_t jsonLength) {
    // Mark parameters as unused to avoid compiler warnings
    (void)json;
    (void)jsonLength;
    
    // Not implemented for simplicity
    // In a real implementation, you'd parse the JSON and set config values
    // Return a simple "not implemented" code
    return -100;
}