# Project Organization

This document describes how the MCP Embedded System codebase has been organized for better maintainability and readability.

## Directory Structure

The project has been organized into a logical directory structure:

```
.
├── CMakeLists.txt        # Main CMake build configuration
├── README.md             # Project documentation
├── docs/                 # Detailed documentation
│   ├── BUILD_FIXES.md    # Description of build issue fixes
│   └── ORGANIZATION.md   # This file
├── scripts/              # Build and utility scripts
│   ├── build.sh          # Main build script (using CMake)
│   ├── test.sh           # Test execution script
│   └── clean.sh          # Clean build artifacts
├── src/                  # Source code
│   ├── core/             # Core MCP implementation
│   ├── driver/           # Device drivers
│   ├── hal/              # Hardware Abstraction Layer
│   ├── json/             # JSON parsing and generation
│   ├── main.cpp          # Main application entry point
│   ├── system/           # System services
│   ├── test/             # Test implementations and mocks
│   └── util/             # Utility functions and helpers
└── tests/                # Test cases
```

## Organizational Principles

The reorganization followed these key principles:

### 1. Separation of Concerns

- **Core**: Essential MCP protocol implementation
- **HAL**: Platform-specific code isolated in the Hardware Abstraction Layer
- **Driver**: Device-specific code for sensors and actuators
- **System**: Common system services like logging and storage
- **Test**: Test-specific code and mocks separated from production code

### 2. Build System Improvements

- Replaced multiple shell scripts with a CMake-based build system
- Added proper dependency management
- Set up test framework integration

### 3. Code Fixes

- Fixed duplicate symbol errors
- Implemented proper forward declarations
- Created specialized mock implementations for tests
- Removed circular dependencies

### 4. Documentation

- Added detailed documentation on the codebase organization
- Documented build fixes and solutions
- Created a comprehensive README with build instructions

## Mapping Between Old and New Locations

| Old Location | New Location |
|--------------|--------------|
| `*.c` in root | `src/util/` or appropriate subdirectory |
| `*.h` in root | `src/util/` or appropriate subdirectory |
| `main.cpp` | `src/main.cpp` |
| `test_*.c` | `src/test/` |
| `core/` | `src/core/` |
| `drivers/` | `src/driver/` |
| `platforms/` | `src/hal/` |
| `*.sh` | `scripts/` |

## Next Steps

1. **Update Include Paths**: Code might need updates to reflect new file locations
2. **Complete CMake Setup**: Fine-tune CMake configuration for each platform
3. **Add Unit Tests**: Add more comprehensive tests for each component
4. **Continuous Integration**: Set up CI/CD pipeline for automated testing