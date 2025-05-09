# MCP Embedded Platform Guide

This guide provides instructions for setting up development environments and building the MCP Embedded System on each supported platform.

## Table of Contents

- [Platform Compatibility Approach](#platform-compatibility-approach)
- [Raspberry Pi](#raspberry-pi)
- [ESP32](#esp32)
- [Arduino](#arduino)
- [Mbed](#mbed)

## Platform Compatibility Approach

The MCP implementation uses a carefully designed cross-platform approach to ensure code works consistently across all supported platforms:

### Key Platform Compatibility Features

1. **Platform-specific code is isolated to designated areas:**
   - Hardware Abstraction Layer (HAL) in `src/hal/<platform>`
   - Platform-specific entry points in `src/main.cpp`

2. **Common platform-independent code is maintained in:**
   - Core MCP implementation in `src/core`
   - JSON utilities in `src/json`
   - System services in `src/system`
   - Utility functions in `src/util`

3. **Platform compatibility is provided through:**
   - `src/util/platform_compatibility.h` - Common type definitions and function declarations
   - `src/util/arduino_compat.h` - Arduino-specific compatibility layer (for ESP32/Arduino builds)

### Structure Consistency Across Platforms

The implementation ensures structure consistency across platforms:

1. **MCP_Content:**
   - Defined appropriately for each platform
   - Extended with resultJson field for Arduino compatibility

2. **MCP_ToolResult:**
   - Unified structure across platforms
   - ESP32-specific fields only included for ESP32 builds

### Platform Detection Macros

The following macros are used for platform detection:

```c
#if defined(MCP_OS_HOST) || defined(MCP_PLATFORM_HOST)
    // Host platform code
#elif defined(MCP_OS_ARDUINO) || defined(MCP_PLATFORM_ARDUINO)
    // Arduino platform code
#elif defined(MCP_OS_ESP32) || defined(MCP_PLATFORM_ESP32)
    // ESP32 native platform code
#elif defined(MCP_OS_RPI) || defined(MCP_PLATFORM_RPI)
    // Raspberry Pi platform code
#elif defined(MCP_OS_MBED) || defined(MCP_PLATFORM_MBED)
    // Mbed OS platform code
#endif
```

### Best Practices for Cross-Platform Development

1. **Use platform.h for platform-specific definitions**
   - Include common declarations in platform_compatibility.h
   - Use platform-specific extensions only in designated areas

2. **Avoid direct include of config.json**
   - Use config_json.h to embed the configuration

3. **Use the proper time functions**
   - Use MCP_GetCurrentTimeMs() for consistent time across platforms

4. **Be careful with C++ reserved keywords**
   - Avoid using words like `operator`, `new`, `class`, etc.

5. **Handle structure definitions carefully**
   - Use proper #ifdef guards to prevent duplicate definitions

## Raspberry Pi

### Prerequisites

1. **Hardware Requirements**:
   - Raspberry Pi (Model 3 or newer recommended)
   - microSD card with Raspberry Pi OS installed

2. **Software Requirements**:
   - Development tools:
     ```bash
     sudo apt update
     sudo apt install -y git cmake build-essential
     ```

### Building

```bash
# Clone the repository
git clone https://github.com/MCP-Dev-Studio/autostudio-embedded.git
cd autostudio-embedded

# Build for Raspberry Pi
./scripts/build.sh rpi
```

## ESP32

### Prerequisites

1. **Option A: ESP-IDF (Native ESP32 Development)**
   - Install ESP-IDF by following the [official guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
   - Set up environment variables:
     
     For Linux/macOS:
     ```bash
     . $HOME/esp/esp-idf/export.sh
     ```
     
     For Windows Command Prompt:
     ```cmd
     C:\esp\esp-idf\export.bat
     ```
     
     For Windows PowerShell:
     ```powershell
     C:\esp\esp-idf\export.ps1
     ```

2. **Option B: Arduino with ESP32 Support**
   - Install Arduino CLI
     ```bash
     curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
     export PATH="$HOME/bin:$PATH"  # Add to PATH if needed
     ```
     
     After installation, verify the CLI is in your PATH:
     ```bash
     arduino-cli version
     ```

   - Configure Arduino CLI with ESP32 board support
     ```bash
     # Initialize config
     arduino-cli config init
     
     # Add ESP32 board manager URL once
     arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     
     # Update index and install ESP32 board package
     arduino-cli core update-index
     arduino-cli core install esp32:esp32
     ```

### Building

```bash
# With ESP-IDF
./scripts/build.sh esp32

# With Arduino
./scripts/build.sh arduino
```

Inside the build.sh script, the ESP32 Arduino build performs operations similar to:
```bash
# This is what happens inside build.sh (example)
arduino-cli compile --fqbn esp32:esp32:esp32 path/to/project
```

## Arduino

### Prerequisites

1. **Install Arduino CLI**:
   ```bash
   curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
   export PATH="$HOME/bin:$PATH"  # Add to PATH if needed
   ```

2. **Install Board Support Packages**:
   ```bash
   # Initialize config
   arduino-cli config init
   
   # Update index
   arduino-cli core update-index
   
   # For standard Arduino boards (Uno, Mega, etc.)
   arduino-cli core install arduino:avr
   
   # For other boards as needed
   arduino-cli core install arduino:samd  # For Arduino Zero, MKR family
   arduino-cli core install arduino:mbed  # For Nano 33 BLE, Portenta
   ```

3. **Automatic Board Detection** (optional):
   You can automatically detect connected boards with:
   ```bash
   # List all connected boards
   arduino-cli board list
   ```
   
   On macOS, filter out system ports that aren't Arduino devices:
   ```bash
   # Filter out system ports on macOS
   PORT=$(arduino-cli board list | grep "/dev/" | grep -v "Bluetooth\|debug-console\|wlan-debug" | head -1 | awk '{print $1}')
   ```
   
   On Linux, focus on typical Arduino ports:
   ```bash
   # Get Arduino ports on Linux
   PORT=$(arduino-cli board list | grep "/dev/ttyUSB\|/dev/ttyACM" | head -1 | awk '{print $1}')
   ```

### Building

```bash
# Build for Arduino using the universal build script
./scripts/build_arduino_universal.sh --board=esp32

# For uploading to a specific port
./scripts/build_arduino_universal.sh --board=esp32 --port=/dev/ttyUSB0

# For other Arduino boards
./scripts/build_arduino_universal.sh --board=uno
./scripts/build_arduino_universal.sh --board=mega
./scripts/build_arduino_universal.sh --board=nano

# Legacy build script (might not handle all compatibility issues)
./scripts/build.sh arduino
```

The universal Arduino build script (`build_arduino_universal.sh`) handles all necessary compatibility issues:

1. **C++ Compatibility:**
   - Fixes include paths for flat Arduino directory structure
   - Replaces C++ reserved keywords with alternatives
   - Adds extern "C" declarations for cross-language linking

2. **Arduino Adaptation:**
   - Copies `.c` files as `.cpp` for Arduino compilation
   - Adds necessary Arduino compatibility headers
   - Creates stubs for platform-specific functions

3. **Config Management:**
   - Uses embedded config.json as string constant
   - Properly formats all paths for Arduino environment

## Mbed

### Prerequisites

1. **Install Mbed CLI**:
   ```bash
   pip install mbed-cli
   ```

2. **Install GNU Arm Embedded Toolchain**:
   - Download from [ARM Developer site](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
   - Add to PATH

3. **Configure Mbed CLI**:
   ```bash
   mbed config -G GCC_ARM_PATH "/path/to/arm-gcc/bin"
   ```

### Building

```bash
# Build for Mbed
./scripts/build.sh mbed

# For specific targets (manual build)
mbed target NUCLEO-F446RE
mbed toolchain GCC_ARM
mbed compile
```

## Common Issues

### ESP32
- **Serial port permission denied**: `sudo usermod -a -G dialout $USER` (log out and back in)
- **Upload fails**: Check cable, hold BOOT button while initiating upload
- **ESP-IDF environment not found**: Make sure you've run the export script from the correct location
- **ESP32 Arduino build fails with "operator" errors**: These are C++ reserved keyword conflicts. Use the `build_arduino_universal.sh` script which automatically addresses these issues.
- **ESP32 Arduino build fails with config.json not found**: This is fixed in the `build_arduino_universal.sh` script which uses config_json.h instead of directly including the JSON file.

### Arduino
- **Board not found**: Check USB connection, drivers
- **Compile errors**: Verify board support package is installed
- **Arduino CLI not in PATH**: Check installation and add $HOME/bin to your PATH
- **MCP_Content structure issues**: Ensure you're using the Arduino-compatible MCP_Content structure via arduino_compat.h
- **"resultJson" field not found**: This is often caused by the wrong MCP_Content structure. Make sure platform_compatibility.h and arduino_compat.h are properly included.
- **Inconsistent structure definitions**: Check that each structure is defined only once and properly guarded with #ifdef statements

### Mbed
- **Toolchain not found**: Verify PATH and mbed config settings
- **Compile errors**: Check board compatibility with MCP features

## Additional Resources

- [Arduino CLI Official Documentation](https://arduino.github.io/arduino-cli/latest/)
- [ESP-IDF Installation Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- [Mbed CLI Documentation](https://os.mbed.com/docs/mbed-os/v6.15/build-tools/mbed-cli.html)