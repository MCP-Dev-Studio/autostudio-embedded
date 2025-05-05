cmake_minimum_required(VERSION 3.10)
project(MCP_Embedded_System C CXX)

# Set C/C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -DMCP_OS_ESP32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -DMCP_OS_ESP32")

# Define source directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CORE_DIR ${SRC_DIR}/core)
set(DRIVER_DIR ${SRC_DIR}/driver)
set(HAL_DIR ${SRC_DIR}/hal)
set(JSON_DIR ${SRC_DIR}/json)
set(SYSTEM_DIR ${SRC_DIR}/system)
set(TEST_DIR ${SRC_DIR}/test)
set(UTIL_DIR ${SRC_DIR}/util)

# Include directories
include_directories(
    ${SRC_DIR}
    ${CORE_DIR}
    ${DRIVER_DIR}
    ${HAL_DIR}
    ${JSON_DIR}
    ${SYSTEM_DIR}
    ${UTIL_DIR}
)

# Find all source files
file(GLOB_RECURSE CORE_SOURCES "${CORE_DIR}/*.c")
file(GLOB_RECURSE DRIVER_SOURCES "${DRIVER_DIR}/*.c")
file(GLOB_RECURSE JSON_SOURCES "${JSON_DIR}/*.c")
file(GLOB_RECURSE SYSTEM_SOURCES "${SYSTEM_DIR}/*.c")
file(GLOB_RECURSE UTIL_SOURCES "${UTIL_DIR}/*.c")

# Make sure json_helpers.c is included
list(APPEND JSON_SOURCES "${JSON_DIR}/json_helpers.c")

# Define specific HAL files to include (ESP32 only)
set(HAL_SOURCES 
    "${HAL_DIR}/esp32/hal_esp32.c"
    "${HAL_DIR}/esp32/mcp_esp32.c"
)

# Main MCP library
add_library(mcp_lib STATIC
    ${CORE_SOURCES}
    ${JSON_SOURCES}
    ${SYSTEM_SOURCES}
    ${UTIL_SOURCES}
    ${HAL_SOURCES}
)

# Main executable
add_executable(mcp_embedded
    ${SRC_DIR}/main.cpp
    ${HAL_DIR}/esp32/mcp_esp32.c
    ${HAL_DIR}/esp32/hal_esp32.c
)
target_link_libraries(mcp_embedded mcp_lib)

# Testing executables
add_executable(test_device_info
    ${SRC_DIR}/test/test_device_info.c
    ${SRC_DIR}/test/json_device_info_mock.c
)
target_link_libraries(test_device_info mcp_lib)

add_executable(test_dynamic_tools
    ${SRC_DIR}/test/test_dynamic_tools.c
    ${SRC_DIR}/test/json_mock.c
)
target_link_libraries(test_dynamic_tools mcp_lib)

add_executable(self_contained_test
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/self_contained_test.c
)

# Install targets
install(TARGETS mcp_embedded
    RUNTIME DESTINATION bin
)

# Testing
enable_testing()
add_test(NAME DeviceInfoTest COMMAND test_device_info)
add_test(NAME DynamicToolsTest COMMAND test_dynamic_tools)
add_test(NAME SelfContainedTest COMMAND self_contained_test)