#ifndef IO_MAPPER_H
#define IO_MAPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief IO value types
 */
typedef enum {
    IO_VALUE_TYPE_BOOL,
    IO_VALUE_TYPE_INT,
    IO_VALUE_TYPE_FLOAT,
    IO_VALUE_TYPE_STRING
} IOValueType;

/**
 * @brief IO value union
 */
typedef union {
    bool boolValue;
    int32_t intValue;
    float floatValue;
    char* stringValue;
} IOValueData;

/**
 * @brief IO value structure
 */
typedef struct {
    IOValueType type;
    IOValueData value;
} IOValue;

/**
 * @brief IO mapping types
 */
typedef enum {
    IO_MAPPING_TYPE_DIGITAL_INPUT,
    IO_MAPPING_TYPE_DIGITAL_OUTPUT,
    IO_MAPPING_TYPE_ANALOG_INPUT,
    IO_MAPPING_TYPE_ANALOG_OUTPUT,
    IO_MAPPING_TYPE_PWM_OUTPUT,
    IO_MAPPING_TYPE_I2C_DEVICE,
    IO_MAPPING_TYPE_SPI_DEVICE,
    IO_MAPPING_TYPE_UART_DEVICE,
    IO_MAPPING_TYPE_VIRTUAL
} IOMappingType;

/**
 * @brief IO mapping configuration
 */
typedef struct {
    char* id;                   // Mapping ID
    IOMappingType type;         // Mapping type
    char* pin;                  // Pin identifier (can be NULL for virtual)
    uint32_t instance;          // Interface instance (for I2C, SPI, UART)
    uint32_t address;           // Device address (for I2C)
    void* config;               // Additional configuration
    bool initialized;           // Initialization state
} IOMapping;

/**
 * @brief Initialize the IO mapper
 * 
 * @return bool True on success, false on failure
 */
bool io_mapper_init(void);

/**
 * @brief Create IO mappings from JSON configuration
 * 
 * @param json JSON configuration string
 * @param length JSON string length
 * @return bool True on success, false on failure
 */
bool io_mapper_map_from_json(const char* json, size_t length);

/**
 * @brief Read from a mapped IO
 * 
 * @param id Mapping ID
 * @param value Pointer to store read value
 * @return bool True on success, false on failure
 */
bool io_mapper_read(const char* id, IOValue* value);

/**
 * @brief Write to a mapped IO
 * 
 * @param id Mapping ID
 * @param value Value to write
 * @return bool True on success, false on failure
 */
bool io_mapper_write(const char* id, const IOValue* value);

/**
 * @brief Get IO mapping by ID
 * 
 * @param id Mapping ID
 * @return const IOMapping* Mapping configuration or NULL if not found
 */
const IOMapping* io_mapper_get_mapping(const char* id);

/**
 * @brief Get list of IO mappings
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int io_mapper_get_mappings(char* buffer, size_t bufferSize);

/**
 * @brief Add a new IO mapping
 * 
 * @param id Mapping ID
 * @param type Mapping type
 * @param pin Pin identifier
 * @param config Additional configuration (JSON)
 * @return bool True on success, false on failure
 */
bool io_mapper_add_mapping(const char* id, IOMappingType type, const char* pin, const char* config);

/**
 * @brief Remove an IO mapping
 * 
 * @param id Mapping ID
 * @return bool True on success, false on failure
 */
bool io_mapper_remove_mapping(const char* id);

/**
 * @brief Create IO value from different types
 */
IOValue io_value_create_bool(bool value);
IOValue io_value_create_int(int32_t value);
IOValue io_value_create_float(float value);
IOValue io_value_create_string(const char* value);

/**
 * @brief Free IO value resources if needed
 * 
 * @param value IO value to free
 */
void io_value_free(IOValue* value);

#endif /* IO_MAPPER_H */