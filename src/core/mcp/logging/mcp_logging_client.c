#include "mcp_logging_client.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Static client configuration
static MCP_LogClientConfig s_config = {
    .enabled = false,
    .level = LOG_LEVEL_INFO,
    .output = MCP_LOG_OUTPUT_CONSOLE,
    .filePath = NULL,
    .maxFileSize = 1024 * 1024, // 1MB
    .maxMemoryEntries = 1000,
    .includeTimestamp = true,
    .includeLevelName = true,
    .includeModuleName = true,
    .colorOutput = true,
    .customHandler = NULL
};

// Memory buffer for logs
typedef struct {
    LogLevel level;
    char module[32];
    char message[256];
    uint32_t timestamp;
} LogEntry;

static LogEntry* s_memoryBuffer = NULL;
static uint32_t s_memoryBufferSize = 0;
static uint32_t s_memoryBufferHead = 0;
static uint32_t s_memoryBufferCount = 0;
static FILE* s_logFile = NULL;

// Forward declarations
static void output_log_to_console(LogLevel level, const char* module, 
                                const char* message, uint32_t timestamp);
static void output_log_to_file(LogLevel level, const char* module, 
                             const char* message, uint32_t timestamp);
static void output_log_to_memory(LogLevel level, const char* module, 
                               const char* message, uint32_t timestamp);

/**
 * @brief Initialize client log configuration
 */
int MCP_LogClientInit(const MCP_LogClientConfig* config) {
    // Free any existing memory buffer
    if (s_memoryBuffer != NULL) {
        free(s_memoryBuffer);
        s_memoryBuffer = NULL;
        s_memoryBufferSize = 0;
        s_memoryBufferHead = 0;
        s_memoryBufferCount = 0;
    }
    
    // Close log file if open
    if (s_logFile != NULL) {
        fclose(s_logFile);
        s_logFile = NULL;
    }
    
    // Use default config if NULL
    if (config == NULL) {
        config = &s_config;
    } else {
        // Copy config
        memcpy(&s_config, config, sizeof(MCP_LogClientConfig));
        
        // Copy file path if set
        if (config->filePath != NULL) {
            s_config.filePath = strdup(config->filePath);
        }
    }
    
    // Initialize memory buffer if needed
    if (s_config.output == MCP_LOG_OUTPUT_MEMORY && s_config.maxMemoryEntries > 0) {
        s_memoryBufferSize = s_config.maxMemoryEntries;
        s_memoryBuffer = (LogEntry*)calloc(s_memoryBufferSize, sizeof(LogEntry));
        if (s_memoryBuffer == NULL) {
            return -1;
        }
    }
    
    // Open log file if needed
    if (s_config.output == MCP_LOG_OUTPUT_FILE && s_config.filePath != NULL) {
        s_logFile = fopen(s_config.filePath, "a");
        if (s_logFile == NULL) {
            return -2;
        }
    }
    
    return 0;
}

/**
 * @brief Get current client log configuration
 */
int MCP_LogClientGetConfig(MCP_LogClientConfig* config) {
    if (config == NULL) {
        return -1;
    }
    
    // Copy config
    memcpy(config, &s_config, sizeof(MCP_LogClientConfig));
    
    // Copy file path if set
    if (s_config.filePath != NULL) {
        config->filePath = strdup(s_config.filePath);
    } else {
        config->filePath = NULL;
    }
    
    return 0;
}

/**
 * @brief Set client log configuration
 */
int MCP_LogClientSetConfig(const MCP_LogClientConfig* config) {
    if (config == NULL) {
        return -1;
    }
    
    return MCP_LogClientInit(config);
}

/**
 * @brief Enable or disable client logging
 */
bool MCP_LogClientEnable(bool enable) {
    bool prevState = s_config.enabled;
    s_config.enabled = enable;
    return prevState;
}

/**
 * @brief Set client log level
 */
LogLevel MCP_LogClientSetLevel(LogLevel level) {
    LogLevel prevLevel = s_config.level;
    s_config.level = level;
    return prevLevel;
}

/**
 * @brief Set client log output destination
 */
MCP_LogOutputDest MCP_LogClientSetOutput(MCP_LogOutputDest output) {
    MCP_LogOutputDest prevOutput = s_config.output;
    
    // If changing output type, reinitialize
    if (prevOutput != output) {
        s_config.output = output;
        MCP_LogClientInit(&s_config);
    }
    
    return prevOutput;
}

/**
 * @brief Set custom log handler
 */
int MCP_LogClientSetCustomHandler(void (*handler)(LogLevel level, const char* module,
                                               const char* message, uint32_t timestamp)) {
    s_config.customHandler = handler;
    return 0;
}

/**
 * @brief Process incoming log event from server
 */
int MCP_LogClientProcessEvent(LogLevel level, const char* module,
                           const char* message, uint32_t timestamp) {
    if (!s_config.enabled || level > s_config.level) {
        return 0; // Skip if disabled or level too high
    }
    
    // Process based on output type
    switch (s_config.output) {
        case MCP_LOG_OUTPUT_CONSOLE:
            output_log_to_console(level, module, message, timestamp);
            break;
            
        case MCP_LOG_OUTPUT_FILE:
            output_log_to_file(level, module, message, timestamp);
            break;
            
        case MCP_LOG_OUTPUT_MEMORY:
            output_log_to_memory(level, module, message, timestamp);
            break;
            
        case MCP_LOG_OUTPUT_CUSTOM:
            if (s_config.customHandler != NULL) {
                s_config.customHandler(level, module, message, timestamp);
            }
            break;
    }
    
    return 0;
}

/**
 * @brief Get default client log configuration
 */
MCP_LogClientConfig MCP_LogClientGetDefaultConfig(void) {
    MCP_LogClientConfig config = {
        .enabled = true,
        .level = LOG_LEVEL_INFO,
        .output = MCP_LOG_OUTPUT_CONSOLE,
        .filePath = NULL,
        .maxFileSize = 1024 * 1024, // 1MB
        .maxMemoryEntries = 1000,
        .includeTimestamp = true,
        .includeLevelName = true,
        .includeModuleName = true,
        .colorOutput = true,
        .customHandler = NULL
    };
    
    return config;
}

/**
 * @brief Output log to console
 */
static void output_log_to_console(LogLevel level, const char* module, 
                               const char* message, uint32_t timestamp) {
    // ANSI color codes for different log levels
    const char* colors[] = {
        "\033[0m",    // NONE - Reset
        "\033[31m",   // ERROR - Red
        "\033[33m",   // WARN - Yellow
        "\033[32m",   // INFO - Green
        "\033[36m",   // DEBUG - Cyan
        "\033[35m"    // TRACE - Magenta
    };
    
    const char* levelNames[] = {
        "NONE",
        "ERROR",
        "WARN",
        "INFO",
        "DEBUG",
        "TRACE"
    };
    
    // Format timestamp
    char timeStr[32] = "";
    if (s_config.includeTimestamp) {
        uint32_t ms = timestamp % 1000;
        uint32_t sec = timestamp / 1000;
        uint32_t min = sec / 60 % 60;
        uint32_t hour = sec / 3600 % 24;
        
        sprintf(timeStr, "%02u:%02u:%02u.%03u ", hour, min, sec % 60, ms);
    }
    
    // Format level
    char levelStr[16] = "";
    if (s_config.includeLevelName) {
        if (level >= 0 && level <= LOG_LEVEL_TRACE) {
            sprintf(levelStr, "[%s] ", levelNames[level]);
        } else {
            sprintf(levelStr, "[LVL%d] ", level);
        }
    }
    
    // Format module
    char moduleStr[48] = "";
    if (s_config.includeModuleName && module != NULL) {
        sprintf(moduleStr, "[%s] ", module);
    }
    
    // Output with or without color
    if (s_config.colorOutput) {
        const char* color = colors[level >= 0 && level <= LOG_LEVEL_TRACE ? level : 0];
        fprintf(stdout, "%s%s%s%s%s\033[0m\n", 
                color, timeStr, levelStr, moduleStr, message);
    } else {
        fprintf(stdout, "%s%s%s%s\n", 
                timeStr, levelStr, moduleStr, message);
    }
    
    fflush(stdout);
}

/**
 * @brief Output log to file
 */
static void output_log_to_file(LogLevel level, const char* module, 
                            const char* message, uint32_t timestamp) {
    if (s_logFile == NULL) {
        return;
    }
    
    const char* levelNames[] = {
        "NONE",
        "ERROR",
        "WARN",
        "INFO",
        "DEBUG",
        "TRACE"
    };
    
    // Format timestamp
    char timeStr[32] = "";
    if (s_config.includeTimestamp) {
        uint32_t ms = timestamp % 1000;
        uint32_t sec = timestamp / 1000;
        uint32_t min = sec / 60 % 60;
        uint32_t hour = sec / 3600 % 24;
        
        sprintf(timeStr, "%02u:%02u:%02u.%03u ", hour, min, sec % 60, ms);
    }
    
    // Format level
    char levelStr[16] = "";
    if (s_config.includeLevelName) {
        if (level >= 0 && level <= LOG_LEVEL_TRACE) {
            sprintf(levelStr, "[%s] ", levelNames[level]);
        } else {
            sprintf(levelStr, "[LVL%d] ", level);
        }
    }
    
    // Format module
    char moduleStr[48] = "";
    if (s_config.includeModuleName && module != NULL) {
        sprintf(moduleStr, "[%s] ", module);
    }
    
    // Write to file
    fprintf(s_logFile, "%s%s%s%s\n", 
            timeStr, levelStr, moduleStr, message);
    
    fflush(s_logFile);
    
    // Check file size and rotate if needed
    long fileSize = ftell(s_logFile);
    if (fileSize > (long)s_config.maxFileSize) {
        // Close current file
        fclose(s_logFile);
        
        // Create backup file name
        char* backupPath = NULL;
        if (s_config.filePath != NULL) {
            backupPath = (char*)malloc(strlen(s_config.filePath) + 5);
            if (backupPath != NULL) {
                sprintf(backupPath, "%s.bak", s_config.filePath);
                
                // Remove old backup if exists
                remove(backupPath);
                
                // Rename current log to backup
                rename(s_config.filePath, backupPath);
                
                free(backupPath);
            }
        }
        
        // Open new log file
        s_logFile = fopen(s_config.filePath, "w");
    }
}

/**
 * @brief Output log to memory
 */
static void output_log_to_memory(LogLevel level, const char* module, 
                              const char* message, uint32_t timestamp) {
    if (s_memoryBuffer == NULL) {
        return;
    }
    
    // Get next entry index
    uint32_t index = s_memoryBufferHead;
    s_memoryBufferHead = (s_memoryBufferHead + 1) % s_memoryBufferSize;
    
    // Update count
    if (s_memoryBufferCount < s_memoryBufferSize) {
        s_memoryBufferCount++;
    }
    
    // Fill entry
    s_memoryBuffer[index].level = level;
    s_memoryBuffer[index].timestamp = timestamp;
    
    // Copy module name (truncate if too long)
    if (module != NULL) {
        strncpy(s_memoryBuffer[index].module, module, sizeof(s_memoryBuffer[index].module) - 1);
        s_memoryBuffer[index].module[sizeof(s_memoryBuffer[index].module) - 1] = '\0';
    } else {
        s_memoryBuffer[index].module[0] = '\0';
    }
    
    // Copy message (truncate if too long)
    if (message != NULL) {
        strncpy(s_memoryBuffer[index].message, message, sizeof(s_memoryBuffer[index].message) - 1);
        s_memoryBuffer[index].message[sizeof(s_memoryBuffer[index].message) - 1] = '\0';
    } else {
        s_memoryBuffer[index].message[0] = '\0';
    }
}