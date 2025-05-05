#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief JSON value types
 */
typedef enum {
    JSON_VALUE_NULL,
    JSON_VALUE_BOOL,
    JSON_VALUE_NUMBER,
    JSON_VALUE_STRING,
    JSON_VALUE_ARRAY,
    JSON_VALUE_OBJECT
} JSONValueType;

/**
 * @brief JSON value structure
 */
typedef struct JSONValue {
    JSONValueType type;
    union {
        bool boolValue;
        double numberValue;
        char* stringValue;
        struct {
            struct JSONValue* values;
            size_t count;
        } arrayValue;
        struct {
            char** keys;
            struct JSONValue* values;
            size_t count;
        } objectValue;
    } value;
} JSONValue;

/**
 * @brief Parse JSON string into value structure
 * 
 * @param json JSON string to parse
 * @param length JSON string length
 * @return JSONValue* Parsed JSON value or NULL on failure
 */
JSONValue* json_parse(const char* json, size_t length);

/**
 * @brief Free a JSON value structure
 * 
 * @param value JSON value to free
 */
void json_free(JSONValue* value);

/**
 * @brief Get JSON value as formatted string
 * 
 * @param value JSON value
 * @param buffer Buffer to store string
 * @param bufferSize Size of buffer
 * @param pretty Enable pretty formatting
 * @return int Number of bytes written or negative error code
 */
int json_stringify(const JSONValue* value, char* buffer, size_t bufferSize, bool pretty);

/**
 * @brief Get string field from JSON object
 * 
 * @param json JSON string
 * @param field Field name
 * @return char* Field value (caller must free) or NULL if not found
 */
char* json_get_string_field(const char* json, const char* field);

/**
 * @brief Get integer field from JSON object
 * 
 * @param json JSON string
 * @param field Field name
 * @param defaultValue Default value if field not found
 * @return int Field value or default value if not found
 */
int json_get_int_field(const char* json, const char* field, int defaultValue);

/**
 * @brief Get boolean field from JSON object
 * 
 * @param json JSON string
 * @param field Field name
 * @param defaultValue Default value if field not found
 * @return bool Field value or default value if not found
 */
bool json_get_bool_field(const char* json, const char* field, bool defaultValue);

/**
 * @brief Get float field from JSON object
 * 
 * @param json JSON string
 * @param field Field name
 * @param defaultValue Default value if field not found
 * @return float Field value or default value if not found
 */
float json_get_float_field(const char* json, const char* field, float defaultValue);

/**
 * @brief Get object field from JSON object
 * 
 * @param json JSON string
 * @param field Field name
 * @return void* Field value (opaque pointer) or NULL if not found
 */
void* json_get_object_field(const char* json, const char* field);

/**
 * @brief Get array field from JSON object
 * 
 * @param json JSON string
 * @param field Field name
 * @return void* Field value (opaque pointer) or NULL if not found
 */
void* json_get_array_field(const char* json, const char* field);

/**
 * @brief Check if field exists in JSON object
 * 
 * @param json JSON string
 * @param field Field name
 * @return bool True if field exists, false otherwise
 */
bool json_field_exists(const char* json, const char* field);

/**
 * @brief Get array length
 * 
 * @param array Array value (from json_get_array_field)
 * @return size_t Array length or 0 if invalid
 */
size_t json_array_length(const void* array);

/**
 * @brief Get array element as string
 * 
 * @param array Array value (from json_get_array_field)
 * @param index Element index
 * @return char* Element value (caller must free) or NULL if not found
 */
char* json_array_get_string(const void* array, size_t index);

/**
 * @brief Get array element as integer
 * 
 * @param array Array value (from json_get_array_field)
 * @param index Element index
 * @param defaultValue Default value if element not found
 * @return int Element value or default value if not found
 */
int json_array_get_int(const void* array, size_t index, int defaultValue);

/**
 * @brief Get array element as boolean
 * 
 * @param array Array value (from json_get_array_field)
 * @param index Element index
 * @param defaultValue Default value if element not found
 * @return bool Element value or default value if not found
 */
bool json_array_get_bool(const void* array, size_t index, bool defaultValue);

/**
 * @brief Get array element as float
 * 
 * @param array Array value (from json_get_array_field)
 * @param index Element index
 * @param defaultValue Default value if element not found
 * @return float Element value or default value if not found
 */
float json_array_get_float(const void* array, size_t index, float defaultValue);

/**
 * @brief Get array element as object
 * 
 * @param array Array value (from json_get_array_field)
 * @param index Element index
 * @return void* Element value (opaque pointer) or NULL if not found
 */
void* json_array_get_object(const void* array, size_t index);

/**
 * @brief Create a new JSON object
 * 
 * @return JSONValue* New JSON object or NULL on failure
 */
JSONValue* json_create_object(void);

/**
 * @brief Create a new JSON array
 * 
 * @return JSONValue* New JSON array or NULL on failure
 */
JSONValue* json_create_array(void);

/**
 * @brief Create a new JSON string value
 * 
 * @param value String value
 * @return JSONValue* New JSON string value or NULL on failure
 */
JSONValue* json_create_string(const char* value);

/**
 * @brief Create a new JSON number value
 * 
 * @param value Number value
 * @return JSONValue* New JSON number value or NULL on failure
 */
JSONValue* json_create_number(double value);

/**
 * @brief Create a new JSON boolean value
 * 
 * @param value Boolean value
 * @return JSONValue* New JSON boolean value or NULL on failure
 */
JSONValue* json_create_bool(bool value);

/**
 * @brief Create a new JSON null value
 * 
 * @return JSONValue* New JSON null value or NULL on failure
 */
JSONValue* json_create_null(void);

/**
 * @brief Add a field to a JSON object
 * 
 * @param object JSON object
 * @param key Field key
 * @param value Field value
 * @return int 0 on success, negative error code on failure
 */
int json_object_add(JSONValue* object, const char* key, JSONValue* value);

/**
 * @brief Add an element to a JSON array
 * 
 * @param array JSON array
 * @param value Element value
 * @return int 0 on success, negative error code on failure
 */
int json_array_add(JSONValue* array, JSONValue* value);

/**
 * @brief Validate JSON against a JSON schema
 * 
 * @param json JSON string
 * @param schema JSON schema string
 * @return bool True if valid, false otherwise
 */
bool json_validate_schema(const char* json, const char* schema);

#endif /* JSON_PARSER_H */