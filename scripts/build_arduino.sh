#!/bin/bash
# Build script for ESP32/Arduino with flat structure (removing bus controller folder structure)

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
BUILD_DIR="${SRC_DIR}/build/arduino_flat"
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

# Create and clean build directory
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
mkdir -p "${ARDUINO_DIR}/${SKETCH_NAME}"

# Create main sketch file
(
    echo "#define MCP_OS_ARDUINO"
    echo "#define MCP_PLATFORM_ARDUINO"
    echo "#define MCP_CPP_FIXES"  # Add C++ compatibility macro
    echo ""
    cat "$SRC_DIR/src/main.cpp"
) > "$ARDUINO_DIR/$SKETCH_NAME/$SKETCH_NAME.ino"

echo "Preparing Arduino project directory: $ARDUINO_DIR/$SKETCH_NAME"

# Copy all necessary header files first
echo "Copying header files..."

# Copy required header files directly
if [ -f "$SRC_DIR/src/system/logging.h" ]; then
    cp "$SRC_DIR/src/system/logging.h" "$ARDUINO_DIR/$SKETCH_NAME/logging.h"
    echo "Copied logging.h file"
fi

# Copy essential compatibility files
echo "Copying essential compatibility files..."
# Copy platform_compatibility.h
if [ -f "$SRC_DIR/src/util/platform_compatibility.h" ]; then
    cp "$SRC_DIR/src/util/platform_compatibility.h" "$ARDUINO_DIR/$SKETCH_NAME/platform_compatibility.h"
    echo "Copied platform_compatibility.h file"
fi
# Copy arduino_compat.h  
if [ -f "$SRC_DIR/src/util/arduino_compat.h" ]; then
    cp "$SRC_DIR/src/util/arduino_compat.h" "$ARDUINO_DIR/$SKETCH_NAME/arduino_compat.h"
    echo "Copied arduino_compat.h file"
fi
# Process config_json.h
if [ -f "$SRC_DIR/src/core/mcp/config/config_json.h" ]; then
    cp "$SRC_DIR/src/core/mcp/config/config_json.h" "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
    echo "Copied config_json.h file"
else
    # Convert config.json to header
    if [ -f "$SRC_DIR/src/core/mcp/config/config.json" ]; then
        echo "Converting config.json to config_json.h..."
        cat > "$ARDUINO_DIR/$SKETCH_NAME/config_json.h" << 'EOL'
/**
 * @file config_json.h
 * @brief Default configuration JSON as a string constant
 */
#ifndef CONFIG_JSON_H
#define CONFIG_JSON_H

static const char* DEFAULT_CONFIG_JSON = 
EOL
        
        # Convert each JSON line to C string
        sed 's/"/\\"/g' "$SRC_DIR/src/core/mcp/config/config.json" | sed 's/^/"/;s/$/\\n"/g' >> "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
        
        # Finalize header
        echo ";" >> "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
        echo "" >> "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
        echo "#endif /* CONFIG_JSON_H */" >> "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
        
        echo "Generated config_json.h file"
    else
        # Create default config_json.h
        cat > "$ARDUINO_DIR/$SKETCH_NAME/config_json.h" << 'EOL'
/**
 * @file config_json.h
 * @brief Default configuration JSON as a string constant
 */
#ifndef CONFIG_JSON_H
#define CONFIG_JSON_H

static const char* DEFAULT_CONFIG_JSON = "{}";

#endif /* CONFIG_JSON_H */
EOL
        echo "Created default config_json.h file"
    fi
fi

# Copy arduino_compat.c as arduino_compat.cpp
if [ -f "$SRC_DIR/src/util/arduino_compat.c" ]; then
    cp "$SRC_DIR/src/util/arduino_compat.c" "$ARDUINO_DIR/$SKETCH_NAME/arduino_compat.cpp"
    echo "Copied arduino_compat.c to arduino_compat.cpp"
fi

if [ -f "$SRC_DIR/src/arduino/mcp_arduino.h" ]; then
    cp "$SRC_DIR/src/arduino/mcp_arduino.h" "$ARDUINO_DIR/$SKETCH_NAME/mcp_arduino.h"
    echo "Copied mcp_arduino.h file"
fi

# Copy only Arduino platform-specific HAL files
echo "Copying Arduino HAL files..."
if [ -d "$SRC_DIR/src/hal/arduino" ]; then
    # Convert .c files to .cpp and copy
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
        echo "Copied Arduino HAL header file: $base_name"
    done
else
    echo "Warning: Arduino HAL directory not found: $SRC_DIR/src/hal/arduino"
fi

# Copy all files from other directories (excluding hal and test)
echo "Copying other source files (excluding non-Arduino HAL and test files)..."

# JSON file handling - exclude all .json files (C++ compatibility issues)
echo "Managing JSON files..."
echo "Not copying JSON files directly (provided as header files for C++ compatibility)"

# Check if JSON header file already exists
if [ ! -f "$ARDUINO_DIR/$SKETCH_NAME/config_json.h" ]; then
    # Copy config_json.h file
    if [ -f "$SRC_DIR/src/core/mcp/config/config_json.h" ]; then
        cp "$SRC_DIR/src/core/mcp/config/config_json.h" "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
        echo "Copied config_json.h file"
    else
        echo "Warning: config_json.h file not found"
    fi
fi

# Process all header files included by source code first
find "$SRC_DIR/src" -name "*.h" | while read header_file; do
    base_name=$(basename "$header_file")
    if [ ! -f "$ARDUINO_DIR/$SKETCH_NAME/$base_name" ]; then
        cp "$header_file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
        echo "Copied header file: $base_name"
    fi
done

# Convert .c files to .cpp and copy
find "$SRC_DIR/src" \
    \( -path "$SRC_DIR/src/hal" -o -path "$SRC_DIR/src/test" \) -prune \
    -o -name "*.c" -print | while read file; do
    base_name=$(basename "$file")
    cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/${base_name%.c}.cpp"
done

# Copy .cpp files
find "$SRC_DIR/src" \
    \( -path "$SRC_DIR/src/hal" -o -path "$SRC_DIR/src/test" \) -prune \
    -o -name "*.cpp" -print | while read file; do
    base_name=$(basename "$file")
    cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
done

# ------------------------ Safe File Content Modifications ------------------------
echo "Modifying file contents..."

# Fix include statements and C++ compatibility issues in all source files - macOS compatible approach
for file in "$ARDUINO_DIR/$SKETCH_NAME"/*.{cpp,h,ino}; do
    if [ -f "$file" ]; then
        echo "Processing file: $(basename "$file")"
        
        # Ensure arduino_compat.h is included first in source files
        if [[ "$file" == *.cpp ]] && [ "$(basename "$file")" != "arduino_compat.cpp" ]; then
            # Ensure arduino_compat.h appears first
            # Remove any existing arduino_compat.h include
            grep -v "#include \"arduino_compat.h\"" "$file" > "$file.tmp"
            # Add compatibility header at beginning of file
            echo '#include "arduino_compat.h"' > "$file"
            cat "$file.tmp" >> "$file"
            rm "$file.tmp"
        fi
        
        # Use simple sed commands - line by line modifications
        
        # 1. Fix normal include statements (e.g.: #include "path/to/header.h" -> #include "header.h")
        sed -i '' 's/#include[[:space:]]*"[^"]*\/\([^"]*\)"/#include "\1"/g' "$file"
        
        # 2. Fix incomplete include statements (e.g.: system/logging.h" -> #include "logging.h")
        sed -i '' 's/^\([[:space:]]*\)\([a-zA-Z0-9_]*\)\/\([a-zA-Z0-9_\.]*\)"/\1#include "\3"/g' "$file"
        
        # 3. Fix C++ reserved keyword "operator"
        sed -i '' 's/\([^a-zA-Z0-9_]\)operator\([^a-zA-Z0-9_]\)/\1op\2/g' "$file"
        sed -i '' 's/^operator /op /g' "$file"
        sed -i '' 's/ operator;/ op;/g' "$file"
        sed -i '' 's/\.operator/\.op/g' "$file"
        sed -i '' 's/->operator/->op/g' "$file"
        
        # 4. Replace config.json includes with config_json.h
        sed -i '' 's/#include "config.json"/#include "config_json.h"/g' "$file"
        
        # 5. Fix MCP_ContentGetString function calls 
        sed -i '' 's/MCP_ContentGetString(params, "moduleName", &moduleName)/MCP_ContentGetStringValue(params, "moduleName", \&moduleName)/g' "$file"
        sed -i '' 's/MCP_ContentGetString(params, "level", &levelStr)/MCP_ContentGetStringValue(params, "level", \&levelStr)/g' "$file"
        sed -i '' 's/MCP_ContentGetString(params, /MCP_ContentGetStringValue(params, /g' "$file"
        
        # 6. Handle time(NULL) calls
        sed -i '' 's/(double)time(NULL) \* 1000/(double)MCP_GetCurrentTimeMs()/g' "$file"
    fi
done

# ------------------------ Final Validation ------------------------
echo "Validating file contents..."

# Search for incomplete include statements
INCOMPLETE_INCLUDES=$(grep -l '[a-zA-Z0-9_]\+/[a-zA-Z0-9_\.]\+"' "$ARDUINO_DIR/$SKETCH_NAME"/* 2>/dev/null || echo "")
if [ -n "$INCOMPLETE_INCLUDES" ]; then
    echo "Warning: Still found incomplete include statements:"
    echo "$INCOMPLETE_INCLUDES"
    
    # Fix problematic files manually
    for problem_file in $INCOMPLETE_INCLUDES; do
        echo "Fixing problem file: $(basename "$problem_file")"
        
        # Display problematic lines
        problem_lines=$(grep -n '[a-zA-Z0-9_]\+/[a-zA-Z0-9_\.]\+"' "$problem_file" || echo "")
        echo "$problem_lines"
        
        # Fix each problematic line - macOS compatible version
        echo "$problem_lines" | while IFS=: read -r line_num line_content; do
            # Extract header name using regex
            header_name=$(echo "$line_content" | grep -o '[a-zA-Z0-9_]\+\.[a-zA-Z0-9_\.]\+\"' | sed 's/\"//')
            if [ -n "$header_name" ]; then
                # Fix the line
                echo "Line $line_num fixed: $line_content -> #include \"$header_name\""
                sed -i '' "${line_num}s|.*|#include \"$header_name\"|" "$problem_file"
            fi
        done
    done
else
    echo "Incomplete include statement issues resolved"
fi

# Fix config.json includes
echo "Fixing config.json include issues..."
for cpp_file in "$ARDUINO_DIR/$SKETCH_NAME"/*.cpp; do
    if grep -q '#include "config.json"' "$cpp_file"; then
        echo "Replacing config.json include in: $(basename "$cpp_file")"
        sed -i '' 's/#include "config.json"/#include "config_json.h"/' "$cpp_file"
    fi
done

# Fix MCP_ContentGetString function calls - match parameter count
echo "Fixing MCP_ContentGetString function call issues..."
for cpp_file in "$ARDUINO_DIR/$SKETCH_NAME"/*.cpp; do
    if grep -q 'MCP_ContentGetString.*params' "$cpp_file"; then
        echo "Fixing MCP_ContentGetString calls in: $(basename "$cpp_file")"
        sed -i '' 's/MCP_ContentGetString(params, "moduleName", &moduleName)/MCP_ContentGetStringValue(params, "moduleName", \&moduleName)/g' "$cpp_file"
        sed -i '' 's/MCP_ContentGetString(params, "level", &levelStr)/MCP_ContentGetStringValue(params, "level", \&levelStr)/g' "$cpp_file"
        sed -i '' 's/MCP_ContentGetString(params, /MCP_ContentGetStringValue(params, /g' "$cpp_file"
    fi
done

# Check for remaining operator keyword uses
OPERATOR_USES=$(grep -l '\boperator\b' "$ARDUINO_DIR/$SKETCH_NAME"/* 2>/dev/null || echo "")
if [ -n "$OPERATOR_USES" ]; then
    echo "Warning: Still found C++ reserved keyword 'operator':"
    for op_file in $OPERATOR_USES; do
        echo "File: $(basename "$op_file")"
        grep -n '\boperator\b' "$op_file"
        
        # Additional fix - more aggressive operator replacement
        sed -i '' 's/operator/op/g' "$op_file"
    done
else
    echo "C++ reserved keyword 'operator' issues resolved"
fi

echo "Flattened structure Arduino sketch created successfully"

# Build Arduino project
if command -v arduino-cli &> /dev/null; then
    # Define common build flags - add platform-specific macro definitions
    CPP_COMMON_FLAGS="-DMCP_PLATFORM_ARDUINO -DMCP_OS_ARDUINO -DMCP_CPP_FIXES"
    
    # Prepare compile command with improved C++ compatibility
    COMPILE_CMD="arduino-cli compile --verbose --fqbn \"$BOARD_FQBN\" \
        --build-property \"build.extra_flags=$CPP_COMMON_FLAGS\" \
        --build-property \"compiler.c.extra_flags=-w\" \
        --build-property \"compiler.cpp.extra_flags=-w -fpermissive -std=c++11 -Wno-error\""
    
    # Add port if specified
    if [ -n "$PORT" ]; then
        COMPILE_CMD="$COMPILE_CMD --port \"$PORT\""
    fi
    
    # Add target directory - compile main sketch file
    COMPILE_CMD="$COMPILE_CMD \"${ARDUINO_DIR}/${SKETCH_NAME}\""
    
    echo "Executing build command: $COMPILE_CMD"
    
    # Execute command
    eval $COMPILE_CMD
    
    # Display message if build succeeded
    if [ $? -eq 0 ]; then
        echo "Flattened structure Arduino sketch built successfully!"
        
        # Upload if port specified
        if [ -n "$PORT" ]; then
            echo "Uploading to board on port $PORT..."
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