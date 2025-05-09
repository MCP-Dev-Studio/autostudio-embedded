cmake_minimum_required(VERSION 3.10)
project(MCP_Embedded)

# Define HOST platform
add_compile_definitions(MCP_OS_HOST=1 MCP_PLATFORM_HOST=1)
message(STATUS "Building for HOST platform")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Find source files for the HOST platform - using consolidated files with conditional compilation
set(CORE_SOURCES
    src/core/kernel/memory_manager.c
    src/core/kernel/task_scheduler.c
    src/core/tool_system/tool_registry.c
    src/core/tool_system/tool_handler.c
    src/core/mcp/auth_manager.c
    src/core/mcp/server_init.c
    src/core/mcp/content.c
    src/core/mcp/content_api_helpers.c
)

# Use consolidated logging files
set(LOGGING_SOURCES
    src/core/mcp/logging/mcp_logging_tool.c
)

# Common sources for all platforms
set(COMMON_SOURCES
    src/json/json_parser.c
    src/system/logging.c
    src/system/mcp_system.c
    src/system/persistent_storage.c
    src/util/platform_compatibility.c  # Use consolidated platform_compatibility file
)

file(GLOB MAIN_SOURCE "src/main.cpp")

# Combine sources
set(SOURCES ${CORE_SOURCES} ${LOGGING_SOURCES} ${COMMON_SOURCES} ${MAIN_SOURCE})

# Create executable
add_executable(mcp_embedded ${SOURCES})

# Set output directory
set_target_properties(mcp_embedded PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin"
)

# Add C++11 support
target_compile_features(mcp_embedded PRIVATE cxx_std_11)

# Add debug symbols in debug mode
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
