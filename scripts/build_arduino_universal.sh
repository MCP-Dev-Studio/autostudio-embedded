#!/bin/bash
# Universal Arduino build script for MCP systems

# Exit on error
set -e

usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --board=NAME   - Specify the board to use (e.g. --board=esp32, --board=uno)"
    echo "  --port=PATH    - Specify the port for upload (e.g. --port=/dev/ttyUSB0)"
    echo "  --help         - Show this help message"
    exit 1
}

# Source directory
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SRC_DIR}/build/arduino_universal"
ARDUINO_DIR="${BUILD_DIR}/arduino_project"
SKETCH_NAME="MCP_Embedded"

# Default board and port
BOARD_FQBN="esp32:esp32:esp32"
PORT=""

# Parse arguments
for arg in "$@"
do
    case $arg in
        --board=*)
        BOARD_NAME="${arg#*=}"
        
        # Handle various board types
        case $BOARD_NAME in
            esp32)
                BOARD_FQBN="esp32:esp32:esp32"
                ;;
            uno)
                BOARD_FQBN="arduino:avr:uno"
                ;;
            nano)
                BOARD_FQBN="arduino:avr:nano"
                ;;
            mega)
                BOARD_FQBN="arduino:avr:mega"
                ;;
            nano33ble)
                BOARD_FQBN="arduino:mbed:nano33ble"
                ;;
            *)
                # Use board name as FQBN directly
                BOARD_FQBN="$BOARD_NAME"
                ;;
        esac
        ;;
        --port=*)
        PORT="${arg#*=}"
        ;;
        --help)
        usage
        ;;
        *)
        echo "Unknown option: $arg"
        usage
        ;;
    esac
done

echo "Using board FQBN: $BOARD_FQBN"
if [ -n "$PORT" ]; then
    echo "Using port: $PORT"
fi

# Create build directory
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
mkdir -p "${ARDUINO_DIR}/${SKETCH_NAME}"

# Create main sketch file with Arduino platform defines
(
    echo "#define MCP_OS_ARDUINO"
    echo "#define MCP_PLATFORM_ARDUINO"
    echo "#define MCP_CPP_FIXES"
    echo ""
    cat "$SRC_DIR/src/main.cpp"
) > "$ARDUINO_DIR/$SKETCH_NAME/$SKETCH_NAME.ino"

echo "Arduino project directory prepared: $ARDUINO_DIR/$SKETCH_NAME"

# Copy all necessary header files
echo "Copying essential header files..."

# Create central Arduino compatibility file
cp "$SRC_DIR/src/util/arduino_compat.h" "$ARDUINO_DIR/$SKETCH_NAME/arduino_compat.h"
cp "$SRC_DIR/src/util/platform_compatibility.h" "$ARDUINO_DIR/$SKETCH_NAME/platform_compatibility.h"
cp "$SRC_DIR/src/core/mcp/config/config_json.h" "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"

# Copy Arduino-specific HAL files
echo "Copying Arduino HAL files..."
if [ -d "$SRC_DIR/src/hal/arduino" ]; then
    # Copy .c files as .cpp for Arduino compatibility
    find "$SRC_DIR/src/hal/arduino" -name "*.c" | while read file; do
        base_name=$(basename "$file")
        cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/${base_name%.c}.cpp"
        echo "Copied Arduino HAL file: ${base_name%.c}.cpp"
    done
    
    # Copy .cpp files
    find "$SRC_DIR/src/hal/arduino" -name "*.cpp" | while read file; do
        base_name=$(basename "$file")
        cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
        echo "Copied Arduino HAL file: $base_name"
    done
    
    # Copy .h files
    find "$SRC_DIR/src/hal/arduino" -name "*.h" | while read file; do
        base_name=$(basename "$file")
        cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
        echo "Copied Arduino HAL header: $base_name"
    done
else
    echo "Warning: Arduino HAL directory not found: $SRC_DIR/src/hal/arduino"
fi

# Copy Arduino compatibility implementation
cp "$SRC_DIR/src/util/arduino_compat.c" "$ARDUINO_DIR/$SKETCH_NAME/arduino_compat.cpp"
echo "Copied arduino_compat.cpp"

# Create time utilities for Arduino
cat > "$ARDUINO_DIR/$SKETCH_NAME/mcp_time.h" << 'EOL'
/**
 * @file mcp_time.h
 * @brief Time utilities for MCP system
 */
#ifndef MCP_TIME_H
#define MCP_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get current time in milliseconds
 * 
 * @return uint64_t Current time in milliseconds
 */
uint64_t MCP_GetCurrentTimeMs(void);

#ifdef __cplusplus
}
#endif

#endif /* MCP_TIME_H */
EOL
echo "Created mcp_time.h"

cat > "$ARDUINO_DIR/$SKETCH_NAME/mcp_time_arduino.cpp" << 'EOL'
/**
 * @file mcp_time_arduino.cpp
 * @brief Time utilities for Arduino
 */

#include "mcp_time.h"
#include <Arduino.h>

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get current time in milliseconds
 * 
 * @return uint64_t Current time in milliseconds
 */
uint64_t MCP_GetCurrentTimeMs(void) {
    // Use Arduino millis() function
    return (uint64_t)millis();
}

/**
 * @brief Get system time in milliseconds
 * 
 * @return uint32_t System time in milliseconds
 */
uint32_t MCP_SystemGetTimeMs(void) {
    return (uint32_t)millis();
}

#ifdef __cplusplus
}
#endif

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
EOL
echo "Created mcp_time_arduino.cpp"

# Create Arduino logging implementation
cat > "$ARDUINO_DIR/$SKETCH_NAME/logging_arduino.cpp" << 'EOL'
/**
 * @file logging_arduino.cpp
 * @brief Arduino-specific logging implementation
 */
#include "logging.h"
#include <Arduino.h>

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

// Arduino-specific logging functions
void log_error(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print("[ERROR] ");
    Serial.println(buffer);
}

void log_warn(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print("[WARN] ");
    Serial.println(buffer);
}

void log_info(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print("[INFO] ");
    Serial.println(buffer);
}

void log_debug(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print("[DEBUG] ");
    Serial.println(buffer);
}

void log_trace(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print("[TRACE] ");
    Serial.println(buffer);
}

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
EOL
echo "Created logging_arduino.cpp"

# Copy all other headers
echo "Copying other header files..."
find "$SRC_DIR/src" -name "*.h" | while read header_file; do
    # Skip already copied files
    base_name=$(basename "$header_file")
    if [ ! -f "$ARDUINO_DIR/$SKETCH_NAME/$base_name" ]; then
        cp "$header_file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
        echo "Copied header: $base_name"
    fi
done

# Copy C files as .cpp (excluding HAL directories and test directories)
echo "Copying source files as .cpp..."
find "$SRC_DIR/src" \
    \( -path "$SRC_DIR/src/hal" -o -path "$SRC_DIR/src/test" \) -prune \
    -o -name "*.c" -print | while read file; do
    base_name=$(basename "$file")
    cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/${base_name%.c}.cpp"
done

# Copy existing .cpp files
find "$SRC_DIR/src" \
    \( -path "$SRC_DIR/src/hal" -o -path "$SRC_DIR/src/test" \) -prune \
    -o -name "*.cpp" -print | while read file; do
    base_name=$(basename "$file")
    cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
done

# Modify file contents for Arduino compatibility
echo "Modifying file contents for Arduino compatibility..."

# Add Arduino compatibility header to all source files
for cpp_file in "$ARDUINO_DIR/$SKETCH_NAME"/*.cpp; do
    if [ -f "$cpp_file" ]; then
        # Add arduino_compat.h include 
        sed -i '' '1s/^/#include "arduino_compat.h"\n/' "$cpp_file"
    fi
done

# Fix include paths and C++ compatibility issues
for file in "$ARDUINO_DIR/$SKETCH_NAME"/*.{cpp,h,ino}; do
    if [ -f "$file" ]; then
        echo "Processing file: $(basename "$file")"
        
        # Fix include paths: convert detailed paths to just the filename
        sed -i '' 's/#include[[:space:]]*"[^"]*\/\([^"]*\)"/#include "\1"/g' "$file"
        
        # Replace C++ reserved keyword "operator" with "op"
        sed -i '' 's/\([^a-zA-Z0-9_]\)operator\([^a-zA-Z0-9_]\)/\1op\2/g' "$file"
        sed -i '' 's/^operator /op /g' "$file"
        sed -i '' 's/ operator;/ op;/g' "$file"
        sed -i '' 's/\.operator/\.op/g' "$file"
        sed -i '' 's/->operator/->op/g' "$file"
        
        # Replace config.json include with config_json.h include
        sed -i '' 's/#include "config.json"/#include "config_json.h"/g' "$file"
        
        # Fix common function naming issues (MCP_ContentGetString)
        sed -i '' 's/MCP_ContentGetString(params, \([^)]*\))/MCP_ContentGetStringValue(params, \1)/g' "$file"
        
        # Fix resultJson field access
        sed -i '' 's/content->resultJson/MCP_ContentGetString(content)/g' "$file"
        
        # Fix time(NULL) calls to use Arduino time function
        sed -i '' 's/(double)time(NULL) \* 1000/(double)MCP_GetCurrentTimeMs()/g' "$file"
    fi
done

# Final verification 
echo "Performing final verification..."

# Check for incomplete include paths
INCOMPLETE_INCLUDES=$(grep -l '[a-zA-Z0-9_]\+/[a-zA-Z0-9_\.]\+"' "$ARDUINO_DIR/$SKETCH_NAME"/* 2>/dev/null || echo "")
if [ -n "$INCOMPLETE_INCLUDES" ]; then
    echo "Warning: Still found incomplete include paths:"
    
    # Attempt to fix any remaining issues
    for problem_file in $INCOMPLETE_INCLUDES; do
        echo "Fixing problem file: $(basename "$problem_file")"
        
        # Display problematic lines
        problem_lines=$(grep -n '[a-zA-Z0-9_]\+/[a-zA-Z0-9_\.]\+"' "$problem_file" || echo "")
        echo "$problem_lines"
        
        # Extract just the filename and replace the include
        echo "$problem_lines" | while IFS=: read -r line_num line_content; do
            header_name=$(echo "$line_content" | grep -o '[a-zA-Z0-9_]\+\.[a-zA-Z0-9_\.]\+\"' | sed 's/\"//')
            if [ -n "$header_name" ]; then
                echo "Line $line_num fixing: $line_content -> #include \"$header_name\""
                sed -i '' "${line_num}s|.*|#include \"$header_name\"|" "$problem_file"
            fi
        done
    done
else
    echo "No incomplete include paths found"
fi

# Check for "operator" keyword
OPERATOR_USES=$(grep -l '\boperator\b' "$ARDUINO_DIR/$SKETCH_NAME"/* 2>/dev/null || echo "")
if [ -n "$OPERATOR_USES" ]; then
    echo "Warning: Still found C++ operator keyword:"
    
    # More aggressive operator replacement
    for op_file in $OPERATOR_USES; do
        echo "Fixing operator in file: $(basename "$op_file")"
        grep -n '\boperator\b' "$op_file"
        sed -i '' 's/operator/op/g' "$op_file"
    done
else
    echo "No operator keyword issues found"
fi

echo "Arduino sketch preparation complete"

# Arduino project build
if command -v arduino-cli &> /dev/null; then
    # Define all build flags - add platform-specific macros
    CPP_COMMON_FLAGS="-DMCP_PLATFORM_ARDUINO -DMCP_OS_ARDUINO -DMCP_CPP_FIXES"
    
    # Prepare compiler command with C++ compatibility flags
    COMPILE_CMD="arduino-cli compile --verbose --fqbn \"$BOARD_FQBN\" \
        --build-property \"build.extra_flags=$CPP_COMMON_FLAGS\" \
        --build-property \"compiler.c.extra_flags=-w\" \
        --build-property \"compiler.cpp.extra_flags=-w -fpermissive -std=c++11 -Wno-error\""
    
    # Add port if specified
    if [ -n "$PORT" ]; then
        COMPILE_CMD="$COMPILE_CMD --port \"$PORT\""
    fi
    
    # Add target directory
    COMPILE_CMD="$COMPILE_CMD \"${ARDUINO_DIR}/${SKETCH_NAME}\""
    
    echo "Running build command: $COMPILE_CMD"
    
    # Execute build
    eval $COMPILE_CMD
    
    # Check build result
    if [ $? -eq 0 ]; then
        echo "Arduino sketch build successful!"
        
        # Upload if port specified
        if [ -n "$PORT" ]; then
            echo "Uploading to $PORT..."
            arduino-cli upload --fqbn "$BOARD_FQBN" --port "$PORT" "${ARDUINO_DIR}/${SKETCH_NAME}"
            
            if [ $? -eq 0 ]; then
                echo "Upload successful!"
            else
                echo "Upload failed."
            fi
        fi
    else
        echo "Build failed."
    fi
else
    echo "Arduino CLI not found. Please install it first."
    echo "Project has been set up for manual building at ${ARDUINO_DIR}"
fi