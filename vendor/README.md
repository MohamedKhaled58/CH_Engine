# Vendor Libraries

This directory contains external libraries and dependencies for the CH Engine.

## Required Libraries

### DirectXMath
- **Purpose**: Modern C++ math library for DirectX 11
- **Location**: `vendor/DirectXMath/`
- **Version**: Latest from Microsoft
- **Usage**: Replaces D3DX math functions

### spdlog
- **Purpose**: Fast C++ logging library
- **Location**: `vendor/spdlog/`
- **Version**: Latest stable
- **Usage**: Engine logging and debugging

### GLM (OpenGL Mathematics)
- **Purpose**: Mathematics library for graphics software
- **Location**: `vendor/glm/`
- **Version**: Latest stable
- **Usage**: Additional math utilities (optional)

### STB
- **Purpose**: Single-file public domain libraries
- **Location**: `vendor/stb/`
- **Version**: Latest
- **Usage**: Image loading (stb_image.h)

## Installation

1. Clone or download each library into its respective directory
2. Ensure the directory structure matches the premake5.lua configuration
3. Run `GenerateProjectFiles.bat` to set up the project

## Directory Structure

```
vendor/
├── DirectXMath/
│   └── include/
├── spdlog/
│   └── include/
├── glm/
│   └── glm/
├── stb/
│   └── *.h
└── bin/
    └── Premake/
        └── premake5.exe
```

## Notes

- All libraries are header-only or single-file libraries for simplicity
- No complex build dependencies required
- Libraries are treated as external modules, not hardcoded into the engine
- Premake5 configuration handles all include paths and linking 