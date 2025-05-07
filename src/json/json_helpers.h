#ifndef JSON_HELPERS_H
#define JSON_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Get a string field from a JSON string
 * 
 * @param json The JSON string
 * @param field The field name
 * @return char* The field value or NULL if not found (caller must free)
 */
char* json_get_string_field(const char* json, const char* field);

/**
 * @brief Get a boolean field from a JSON string
 * 
 * @param json The JSON string
 * @param field The field name
 * @param defaultValue The default value if field not found
 * @return bool The field value or default if not found
 */
bool json_get_bool_field(const char* json, const char* field, bool defaultValue);

/**
 * @brief Get an integer field from a JSON string
 * 
 * @param json The JSON string
 * @param field The field name
 * @param defaultValue The default value if field not found
 * @return int The field value or default if not found
 */
int json_get_int_field(const char* json, const char* field, int defaultValue);

/**
 * @brief Get a floating-point field from a JSON string
 * 
 * @param json The JSON string
 * @param field The field name
 * @param defaultValue The default value if field not found
 * @return double The field value or default if not found
 */
double json_get_double_field(const char* json, const char* field, double defaultValue);

/**
 * @brief Get an object field from a JSON string
 * 
 * @param json The JSON string
 * @param field The field name
 * @return void* Opaque pointer to the object or NULL if not found
 */
void* json_get_object_field(const char* json, const char* field);

/**
 * @brief Get an array field from a JSON string
 * 
 * @param json The JSON string
 * @param field The field name
 * @return void* Opaque pointer to the array or NULL if not found
 */
void* json_get_array_field(const char* json, const char* field);

/**
 * @brief Get the length of a JSON array
 * 
 * @param array Opaque pointer to the array
 * @return size_t The array length
 */
size_t json_array_length(const void* array);

/**
 * @brief Get an item from a JSON array
 * 
 * @param array Opaque pointer to the array
 * @param index The index of the item
 * @return char* The item value as a JSON string (caller must free)
 */
char* json_array_get_item(const void* array, size_t index);

/**
 * @brief Validate a JSON string against a schema
 * 
 * @param json The JSON string
 * @param schema The schema to validate against
 * @return bool true if valid, false otherwise
 */
bool json_validate_schema(const char* json, const char* schema);

#endif /* JSON_HELPERS_H */