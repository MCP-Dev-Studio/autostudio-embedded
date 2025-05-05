#ifndef TEST_MOCKS_H
#define TEST_MOCKS_H

// Mock declarations for functions used by tool_registry.c
size_t json_array_length(const void* array);
void* json_get_array_field(const char* json, const char* field);
void* json_get_object_field(const char* json, const char* field);
void* json_array_get_object(const void* array, size_t index);
bool json_get_bool_field(const char* json, const char* field, bool defaultValue);
bool json_validate_schema(const char* schema, const char* json);

#endif // TEST_MOCKS_H
